#include "freelancer.h"
#include <stdio.h>
#include <string.h>

static void append_freelancer_row_json(StrBuf *sb, sqlite3_stmt *stmt) {
    sb_append(sb, "{");
    sb_appendf(sb, "\"id\":%d,", sqlite3_column_int(stmt, 0));
    sb_appendf(sb, "\"user_id\":%d,", sqlite3_column_int(stmt, 1));
    sb_append(sb, "\"full_name\":"); sb_append_json_str(sb, (const char *)sqlite3_column_text(stmt, 2)); sb_append(sb, ",");
    sb_append(sb, "\"avatar_url\":"); sb_append_json_str(sb, (const char *)sqlite3_column_text(stmt, 3)); sb_append(sb, ",");
    sb_append(sb, "\"profession\":"); sb_append_json_str(sb, (const char *)sqlite3_column_text(stmt, 4)); sb_append(sb, ",");
    sb_append(sb, "\"bio\":"); sb_append_json_str(sb, (const char *)sqlite3_column_text(stmt, 5)); sb_append(sb, ",");
    sb_append(sb, "\"skills\":"); sb_append_json_str(sb, (const char *)sqlite3_column_text(stmt, 6)); sb_append(sb, ",");
    sb_appendf(sb, "\"starting_price\":%.2f,", sqlite3_column_double(stmt, 7));
    sb_append(sb, "\"location\":"); sb_append_json_str(sb, (const char *)sqlite3_column_text(stmt, 8)); sb_append(sb, ",");
    sb_appendf(sb, "\"rating_avg\":%.2f,", sqlite3_column_double(stmt, 9));
    sb_appendf(sb, "\"review_count\":%d,", sqlite3_column_int(stmt, 10));
    sb_appendf(sb, "\"completed_count\":%d,", sqlite3_column_int(stmt, 11));
    sb_appendf(sb, "\"is_featured\":%d,", sqlite3_column_int(stmt, 12));
    const char *cover = (const char *)sqlite3_column_text(stmt, 13);
    sb_append(sb, "\"cover_url\":"); sb_append_json_str(sb, cover ? cover : "");
    sb_append(sb, "}");
}

int freelancer_handle_list(int featured_only, StrBuf *res_json, int *status_code) {
    sqlite3 *db = db_get_handle();
    sqlite3_stmt *stmt = NULL;
    const char *sql = featured_only
        ? "SELECT fp.id, fp.user_id, u.full_name, u.avatar_url, fp.profession, fp.bio, fp.skills, "
          "fp.starting_price, fp.location, fp.rating_avg, fp.review_count, fp.completed_count, fp.is_featured, "
          "COALESCE((SELECT pi.thumbnail_path FROM portfolio_items pi WHERE pi.freelancer_id = fp.id ORDER BY pi.id DESC LIMIT 1), u.avatar_url) "
          "FROM freelancer_profiles fp JOIN users u ON fp.user_id = u.id "
          "WHERE fp.is_featured = 1 ORDER BY fp.rating_avg DESC, fp.completed_count DESC;"
        : "SELECT fp.id, fp.user_id, u.full_name, u.avatar_url, fp.profession, fp.bio, fp.skills, "
          "fp.starting_price, fp.location, fp.rating_avg, fp.review_count, fp.completed_count, fp.is_featured, "
          "COALESCE((SELECT pi.thumbnail_path FROM portfolio_items pi WHERE pi.freelancer_id = fp.id ORDER BY pi.id DESC LIMIT 1), u.avatar_url) "
          "FROM freelancer_profiles fp JOIN users u ON fp.user_id = u.id "
          "ORDER BY fp.is_featured DESC, fp.rating_avg DESC;";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        *status_code = 500;
        sb_append(res_json, "{\"error\":\"Failed to fetch freelancers.\"}");
        return 0;
    }

    sb_append(res_json, "{\"freelancers\":[");
    int first = 1;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        if (!first) sb_append(res_json, ",");
        first = 0;
        append_freelancer_row_json(res_json, stmt);
    }
    sqlite3_finalize(stmt);
    sb_append(res_json, "]}");
    *status_code = 200;
    return 1;
}

