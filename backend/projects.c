#include "projects.h"
#include <stdio.h>
#include <string.h>

int projects_handle_list(const AuthUser *user, StrBuf *res_json, int *status_code) {
    sqlite3 *db = db_get_handle();
    sqlite3_stmt *stmt = NULL;
    const char *sql = strcmp(user->account_type, "freelancer") == 0
        ? "SELECT p.id, p.client_id, p.freelancer_id, p.title, p.description, p.budget, p.commission_rate, p.platform_fee, p.freelancer_earnings, "
          "p.deadline, p.status, p.progress_percent, p.payment_status, p.created_at, p.updated_at, "
          "cu.full_name, cu.avatar_url, fu.full_name, fp.profession, fu.id, "
          "COALESCE((SELECT r.id FROM reviews r WHERE r.project_id = p.id), 0) "
          "FROM projects p "
          "JOIN users cu ON p.client_id = cu.id "
          "JOIN freelancer_profiles fp ON p.freelancer_id = fp.id "
          "JOIN users fu ON fp.user_id = fu.id "
          "WHERE p.freelancer_id = ? ORDER BY p.id DESC;"
        : "SELECT p.id, p.client_id, p.freelancer_id, p.title, p.description, p.budget, p.commission_rate, p.platform_fee, p.freelancer_earnings, "
          "p.deadline, p.status, p.progress_percent, p.payment_status, p.created_at, p.updated_at, "
          "cu.full_name, cu.avatar_url, fu.full_name, fp.profession, fu.id, "
          "COALESCE((SELECT r.id FROM reviews r WHERE r.project_id = p.id), 0) "
          "FROM projects p "
          "JOIN users cu ON p.client_id = cu.id "
          "JOIN freelancer_profiles fp ON p.freelancer_id = fp.id "
          "JOIN users fu ON fp.user_id = fu.id "
          "WHERE p.client_id = ? ORDER BY p.id DESC;";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        *status_code = 500;
        sb_append(res_json, "{\"error\":\"Failed to load projects.\"}");
        return 0;
    }
    sqlite3_bind_int(stmt, 1, strcmp(user->account_type, "freelancer") == 0 ? user->freelancer_profile_id : user->id);

    sb_append(res_json, "{\"projects\":[");
    int first = 1;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        if (!first) sb_append(res_json, ",");
        first = 0;
        sb_append(res_json, "{");
        sb_appendf(res_json, "\"id\":%d,", sqlite3_column_int(stmt, 0));
        sb_appendf(res_json, "\"client_id\":%d,", sqlite3_column_int(stmt, 1));
        sb_appendf(res_json, "\"freelancer_id\":%d,", sqlite3_column_int(stmt, 2));
        sb_append(res_json, "\"title\":"); sb_append_json_str(res_json, (const char *)sqlite3_column_text(stmt, 3)); sb_append(res_json, ",");
        sb_append(res_json, "\"description\":"); sb_append_json_str(res_json, (const char *)sqlite3_column_text(stmt, 4)); sb_append(res_json, ",");
        sb_appendf(res_json, "\"budget\":%.2f,", sqlite3_column_double(stmt, 5));
        sb_appendf(res_json, "\"commission_rate\":%.2f,", sqlite3_column_double(stmt, 6));
        sb_appendf(res_json, "\"platform_fee\":%.2f,", sqlite3_column_double(stmt, 7));
        sb_appendf(res_json, "\"freelancer_earnings\":%.2f,", sqlite3_column_double(stmt, 8));
        sb_append(res_json, "\"deadline\":"); sb_append_json_str(res_json, (const char *)sqlite3_column_text(stmt, 9)); sb_append(res_json, ",");
        sb_append(res_json, "\"status\":"); sb_append_json_str(res_json, (const char *)sqlite3_column_text(stmt, 10)); sb_append(res_json, ",");
        sb_appendf(res_json, "\"progress_percent\":%d,", sqlite3_column_int(stmt, 11));
        sb_append(res_json, "\"payment_status\":"); sb_append_json_str(res_json, (const char *)sqlite3_column_text(stmt, 12)); sb_append(res_json, ",");
        sb_append(res_json, "\"created_at\":"); sb_append_json_str(res_json, (const char *)sqlite3_column_text(stmt, 13)); sb_append(res_json, ",");
        sb_append(res_json, "\"updated_at\":"); sb_append_json_str(res_json, (const char *)sqlite3_column_text(stmt, 14)); sb_append(res_json, ",");
        sb_append(res_json, "\"client_name\":"); sb_append_json_str(res_json, (const char *)sqlite3_column_text(stmt, 15)); sb_append(res_json, ",");
        sb_append(res_json, "\"client_avatar\":"); sb_append_json_str(res_json, (const char *)sqlite3_column_text(stmt, 16)); sb_append(res_json, ",");
        sb_append(res_json, "\"freelancer_name\":"); sb_append_json_str(res_json, (const char *)sqlite3_column_text(stmt, 17)); sb_append(res_json, ",");
        sb_append(res_json, "\"freelancer_profession\":"); sb_append_json_str(res_json, (const char *)sqlite3_column_text(stmt, 18)); sb_append(res_json, ",");
        sb_appendf(res_json, "\"freelancer_user_id\":%d,", sqlite3_column_int(stmt, 19));
        sb_appendf(res_json, "\"has_review\":%d", sqlite3_column_int(stmt, 20) > 0 ? 1 : 0);
        sb_append(res_json, "}");
    }
    sqlite3_finalize(stmt);
    sb_append(res_json, "]}");
    *status_code = 200;
    return 1;
}

