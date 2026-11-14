#include "hiring.h"
#include "commission.h"
#include <stdio.h>
#include <string.h>

int hiring_handle_create_request(const AuthUser *user, const char *body, StrBuf *res_json, int *status_code) {
    if (strcmp(user->account_type, "client") != 0) {
        *status_code = 403;
        sb_append(res_json, "{\"error\":\"Only client accounts can send project requests.\"}");
        return 0;
    }

    int freelancer_id = 0;
    char title[256], description[2048], deadline[64], attachment_path[512];
    double budget = 0.0;

    if (!json_get_int(body, "freelancer_id", &freelancer_id) || freelancer_id <= 0 ||
        !json_get_string(body, "title", title, sizeof(title)) || !title[0] ||
        !json_get_string(body, "description", description, sizeof(description)) || !description[0] ||
        !json_get_double(body, "budget", &budget) || budget < 100.0 ||
        !json_get_string(body, "deadline", deadline, sizeof(deadline)) || !deadline[0]) {
        *status_code = 400;
        sb_append(res_json, "{\"error\":\"freelancer_id, title, description, budget (>= 100), and deadline are required.\"}");
        return 0;
    }
    json_get_string(body, "attachment_path", attachment_path, sizeof(attachment_path));

    sqlite3 *db = db_get_handle();
    /* Verify freelancer exists */
    sqlite3_stmt *chk = NULL;
    int f_exists = 0;
    if (sqlite3_prepare_v2(db, "SELECT id FROM freelancer_profiles WHERE id = ?;", -1, &chk, NULL) == SQLITE_OK) {
        sqlite3_bind_int(chk, 1, freelancer_id);
        if (sqlite3_step(chk) == SQLITE_ROW) f_exists = 1;
        sqlite3_finalize(chk);
    }
    if (!f_exists) {
        *status_code = 404;
        sb_append(res_json, "{\"error\":\"Selected freelancer not found.\"}");
        return 0;
    }

    sqlite3_stmt *stmt = NULL;
    const char *sql =
        "INSERT INTO project_requests (client_id, freelancer_id, title, description, budget, deadline, attachment_path, status) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, 'pending');";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        *status_code = 500;
        sb_append(res_json, "{\"error\":\"Failed to create project request.\"}");
        return 0;
    }
    sqlite3_bind_int(stmt, 1, user->id);
    sqlite3_bind_int(stmt, 2, freelancer_id);
    sqlite3_bind_text(stmt, 3, title, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, description, -1, SQLITE_STATIC);
    sqlite3_bind_double(stmt, 5, budget);
    sqlite3_bind_text(stmt, 6, deadline, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 7, attachment_path, -1, SQLITE_STATIC);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (rc != SQLITE_DONE) {
        *status_code = 500;
        sb_append(res_json, "{\"error\":\"Database error submitting project request.\"}");
        return 0;
    }

    int req_id = (int)sqlite3_last_insert_rowid(db);
    double rate = commission_get_rate();
    CommissionBreakdown cb = commission_calculate(budget, rate);

    *status_code = 201;
    sb_appendf(res_json,
        "{\"id\":%d,\"status\":\"pending\",\"commission_rate\":%.2f,\"platform_fee\":%.2f,\"freelancer_earnings\":%.2f,\"message\":\"Project request sent to freelancer.\"}",
        req_id, cb.commission_rate, cb.platform_earnings, cb.freelancer_earnings);
    return 1;
}