int freelancer_handle_get_detail(int freelancer_id, StrBuf *res_json, int *status_code) {
    sqlite3 *db = db_get_handle();
    sqlite3_stmt *stmt = NULL;
    const char *sql =
        "SELECT fp.id, fp.user_id, u.full_name, u.avatar_url, fp.profession, fp.bio, fp.skills, "
        "fp.starting_price, fp.location, fp.rating_avg, fp.review_count, fp.completed_count, fp.is_featured, "
        "COALESCE((SELECT pi.thumbnail_path FROM portfolio_items pi WHERE pi.freelancer_id = fp.id ORDER BY pi.id DESC LIMIT 1), u.avatar_url) "
        "FROM freelancer_profiles fp JOIN users u ON fp.user_id = u.id WHERE fp.id = ?;";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        *status_code = 500;
        sb_append(res_json, "{\"error\":\"Database error.\"}");
        return 0;
    }
    sqlite3_bind_int(stmt, 1, freelancer_id);
    if (sqlite3_step(stmt) != SQLITE_ROW) {
        sqlite3_finalize(stmt);
        *status_code = 404;
        sb_append(res_json, "{\"error\":\"Freelancer profile not found.\"}");
        return 0;
    }

    sb_append(res_json, "{\"freelancer\":");
    append_freelancer_row_json(res_json, stmt);
    sqlite3_finalize(stmt);

    /* Portfolio items */
    sb_append(res_json, ",\"portfolio\":[");
    sqlite3_stmt *pstmt = NULL;
    const char *psql =
        "SELECT id, freelancer_id, title, description, category, media_type, file_path, thumbnail_path, "
        "original_filename, file_size_bytes, mime_type, storage_provider, r2_object_key, mux_asset_id, mux_playback_id, is_trending, created_at "
        "FROM portfolio_items WHERE freelancer_id = ? ORDER BY id DESC;";
    if (sqlite3_prepare_v2(db, psql, -1, &pstmt, NULL) == SQLITE_OK) {
        sqlite3_bind_int(pstmt, 1, freelancer_id);
        int first = 1;
        while (sqlite3_step(pstmt) == SQLITE_ROW) {
            if (!first) sb_append(res_json, ",");
            first = 0;
            sb_append(res_json, "{");
            sb_appendf(res_json, "\"id\":%d,", sqlite3_column_int(pstmt, 0));
            sb_appendf(res_json, "\"freelancer_id\":%d,", sqlite3_column_int(pstmt, 1));
            sb_append(res_json, "\"title\":"); sb_append_json_str(res_json, (const char *)sqlite3_column_text(pstmt, 2)); sb_append(res_json, ",");
            sb_append(res_json, "\"description\":"); sb_append_json_str(res_json, (const char *)sqlite3_column_text(pstmt, 3)); sb_append(res_json, ",");
            sb_append(res_json, "\"category\":"); sb_append_json_str(res_json, (const char *)sqlite3_column_text(pstmt, 4)); sb_append(res_json, ",");
            sb_append(res_json, "\"media_type\":"); sb_append_json_str(res_json, (const char *)sqlite3_column_text(pstmt, 5)); sb_append(res_json, ",");
            sb_append(res_json, "\"file_path\":"); sb_append_json_str(res_json, (const char *)sqlite3_column_text(pstmt, 6)); sb_append(res_json, ",");
            sb_append(res_json, "\"thumbnail_path\":"); sb_append_json_str(res_json, (const char *)sqlite3_column_text(pstmt, 7)); sb_append(res_json, ",");
            sb_append(res_json, "\"original_filename\":"); sb_append_json_str(res_json, (const char *)sqlite3_column_text(pstmt, 8)); sb_append(res_json, ",");
            sb_appendf(res_json, "\"file_size_bytes\":%lld,", sqlite3_column_int64(pstmt, 9));
            sb_append(res_json, "\"mime_type\":"); sb_append_json_str(res_json, (const char *)sqlite3_column_text(pstmt, 10)); sb_append(res_json, ",");
            sb_append(res_json, "\"storage_provider\":"); sb_append_json_str(res_json, (const char *)sqlite3_column_text(pstmt, 11)); sb_append(res_json, ",");
            sb_append(res_json, "\"r2_object_key\":"); sb_append_json_str(res_json, (const char *)sqlite3_column_text(pstmt, 12)); sb_append(res_json, ",");
            sb_append(res_json, "\"mux_asset_id\":"); sb_append_json_str(res_json, (const char *)sqlite3_column_text(pstmt, 13)); sb_append(res_json, ",");
            sb_append(res_json, "\"mux_playback_id\":"); sb_append_json_str(res_json, (const char *)sqlite3_column_text(pstmt, 14)); sb_append(res_json, ",");
            sb_appendf(res_json, "\"is_trending\":%d,", sqlite3_column_int(pstmt, 15));
            sb_append(res_json, "\"created_at\":"); sb_append_json_str(res_json, (const char *)sqlite3_column_text(pstmt, 16));
            sb_append(res_json, "}");
        }
        sqlite3_finalize(pstmt);
    }
    sb_append(res_json, "]");

    /* Reviews */
    sb_append(res_json, ",\"reviews\":[");
    sqlite3_stmt *rstmt = NULL;
    const char *rsql =
        "SELECT r.id, r.project_id, r.rating, r.comment, r.created_at, u.full_name, u.avatar_url, p.title "
        "FROM reviews r "
        "JOIN users u ON r.client_id = u.id "
        "JOIN projects p ON r.project_id = p.id "
        "WHERE r.freelancer_id = ? ORDER BY r.id DESC;";
    if (sqlite3_prepare_v2(db, rsql, -1, &rstmt, NULL) == SQLITE_OK) {
        sqlite3_bind_int(rstmt, 1, freelancer_id);
        int first = 1;
        while (sqlite3_step(rstmt) == SQLITE_ROW) {
            if (!first) sb_append(res_json, ",");
            first = 0;
            sb_append(res_json, "{");
            sb_appendf(res_json, "\"id\":%d,", sqlite3_column_int(rstmt, 0));
            sb_appendf(res_json, "\"project_id\":%d,", sqlite3_column_int(rstmt, 1));
            sb_appendf(res_json, "\"rating\":%d,", sqlite3_column_int(rstmt, 2));
            sb_append(res_json, "\"comment\":"); sb_append_json_str(res_json, (const char *)sqlite3_column_text(rstmt, 3)); sb_append(res_json, ",");
            sb_append(res_json, "\"created_at\":"); sb_append_json_str(res_json, (const char *)sqlite3_column_text(rstmt, 4)); sb_append(res_json, ",");
            sb_append(res_json, "\"client_name\":"); sb_append_json_str(res_json, (const char *)sqlite3_column_text(rstmt, 5)); sb_append(res_json, ",");
            sb_append(res_json, "\"client_avatar\":"); sb_append_json_str(res_json, (const char *)sqlite3_column_text(rstmt, 6)); sb_append(res_json, ",");
            sb_append(res_json, "\"project_title\":"); sb_append_json_str(res_json, (const char *)sqlite3_column_text(rstmt, 7));
            sb_append(res_json, "}");
        }
        sqlite3_finalize(rstmt);
    }
    sb_append(res_json, "]}");
    *status_code = 200;
    return 1;
}