int projects_handle_update_status(const AuthUser *user, int project_id, const char *body, StrBuf *res_json, int *status_code) {
    char new_status[32] = "";
    int progress = -1;
    json_get_string(body, "status", new_status, sizeof(new_status));
    json_get_int(body, "progress_percent", &progress);

    if (!new_status[0] && progress < 0) {
        *status_code = 400;
        sb_append(res_json, "{\"error\":\"status or progress_percent is required.\"}");
        return 0;
    }

    sqlite3 *db = db_get_handle();
    sqlite3_stmt *q = NULL;
    if (sqlite3_prepare_v2(db, "SELECT client_id, freelancer_id, status, progress_percent FROM projects WHERE id = ?;", -1, &q, NULL) != SQLITE_OK) {
        *status_code = 500;
        sb_append(res_json, "{\"error\":\"Database error.\"}");
        return 0;
    }
    sqlite3_bind_int(q, 1, project_id);
    if (sqlite3_step(q) != SQLITE_ROW) {
        sqlite3_finalize(q);
        *status_code = 404;
        sb_append(res_json, "{\"error\":\"Project not found.\"}");
        return 0;
    }
    int client_id = sqlite3_column_int(q, 0);
    int freelancer_id = sqlite3_column_int(q, 1);
    char cur_status[32];
    snprintf(cur_status, sizeof(cur_status), "%s", (const char *)sqlite3_column_text(q, 2));
    int cur_progress = sqlite3_column_int(q, 3);
    sqlite3_finalize(q);

    int is_client = (user->id == client_id);
    int is_freelancer = (user->freelancer_profile_id > 0 && user->freelancer_profile_id == freelancer_id);
    if (!is_client && !is_freelancer) {
        *status_code = 403;
        sb_append(res_json, "{\"error\":\"Unauthorized to update this project.\"}");
        return 0;
    }

    if (!new_status[0]) snprintf(new_status, sizeof(new_status), "%s", cur_status);
    if (strcmp(new_status, "accepted") != 0 && strcmp(new_status, "in_progress") != 0 &&
        strcmp(new_status, "completed") != 0 && strcmp(new_status, "cancelled") != 0) {
        *status_code = 400;
        sb_append(res_json, "{\"error\":\"Invalid project status.\"}");
        return 0;
    }
    if (progress < 0) {
        if (strcmp(new_status, "completed") == 0) progress = 100;
        else if (strcmp(new_status, "in_progress") == 0 && cur_progress < 25) progress = 50;
        else progress = cur_progress;
    }
    if (progress > 100) progress = 100;

    sqlite3_stmt *ustmt = NULL;
    if (sqlite3_prepare_v2(db, "UPDATE projects SET status = ?, progress_percent = ?, updated_at = CURRENT_TIMESTAMP WHERE id = ?;", -1, &ustmt, NULL) == SQLITE_OK) {
        sqlite3_bind_text(ustmt, 1, new_status, -1, SQLITE_STATIC);
        sqlite3_bind_int(ustmt, 2, progress);
        sqlite3_bind_int(ustmt, 3, project_id);
        sqlite3_step(ustmt);
        sqlite3_finalize(ustmt);
    }

    if (strcmp(cur_status, "completed") != 0 && strcmp(new_status, "completed") == 0) {
        sqlite3_stmt *cstmt = NULL;
        if (sqlite3_prepare_v2(db, "UPDATE freelancer_profiles SET completed_count = completed_count + 1 WHERE id = ?;", -1, &cstmt, NULL) == SQLITE_OK) {
            sqlite3_bind_int(cstmt, 1, freelancer_id);
            sqlite3_step(cstmt);
            sqlite3_finalize(cstmt);
        }
    }

    *status_code = 200;
    sb_appendf(res_json, "{\"id\":%d,\"status\":\"%s\",\"progress_percent\":%d}", project_id, new_status, progress);
    return 1;
}