int hiring_handle_list_requests(const AuthUser *user, StrBuf *res_json, int *status_code) {
    sqlite3 *db = db_get_handle();
    sqlite3_stmt *stmt = NULL;
    const char *sql = strcmp(user->account_type, "freelancer") == 0
        ? "SELECT pr.id, pr.client_id, pr.freelancer_id, pr.title, pr.description, pr.budget, pr.deadline, pr.attachment_path, pr.status, pr.created_at, "
          "cu.full_name, cu.avatar_url, fu.full_name, fp.profession, fu.id "
          "FROM project_requests pr "
          "JOIN users cu ON pr.client_id = cu.id "
          "JOIN freelancer_profiles fp ON pr.freelancer_id = fp.id "
          "JOIN users fu ON fp.user_id = fu.id "
          "WHERE pr.freelancer_id = ? ORDER BY pr.id DESC;"
        : "SELECT pr.id, pr.client_id, pr.freelancer_id, pr.title, pr.description, pr.budget, pr.deadline, pr.attachment_path, pr.status, pr.created_at, "
          "cu.full_name, cu.avatar_url, fu.full_name, fp.profession, fu.id "
          "FROM project_requests pr "
          "JOIN users cu ON pr.client_id = cu.id "
          "JOIN freelancer_profiles fp ON pr.freelancer_id = fp.id "
          "JOIN users fu ON fp.user_id = fu.id "
          "WHERE pr.client_id = ? ORDER BY pr.id DESC;";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        *status_code = 500;
        sb_append(res_json, "{\"error\":\"Failed to fetch project requests.\"}");
        return 0;
    }

    int target_id = strcmp(user->account_type, "freelancer") == 0 ? user->freelancer_profile_id : user->id;
    sqlite3_bind_int(stmt, 1, target_id);

    double rate = commission_get_rate();
    sb_append(res_json, "{\"requests\":[");
    int first = 1;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        if (!first) sb_append(res_json, ",");
        first = 0;
        double budget = sqlite3_column_double(stmt, 5);
        CommissionBreakdown cb = commission_calculate(budget, rate);
        sb_append(res_json, "{");
        sb_appendf(res_json, "\"id\":%d,", sqlite3_column_int(stmt, 0));
        sb_appendf(res_json, "\"client_id\":%d,", sqlite3_column_int(stmt, 1));
        sb_appendf(res_json, "\"freelancer_id\":%d,", sqlite3_column_int(stmt, 2));
        sb_append(res_json, "\"title\":"); sb_append_json_str(res_json, (const char *)sqlite3_column_text(stmt, 3)); sb_append(res_json, ",");
        sb_append(res_json, "\"description\":"); sb_append_json_str(res_json, (const char *)sqlite3_column_text(stmt, 4)); sb_append(res_json, ",");
        sb_appendf(res_json, "\"budget\":%.2f,", budget);
        sb_appendf(res_json, "\"commission_rate\":%.2f,", cb.commission_rate);
        sb_appendf(res_json, "\"platform_fee\":%.2f,", cb.platform_earnings);
        sb_appendf(res_json, "\"freelancer_earnings\":%.2f,", cb.freelancer_earnings);
        sb_append(res_json, "\"deadline\":"); sb_append_json_str(res_json, (const char *)sqlite3_column_text(stmt, 6)); sb_append(res_json, ",");
        sb_append(res_json, "\"attachment_path\":"); sb_append_json_str(res_json, (const char *)sqlite3_column_text(stmt, 7)); sb_append(res_json, ",");
        sb_append(res_json, "\"status\":"); sb_append_json_str(res_json, (const char *)sqlite3_column_text(stmt, 8)); sb_append(res_json, ",");
        sb_append(res_json, "\"created_at\":"); sb_append_json_str(res_json, (const char *)sqlite3_column_text(stmt, 9)); sb_append(res_json, ",");
        sb_append(res_json, "\"client_name\":"); sb_append_json_str(res_json, (const char *)sqlite3_column_text(stmt, 10)); sb_append(res_json, ",");
        sb_append(res_json, "\"client_avatar\":"); sb_append_json_str(res_json, (const char *)sqlite3_column_text(stmt, 11)); sb_append(res_json, ",");
        sb_append(res_json, "\"freelancer_name\":"); sb_append_json_str(res_json, (const char *)sqlite3_column_text(stmt, 12)); sb_append(res_json, ",");
        sb_append(res_json, "\"freelancer_profession\":"); sb_append_json_str(res_json, (const char *)sqlite3_column_text(stmt, 13)); sb_append(res_json, ",");
        sb_appendf(res_json, "\"freelancer_user_id\":%d", sqlite3_column_int(stmt, 14));
        sb_append(res_json, "}");
    }
    sqlite3_finalize(stmt);
    sb_append(res_json, "]}");
    *status_code = 200;
    return 1;
}