int freelancer_handle_upsert_profile(const AuthUser *user, const char *body, StrBuf *res_json, int *status_code) {
    if (strcmp(user->account_type, "freelancer") != 0) {
        *status_code = 403;
        sb_append(res_json, "{\"error\":\"Only freelancer accounts can edit a freelancer profile.\"}");
        return 0;
    }

    char profession[128], bio[1024], skills[512], location[128];
    double starting_price = 0.0;
    if (!json_get_string(body, "profession", profession, sizeof(profession)) || !profession[0]) {
        snprintf(profession, sizeof(profession), "Video Editor");
    }
    json_get_string(body, "bio", bio, sizeof(bio));
    json_get_string(body, "skills", skills, sizeof(skills));
    if (!json_get_string(body, "location", location, sizeof(location)) || !location[0]) {
        snprintf(location, sizeof(location), "Remote");
    }
    if (!json_get_double(body, "starting_price", &starting_price) || starting_price < 100.0) {
        starting_price = 1500.0;
    }

    sqlite3 *db = db_get_handle();
    sqlite3_stmt *stmt = NULL;
    const char *sql =
        "INSERT INTO freelancer_profiles (user_id, profession, bio, skills, starting_price, location, updated_at) "
        "VALUES (?, ?, ?, ?, ?, ?, CURRENT_TIMESTAMP) "
        "ON CONFLICT(user_id) DO UPDATE SET "
        "profession = excluded.profession, bio = excluded.bio, skills = excluded.skills, "
        "starting_price = excluded.starting_price, location = excluded.location, updated_at = CURRENT_TIMESTAMP;";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        *status_code = 500;
        sb_append(res_json, "{\"error\":\"Failed to update freelancer profile.\"}");
        return 0;
    }
    sqlite3_bind_int(stmt, 1, user->id);
    sqlite3_bind_text(stmt, 2, profession, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, bio, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, skills, -1, SQLITE_STATIC);
    sqlite3_bind_double(stmt, 5, starting_price);
    sqlite3_bind_text(stmt, 6, location, -1, SQLITE_STATIC);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    /* Fetch profile id */
    int fp_id = user->freelancer_profile_id;
    if (fp_id == 0) {
        sqlite3_stmt *q = NULL;
        if (sqlite3_prepare_v2(db, "SELECT id FROM freelancer_profiles WHERE user_id = ?;", -1, &q, NULL) == SQLITE_OK) {
            sqlite3_bind_int(q, 1, user->id);
            if (sqlite3_step(q) == SQLITE_ROW) fp_id = sqlite3_column_int(q, 0);
            sqlite3_finalize(q);
        }
    }

    return freelancer_handle_get_detail(fp_id, res_json, status_code);
}