int projects_handle_create_review(const AuthUser *user, int project_id, const char *body, StrBuf *res_json, int *status_code) {
    int rating = 5;
    char comment[1024];
    if (!json_get_int(body, "rating", &rating) || rating < 1 || rating > 5 ||
        !json_get_string(body, "comment", comment, sizeof(comment)) || strlen(comment) < 3) {
        *status_code = 400;
        sb_append(res_json, "{\"error\":\"Rating (1-5) and comment are required.\"}");
        return 0;
    }

    sqlite3 *db = db_get_handle();
    sqlite3_stmt *q = NULL;
    if (sqlite3_prepare_v2(db, "SELECT client_id, freelancer_id, status FROM projects WHERE id = ?;", -1, &q, NULL) != SQLITE_OK) {
        *status_code = 500;
        sb_append(res_json, "{\"error\":\"Database error.\"}");
        return 0;
    }
    sqlite3_bind_int(q, 1, project_id);
    if (sqlite3_step(q) != SQLITE_ROW) {
        sqlite3_finalize(q);
        *status_code = 404;
        sb_append(res_json, "{\"error\":\"Project not found.\"}");
        return 0;
    }
    int client_id = sqlite3_column_int(q, 0);
    int freelancer_id = sqlite3_column_int(q, 1);
    const char *status = (const char *)sqlite3_column_text(q, 2);
    int is_completed = (status && strcmp(status, "completed") == 0);
    sqlite3_finalize(q);

    if (client_id != user->id) {
        *status_code = 403;
        sb_append(res_json, "{\"error\":\"Only the project client can submit a review.\"}");
        return 0;
    }
    if (!is_completed) {
        *status_code = 400;
        sb_append(res_json, "{\"error\":\"You can review a project once it is marked completed.\"}");
        return 0;
    }

    sqlite3_stmt *rstmt = NULL;
    if (sqlite3_prepare_v2(db, "INSERT INTO reviews (project_id, client_id, freelancer_id, rating, comment) VALUES (?, ?, ?, ?, ?);", -1, &rstmt, NULL) != SQLITE_OK) {
        *status_code = 500;
        sb_append(res_json, "{\"error\":\"Database error.\"}");
        return 0;
    }
    sqlite3_bind_int(rstmt, 1, project_id);
    sqlite3_bind_int(rstmt, 2, client_id);
    sqlite3_bind_int(rstmt, 3, freelancer_id);
    sqlite3_bind_int(rstmt, 4, rating);
    sqlite3_bind_text(rstmt, 5, comment, -1, SQLITE_STATIC);
    int rc = sqlite3_step(rstmt);
    sqlite3_finalize(rstmt);

    if (rc != SQLITE_DONE) {
        *status_code = 409;
        sb_append(res_json, "{\"error\":\"A review has already been submitted for this project.\"}");
        return 0;
    }

    sqlite3_stmt *ustmt = NULL;
    const char *usql =
        "UPDATE freelancer_profiles SET "
        "rating_avg = COALESCE((SELECT ROUND(AVG(rating), 2) FROM reviews WHERE freelancer_id = ?), 5.0), "
        "review_count = (SELECT COUNT(*) FROM reviews WHERE freelancer_id = ?) "
        "WHERE id = ?;";
    if (sqlite3_prepare_v2(db, usql, -1, &ustmt, NULL) == SQLITE_OK) {
        sqlite3_bind_int(ustmt, 1, freelancer_id);
        sqlite3_bind_int(ustmt, 2, freelancer_id);
        sqlite3_bind_int(ustmt, 3, freelancer_id);
        sqlite3_step(ustmt);
        sqlite3_finalize(ustmt);
    }

    *status_code = 201;
    sb_append(res_json, "{\"ok\":true,\"message\":\"Review submitted successfully.\"}");
    return 1;
}