int hiring_handle_respond_request(const AuthUser *user, int request_id, const char *action, StrBuf *res_json, int *status_code) {
    if (strcmp(user->account_type, "freelancer") != 0 || user->freelancer_profile_id <= 0) {
        *status_code = 403;
        sb_append(res_json, "{\"error\":\"Only the assigned freelancer can accept or reject requests.\"}");
        return 0;
    }

    sqlite3 *db = db_get_handle();
    sqlite3_stmt *stmt = NULL;
    const char *qsql = "SELECT client_id, freelancer_id, title, description, budget, deadline, status FROM project_requests WHERE id = ?;";
    if (sqlite3_prepare_v2(db, qsql, -1, &stmt, NULL) != SQLITE_OK) {
        *status_code = 500;
        sb_append(res_json, "{\"error\":\"Database error.\"}");
        return 0;
    }
    sqlite3_bind_int(stmt, 1, request_id);
    if (sqlite3_step(stmt) != SQLITE_ROW) {
        sqlite3_finalize(stmt);
        *status_code = 404;
        sb_append(res_json, "{\"error\":\"Project request not found.\"}");
        return 0;
    }

    int client_id = sqlite3_column_int(stmt, 0);
    int freelancer_id = sqlite3_column_int(stmt, 1);
    char title[256], description[2048], deadline[64], current_status[32];
    snprintf(title, sizeof(title), "%s", (const char *)sqlite3_column_text(stmt, 2));
    snprintf(description, sizeof(description), "%s", (const char *)sqlite3_column_text(stmt, 3));
    double budget = sqlite3_column_double(stmt, 4);
    snprintf(deadline, sizeof(deadline), "%s", (const char *)sqlite3_column_text(stmt, 5));
    snprintf(current_status, sizeof(current_status), "%s", (const char *)sqlite3_column_text(stmt, 6));
    sqlite3_finalize(stmt);

    if (freelancer_id != user->freelancer_profile_id) {
        *status_code = 403;
        sb_append(res_json, "{\"error\":\"Unauthorized for this project request.\"}");
        return 0;
    }
    if (strcmp(current_status, "pending") != 0) {
        *status_code = 400;
        sb_append(res_json, "{\"error\":\"Project request has already been processed.\"}");
        return 0;
    }

    const char *new_status = (strcmp(action, "accept") == 0) ? "accepted" : "rejected";
    sqlite3_stmt *ustmt = NULL;
    if (sqlite3_prepare_v2(db, "UPDATE project_requests SET status = ?, updated_at = CURRENT_TIMESTAMP WHERE id = ?;", -1, &ustmt, NULL) == SQLITE_OK) {
        sqlite3_bind_text(ustmt, 1, new_status, -1, SQLITE_STATIC);
        sqlite3_bind_int(ustmt, 2, request_id);
        sqlite3_step(ustmt);
        sqlite3_finalize(ustmt);
    }

    int project_id = 0;
    if (strcmp(new_status, "accepted") == 0) {
        double rate = commission_get_rate();
        CommissionBreakdown cb = commission_calculate(budget, rate);
        sqlite3_stmt *pstmt = NULL;
        const char *psql =
            "INSERT INTO projects (client_id, freelancer_id, request_id, title, description, budget, commission_rate, platform_fee, freelancer_earnings, deadline, status, progress_percent, payment_status) "
            "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, 'accepted', 10, 'unpaid');";
        if (sqlite3_prepare_v2(db, psql, -1, &pstmt, NULL) == SQLITE_OK) {
            sqlite3_bind_int(pstmt, 1, client_id);
            sqlite3_bind_int(pstmt, 2, freelancer_id);
            sqlite3_bind_int(pstmt, 3, request_id);
            sqlite3_bind_text(pstmt, 4, title, -1, SQLITE_STATIC);
            sqlite3_bind_text(pstmt, 5, description, -1, SQLITE_STATIC);
            sqlite3_bind_double(pstmt, 6, cb.budget);
            sqlite3_bind_double(pstmt, 7, cb.commission_rate);
            sqlite3_bind_double(pstmt, 8, cb.platform_earnings);
            sqlite3_bind_double(pstmt, 9, cb.freelancer_earnings);
            sqlite3_bind_text(pstmt, 10, deadline, -1, SQLITE_STATIC);
            if (sqlite3_step(pstmt) == SQLITE_DONE) {
                project_id = (int)sqlite3_last_insert_rowid(db);
            }
            sqlite3_finalize(pstmt);
        }
    }

    *status_code = 200;
    sb_appendf(res_json, "{\"status\":\"%s\",\"project_id\":%d}", new_status, project_id);
    return 1;
}