int freelancer_handle_trending_portfolios(StrBuf *res_json, int *status_code) {
    sqlite3 *db = db_get_handle();
    sqlite3_stmt *stmt = NULL;
    const char *sql =
        "SELECT pi.id, pi.freelancer_id, pi.title, pi.description, pi.category, pi.media_type, "
        "pi.file_path, pi.thumbnail_path, pi.storage_provider, pi.is_trending, pi.created_at, "
        "u.full_name, u.avatar_url, fp.profession, fp.starting_price "
        "FROM portfolio_items pi "
        "JOIN freelancer_profiles fp ON pi.freelancer_id = fp.id "
        "JOIN users u ON fp.user_id = u.id "
        "ORDER BY pi.is_trending DESC, pi.id DESC LIMIT 12;";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        *status_code = 500;
        sb_append(res_json, "{\"error\":\"Failed to fetch trending portfolios.\"}");
        return 0;
    }

    sb_append(res_json, "{\"portfolios\":[");
    int first = 1;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        if (!first) sb_append(res_json, ",");
        first = 0;
        sb_append(res_json, "{");
        sb_appendf(res_json, "\"id\":%d,", sqlite3_column_int(stmt, 0));
        sb_appendf(res_json, "\"freelancer_id\":%d,", sqlite3_column_int(stmt, 1));
        sb_append(res_json, "\"title\":"); sb_append_json_str(res_json, (const char *)sqlite3_column_text(stmt, 2)); sb_append(res_json, ",");
        sb_append(res_json, "\"description\":"); sb_append_json_str(res_json, (const char *)sqlite3_column_text(stmt, 3)); sb_append(res_json, ",");
        sb_append(res_json, "\"category\":"); sb_append_json_str(res_json, (const char *)sqlite3_column_text(stmt, 4)); sb_append(res_json, ",");
        sb_append(res_json, "\"media_type\":"); sb_append_json_str(res_json, (const char *)sqlite3_column_text(stmt, 5)); sb_append(res_json, ",");
        sb_append(res_json, "\"file_path\":"); sb_append_json_str(res_json, (const char *)sqlite3_column_text(stmt, 6)); sb_append(res_json, ",");
        sb_append(res_json, "\"thumbnail_path\":"); sb_append_json_str(res_json, (const char *)sqlite3_column_text(stmt, 7)); sb_append(res_json, ",");
        sb_append(res_json, "\"storage_provider\":"); sb_append_json_str(res_json, (const char *)sqlite3_column_text(stmt, 8)); sb_append(res_json, ",");
        sb_appendf(res_json, "\"is_trending\":%d,", sqlite3_column_int(stmt, 9));
        sb_append(res_json, "\"created_at\":"); sb_append_json_str(res_json, (const char *)sqlite3_column_text(stmt, 10)); sb_append(res_json, ",");
        sb_append(res_json, "\"creator_name\":"); sb_append_json_str(res_json, (const char *)sqlite3_column_text(stmt, 11)); sb_append(res_json, ",");
        sb_append(res_json, "\"creator_avatar\":"); sb_append_json_str(res_json, (const char *)sqlite3_column_text(stmt, 12)); sb_append(res_json, ",");
        sb_append(res_json, "\"profession\":"); sb_append_json_str(res_json, (const char *)sqlite3_column_text(stmt, 13)); sb_append(res_json, ",");
        sb_appendf(res_json, "\"starting_price\":%.2f", sqlite3_column_double(stmt, 14));
        sb_append(res_json, "}");
    }
    sqlite3_finalize(stmt);
    sb_append(res_json, "]}");
    *status_code = 200;
    return 1;
}