int projects_handle_list_conversations(const AuthUser *user, StrBuf *res_json, int *status_code) {
    sqlite3 *db = db_get_handle();
    sqlite3_stmt *stmt = NULL;
    const char *sql =
        "SELECT u.id, u.full_name, u.account_type, u.avatar_url, COALESCE(fp.profession, 'Client'), "
        "COALESCE((SELECT m2.content FROM messages m2 WHERE (m2.sender_id = u.id AND m2.receiver_id = ?) OR (m2.sender_id = ? AND m2.receiver_id = u.id) ORDER BY m2.id DESC LIMIT 1), ''), "
        "COALESCE((SELECT m2.created_at FROM messages m2 WHERE (m2.sender_id = u.id AND m2.receiver_id = ?) OR (m2.sender_id = ? AND m2.receiver_id = u.id) ORDER BY m2.id DESC LIMIT 1), '') "
        "FROM users u "
        "LEFT JOIN freelancer_profiles fp ON fp.user_id = u.id "
        "WHERE u.id != ? AND ( "
        "  EXISTS (SELECT 1 FROM messages m WHERE (m.sender_id = u.id AND m.receiver_id = ?) OR (m.sender_id = ? AND m.receiver_id = u.id)) "
        "  OR EXISTS (SELECT 1 FROM projects p JOIN freelancer_profiles fp2 ON p.freelancer_id = fp2.id WHERE (p.client_id = ? AND fp2.user_id = u.id) OR (fp2.user_id = ? AND p.client_id = u.id)) "
        "  OR EXISTS (SELECT 1 FROM project_requests pr JOIN freelancer_profiles fp3 ON pr.freelancer_id = fp3.id WHERE (pr.client_id = ? AND fp3.user_id = u.id) OR (fp3.user_id = ? AND pr.client_id = u.id)) "
        ") "
        "ORDER BY 7 DESC, u.id ASC;";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        *status_code = 500;
        sb_append(res_json, "{\"error\":\"Failed to load conversations.\"}");
        return 0;
    }
    for (int i = 1; i <= 11; i++) sqlite3_bind_int(stmt, i, user->id);

    sb_append(res_json, "{\"conversations\":[");
    int first = 1;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        if (!first) sb_append(res_json, ",");
        first = 0;
        sb_append(res_json, "{");
        sb_appendf(res_json, "\"user_id\":%d,", sqlite3_column_int(stmt, 0));
        sb_append(res_json, "\"full_name\":"); sb_append_json_str(res_json, (const char *)sqlite3_column_text(stmt, 1)); sb_append(res_json, ",");
        sb_append(res_json, "\"account_type\":"); sb_append_json_str(res_json, (const char *)sqlite3_column_text(stmt, 2)); sb_append(res_json, ",");
        sb_append(res_json, "\"avatar_url\":"); sb_append_json_str(res_json, (const char *)sqlite3_column_text(stmt, 3)); sb_append(res_json, ",");
        sb_append(res_json, "\"subtitle\":"); sb_append_json_str(res_json, (const char *)sqlite3_column_text(stmt, 4)); sb_append(res_json, ",");
        sb_append(res_json, "\"last_message\":"); sb_append_json_str(res_json, (const char *)sqlite3_column_text(stmt, 5)); sb_append(res_json, ",");
        sb_append(res_json, "\"last_timestamp\":"); sb_append_json_str(res_json, (const char *)sqlite3_column_text(stmt, 6));
        sb_append(res_json, "}");
    }
    sqlite3_finalize(stmt);
    sb_append(res_json, "]}");
    *status_code = 200;
    return 1;
}

