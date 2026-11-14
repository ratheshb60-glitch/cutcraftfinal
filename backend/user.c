#include "user.h"
#include <stdio.h>
#include <string.h>

int user_handle_update_profile(const AuthUser *user, const char *body, StrBuf *res_json, int *status_code) {
    char full_name[128], avatar_url[256];
    int has_name = json_get_string(body, "full_name", full_name, sizeof(full_name));
    int has_avatar = json_get_string(body, "avatar_url", avatar_url, sizeof(avatar_url));

    if (!has_name && !has_avatar) {
        *status_code = 400;
        sb_append(res_json, "{\"error\":\"No profile fields provided.\"}");
        return 0;
    }

    sqlite3 *db = db_get_handle();
    if (has_name && strlen(full_name) >= 2) {
        sqlite3_stmt *stmt = NULL;
        if (sqlite3_prepare_v2(db, "UPDATE users SET full_name = ? WHERE id = ?;", -1, &stmt, NULL) == SQLITE_OK) {
            sqlite3_bind_text(stmt, 1, full_name, -1, SQLITE_STATIC);
            sqlite3_bind_int(stmt, 2, user->id);
            sqlite3_step(stmt);
            sqlite3_finalize(stmt);
        }
    }
    if (has_avatar && avatar_url[0]) {
        sqlite3_stmt *stmt = NULL;
        if (sqlite3_prepare_v2(db, "UPDATE users SET avatar_url = ? WHERE id = ?;", -1, &stmt, NULL) == SQLITE_OK) {
            sqlite3_bind_text(stmt, 1, avatar_url, -1, SQLITE_STATIC);
            sqlite3_bind_int(stmt, 2, user->id);
            sqlite3_step(stmt);
            sqlite3_finalize(stmt);
        }
    }

    AuthUser refreshed;
    sqlite3_stmt *q = NULL;
    const char *sql =
        "SELECT u.id, u.full_name, u.email, u.account_type, u.avatar_url, COALESCE(fp.id, 0) "
        "FROM users u LEFT JOIN freelancer_profiles fp ON fp.user_id = u.id WHERE u.id = ?;";
    if (sqlite3_prepare_v2(db, sql, -1, &q, NULL) == SQLITE_OK) {
        sqlite3_bind_int(q, 1, user->id);
        if (sqlite3_step(q) == SQLITE_ROW) {
            refreshed.id = sqlite3_column_int(q, 0);
            snprintf(refreshed.full_name, sizeof(refreshed.full_name), "%s", (const char *)sqlite3_column_text(q, 1));
            snprintf(refreshed.email, sizeof(refreshed.email), "%s", (const char *)sqlite3_column_text(q, 2));
            snprintf(refreshed.account_type, sizeof(refreshed.account_type), "%s", (const char *)sqlite3_column_text(q, 3));
            const char *av = (const char *)sqlite3_column_text(q, 4);
            snprintf(refreshed.avatar_url, sizeof(refreshed.avatar_url), "%s", av ? av : "");
            refreshed.freelancer_profile_id = sqlite3_column_int(q, 5);
        }
        sqlite3_finalize(q);
    }

    return auth_handle_me(&refreshed, res_json, status_code);
}

int user_handle_get_by_id(int user_id, StrBuf *res_json, int *status_code) {
    sqlite3 *db = db_get_handle();
    sqlite3_stmt *stmt = NULL;
    const char *sql = "SELECT id, full_name, account_type, avatar_url, created_at FROM users WHERE id = ?;";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        *status_code = 500;
        sb_append(res_json, "{\"error\":\"Database error.\"}");
        return 0;
    }
    sqlite3_bind_int(stmt, 1, user_id);
    if (sqlite3_step(stmt) != SQLITE_ROW) {
        sqlite3_finalize(stmt);
        *status_code = 404;
        sb_append(res_json, "{\"error\":\"User not found.\"}");
        return 0;
    }

    *status_code = 200;
    sb_append(res_json, "{\"user\":{");
    sb_appendf(res_json, "\"id\":%d,", sqlite3_column_int(stmt, 0));
    sb_append(res_json, "\"full_name\":"); sb_append_json_str(res_json, (const char *)sqlite3_column_text(stmt, 1)); sb_append(res_json, ",");
    sb_append(res_json, "\"account_type\":"); sb_append_json_str(res_json, (const char *)sqlite3_column_text(stmt, 2)); sb_append(res_json, ",");
    sb_append(res_json, "\"avatar_url\":"); sb_append_json_str(res_json, (const char *)sqlite3_column_text(stmt, 3)); sb_append(res_json, ",");
    sb_append(res_json, "\"created_at\":"); sb_append_json_str(res_json, (const char *)sqlite3_column_text(stmt, 4));
    sb_append(res_json, "}}");
    sqlite3_finalize(stmt);
    return 1;
}