int freelancer_handle_add_portfolio(const AuthUser *user, const char *body, StrBuf *res_json, int *status_code) {
    if (strcmp(user->account_type, "freelancer") != 0 || user->freelancer_profile_id <= 0) {
        *status_code = 403;
        sb_append(res_json, "{\"error\":\"Only freelancers can add portfolio items.\"}");
        return 0;
    }

    char title[256], description[1024], category[128], media_type[32], file_path[512], thumbnail_path[512];
    char original_filename[256], mime_type[128], r2_key[256], mux_asset[128], mux_playback[128];
    int file_size = 0;

    if (!json_get_string(body, "title", title, sizeof(title)) ||
        !json_get_string(body, "file_path", file_path, sizeof(file_path)) ||
        !json_get_string(body, "media_type", media_type, sizeof(media_type))) {
        *status_code = 400;
        sb_append(res_json, "{\"error\":\"Title, media_type, and file_path are required.\"}");
        return 0;
    }
    if (strcmp(media_type, "image") != 0 && strcmp(media_type, "video") != 0) {
        *status_code = 400;
        sb_append(res_json, "{\"error\":\"media_type must be image or video.\"}");
        return 0;
    }

    json_get_string(body, "description", description, sizeof(description));
    if (!json_get_string(body, "category", category, sizeof(category)) || !category[0]) {
        snprintf(category, sizeof(category), "Video Editors");
    }
    if (!json_get_string(body, "thumbnail_path", thumbnail_path, sizeof(thumbnail_path)) || !thumbnail_path[0]) {
        snprintf(thumbnail_path, sizeof(thumbnail_path), "%s", strcmp(media_type, "image") == 0 ? file_path : "/assets/generated/portfolio_cinema_edit.jpg");
    }
    json_get_string(body, "original_filename", original_filename, sizeof(original_filename));
    json_get_string(body, "mime_type", mime_type, sizeof(mime_type));
    json_get_int(body, "file_size_bytes", &file_size);
    json_get_string(body, "r2_object_key", r2_key, sizeof(r2_key));
    json_get_string(body, "mux_asset_id", mux_asset, sizeof(mux_asset));
    json_get_string(body, "mux_playback_id", mux_playback, sizeof(mux_playback));

    sqlite3 *db = db_get_handle();
    sqlite3_stmt *stmt = NULL;
    const char *sql =
        "INSERT INTO portfolio_items "
        "(freelancer_id, title, description, category, media_type, file_path, thumbnail_path, original_filename, "
        "file_size_bytes, mime_type, storage_provider, r2_object_key, mux_asset_id, mux_playback_id, is_trending) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, 'local_original', ?, ?, ?, 1);";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        *status_code = 500;
        sb_append(res_json, "{\"error\":\"Database error saving portfolio item.\"}");
        return 0;
    }
    sqlite3_bind_int(stmt, 1, user->freelancer_profile_id);
    sqlite3_bind_text(stmt, 2, title, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, description, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, category, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 5, media_type, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 6, file_path, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 7, thumbnail_path, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 8, original_filename, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 9, file_size);
    sqlite3_bind_text(stmt, 10, mime_type, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 11, r2_key, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 12, mux_asset, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 13, mux_playback, -1, SQLITE_STATIC);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (rc != SQLITE_DONE) {
        *status_code = 500;
        sb_append(res_json, "{\"error\":\"Failed to insert portfolio item.\"}");
        return 0;
    }

    int new_id = (int)sqlite3_last_insert_rowid(db);
    *status_code = 201;
    sb_appendf(res_json, "{\"id\":%d,\"message\":\"Portfolio item added with original file preserved.\"}", new_id);
    return 1;
}