int projects_handle_get_messages(const AuthUser *user, int peer_user_id, StrBuf *res_json, int *status_code) {
    sqlite3 *db = db_get_handle();
    sqlite3_stmt *rstmt = NULL;
    if (sqlite3_prepare_v2(db, "UPDATE messages SET is_read = 1 WHERE sender_id = ? AND receiver_id = ?;", -1, &rstmt, NULL) == SQLITE_OK) {
        sqlite3_bind_int(rstmt, 1, peer_user_id);
        sqlite3_bind_int(rstmt, 2, user->id);
        sqlite3_step(rstmt);
        sqlite3_finalize(rstmt);
    }

    sqlite3_stmt *stmt = NULL;
    const char *sql =
        "SELECT m.id, m.sender_id, m.receiver_id, COALESCE(m.project_id, 0), m.content, m.is_read, m.created_at, u.full_name "
        "FROM messages m JOIN users u ON m.sender_id = u.id "
        "WHERE (m.sender_id = ? AND m.receiver_id = ?) OR (m.sender_id = ? AND m.receiver_id = ?) "
        "ORDER BY m.id ASC;";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        *status_code = 500;
        sb_append(res_json, "{\"error\":\"Failed to fetch messages.\"}");
        return 0;
    }
    sqlite3_bind_int(stmt, 1, user->id);
    sqlite3_bind_int(stmt, 2, peer_user_id);
    sqlite3_bind_int(stmt, 3, peer_user_id);
    sqlite3_bind_int(stmt, 4, user->id);

    sb_append(res_json, "{\"messages\":[");
    int first = 1;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        if (!first) sb_append(res_json, ",");
        first = 0;
        sb_append(res_json, "{");
        sb_appendf(res_json, "\"id\":%d,", sqlite3_column_int(stmt, 0));
        sb_appendf(res_json, "\"sender_id\":%d,", sqlite3_column_int(stmt, 1));
        sb_appendf(res_json, "\"receiver_id\":%d,", sqlite3_column_int(stmt, 2));
        sb_appendf(res_json, "\"project_id\":%d,", sqlite3_column_int(stmt, 3));
        sb_append(res_json, "\"content\":"); sb_append_json_str(res_json, (const char *)sqlite3_column_text(stmt, 4)); sb_append(res_json, ",");
        sb_appendf(res_json, "\"is_read\":%d,", sqlite3_column_int(stmt, 5));
        sb_append(res_json, "\"created_at\":"); sb_append_json_str(res_json, (const char *)sqlite3_column_text(stmt, 6)); sb_append(res_json, ",");
        sb_append(res_json, "\"sender_name\":"); sb_append_json_str(res_json, (const char *)sqlite3_column_text(stmt, 7));
        sb_append(res_json, "}");
    }
    sqlite3_finalize(stmt);
    sb_append(res_json, "]}");
    *status_code = 200;
    return 1;
}

int projects_handle_send_message(const AuthUser *user, const char *body, StrBuf *res_json, int *status_code) {
    int receiver_id = 0, project_id = 0;
    char content[2048];
    if (!json_get_int(body, "receiver_id", &receiver_id) || receiver_id <= 0 ||
        !json_get_string(body, "content", content, sizeof(content)) || !content[0]) {
        *status_code = 400;
        sb_append(res_json, "{\"error\":\"receiver_id and content are required.\"}");
        return 0;
    }
    json_get_int(body, "project_id", &project_id);

    sqlite3 *db = db_get_handle();
    sqlite3_stmt *stmt = NULL;
    const char *sql = project_id > 0
        ? "INSERT INTO messages (sender_id, receiver_id, project_id, content) VALUES (?, ?, ?, ?);"
        : "INSERT INTO messages (sender_id, receiver_id, project_id, content) VALUES (?, ?, NULL, ?);";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        *status_code = 500;
        sb_append(res_json, "{\"error\":\"Database error.\"}");
        return 0;
    }
    sqlite3_bind_int(stmt, 1, user->id);
    sqlite3_bind_int(stmt, 2, receiver_id);
    if (project_id > 0) {
        sqlite3_bind_int(stmt, 3, project_id);
        sqlite3_bind_text(stmt, 4, content, -1, SQLITE_STATIC);
    } else {
        sqlite3_bind_text(stmt, 3, content, -1, SQLITE_STATIC);
    }

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (rc != SQLITE_DONE) {
        *status_code = 500;
        sb_append(res_json, "{\"error\":\"Failed to send message.\"}");
        return 0;
    }

    int msg_id = (int)sqlite3_last_insert_rowid(db);
    *status_code = 201;
    sb_appendf(res_json, "{\"id\":%d,\"sender_id\":%d,\"receiver_id\":%d}", msg_id, user->id, receiver_id);
    return 1;
}