int freelancer_handle_update_portfolio(const AuthUser *user, int item_id, const char *body, StrBuf *res_json, int *status_code) {
    if (user->freelancer_profile_id <= 0) {
        *status_code = 403;
        sb_append(res_json, "{\"error\":\"Unauthorized.\"}");
        return 0;
    }

    char title[256], description[1024], category[128];
    if (!json_get_string(body, "title", title, sizeof(title)) || !title[0]) {
        *status_code = 400;
        sb_append(res_json, "{\"error\":\"Title is required.\"}");
        return 0;
    }
    json_get_string(body, "description", description, sizeof(description));
    json_get_string(body, "category", category, sizeof(category));

    sqlite3 *db = db_get_handle();
    sqlite3_stmt *stmt = NULL;
    const char *sql = "UPDATE portfolio_items SET title = ?, description = ?, category = COALESCE(NULLIF(?, ''), category) WHERE id = ? AND freelancer_id = ?;";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        *status_code = 500;
        sb_append(res_json, "{\"error\":\"Database error.\"}");
        return 0;
    }
    sqlite3_bind_text(stmt, 1, title, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, description, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, category, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 4, item_id);
    sqlite3_bind_int(stmt, 5, user->freelancer_profile_id);
    sqlite3_step(stmt);
    int changes = sqlite3_changes(db);
    sqlite3_finalize(stmt);

    if (changes == 0) {
        *status_code = 404;
        sb_append(res_json, "{\"error\":\"Portfolio item not found or unauthorized.\"}");
        return 0;
    }
    *status_code = 200;
    sb_append(res_json, "{\"ok\":true}");
    return 1;
}

int freelancer_handle_delete_portfolio(const AuthUser *user, int item_id, StrBuf *res_json, int *status_code) {
    if (user->freelancer_profile_id <= 0) {
        *status_code = 403;
        sb_append(res_json, "{\"error\":\"Unauthorized.\"}");
        return 0;
    }
    sqlite3 *db = db_get_handle();
    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(db, "DELETE FROM portfolio_items WHERE id = ? AND freelancer_id = ?;", -1, &stmt, NULL) != SQLITE_OK) {
        *status_code = 500;
        sb_append(res_json, "{\"error\":\"Database error.\"}");
        return 0;
    }
    sqlite3_bind_int(stmt, 1, item_id);
    sqlite3_bind_int(stmt, 2, user->freelancer_profile_id);
    sqlite3_step(stmt);
    int changes = sqlite3_changes(db);
    sqlite3_finalize(stmt);

    if (changes == 0) {
        *status_code = 404;
        sb_append(res_json, "{\"error\":\"Portfolio item not found or unauthorized.\"}");
        return 0;
    }
    *status_code = 200;
    sb_append(res_json, "{\"deleted\":true}");
    return 1;
}
