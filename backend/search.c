#include "search.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>

static void url_decode(const char *src, char *dst, size_t max_len) {
    size_t i = 0;
    while (*src && i + 1 < max_len) {
        if (*src == '%' && isxdigit((unsigned char)src[1]) && isxdigit((unsigned char)src[2])) {
            char hex[3] = {src[1], src[2], '\0'};
            dst[i++] = (char)strtol(hex, NULL, 16);
            src += 3;
        } else if (*src == '+') {
            dst[i++] = ' ';
            src++;
        } else {
            dst[i++] = *src++;
        }
    }
    dst[i] = '\0';
}

static int get_query_param(const char *qs, const char *key, char *out, size_t max_len) {
    if (!qs || !key || !out || max_len == 0) return 0;
    out[0] = '\0';
    size_t klen = strlen(key);
    const char *p = qs;
    while (*p) {
        const char *eq = strchr(p, '=');
        const char *amp = strchr(p, '&');
        if (!eq) break;
        if ((size_t)(eq - p) == klen && strncmp(p, key, klen) == 0) {
            size_t vlen = amp ? (size_t)(amp - (eq + 1)) : strlen(eq + 1);
            char raw[512];
            if (vlen >= sizeof(raw)) vlen = sizeof(raw) - 1;
            memcpy(raw, eq + 1, vlen);
            raw[vlen] = '\0';
            url_decode(raw, out, max_len);
            return out[0] != '\0';
        }
        if (!amp) break;
        p = amp + 1;
    }
    return 0;
}

int search_handle_freelancers(const char *query_string, StrBuf *res_json, int *status_code) {
    char q[256] = "", category[128] = "", min_p_str[32] = "", max_p_str[32] = "", min_r_str[32] = "", sort[32] = "";
    get_query_param(query_string, "q", q, sizeof(q));
    get_query_param(query_string, "category", category, sizeof(category));
    get_query_param(query_string, "min_price", min_p_str, sizeof(min_p_str));
    get_query_param(query_string, "max_price", max_p_str, sizeof(max_p_str));
    get_query_param(query_string, "min_rating", min_r_str, sizeof(min_r_str));
    get_query_param(query_string, "sort", sort, sizeof(sort));

    double min_price = min_p_str[0] ? atof(min_p_str) : 0.0;
    double max_price = max_p_str[0] ? atof(max_p_str) : 10000000.0;
    double min_rating = min_r_str[0] ? atof(min_r_str) : 0.0;

    char like_q[260];
    snprintf(like_q, sizeof(like_q), "%%%s%%", q);

    /* Map plural category filter to profession keywords cleanly */
    char cat_match[132] = "";
    if (category[0] && strcasecmp(category, "All") != 0) {
        size_t clen = strlen(category);
        if (clen > 1 && category[clen - 1] == 's') {
            category[clen - 1] = '\0';
        }
        snprintf(cat_match, sizeof(cat_match), "%%%s%%", category);
    }

    const char *order_clause = "ORDER BY fp.rating_avg DESC, fp.completed_count DESC";
    if (strcmp(sort, "price_asc") == 0) order_clause = "ORDER BY fp.starting_price ASC";
    else if (strcmp(sort, "price_desc") == 0) order_clause = "ORDER BY fp.starting_price DESC";
    else if (strcmp(sort, "completed") == 0) order_clause = "ORDER BY fp.completed_count DESC";

    char sql[2048];
    snprintf(sql, sizeof(sql),
        "SELECT fp.id, fp.user_id, u.full_name, u.avatar_url, fp.profession, fp.bio, fp.skills, "
        "fp.starting_price, fp.location, fp.rating_avg, fp.review_count, fp.completed_count, fp.is_featured, "
        "COALESCE((SELECT pi.thumbnail_path FROM portfolio_items pi WHERE pi.freelancer_id = fp.id ORDER BY pi.id DESC LIMIT 1), u.avatar_url) "
        "FROM freelancer_profiles fp "
        "JOIN users u ON fp.user_id = u.id "
        "WHERE ( ? = '%%%%' OR lower(u.full_name) LIKE lower(?) OR lower(fp.profession) LIKE lower(?) OR lower(fp.skills) LIKE lower(?) OR lower(fp.bio) LIKE lower(?) ) "
        "AND ( ? = '' OR lower(fp.profession) LIKE lower(?) ) "
        "AND fp.starting_price >= ? AND fp.starting_price <= ? AND fp.rating_avg >= ? "
        "%s;",
        order_clause);

    sqlite3 *db = db_get_handle();
    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        *status_code = 500;
        sb_append(res_json, "{\"error\":\"Search query error.\"}");
        return 0;
    }

    sqlite3_bind_text(stmt, 1, like_q, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, like_q, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, like_q, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, like_q, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 5, like_q, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 6, cat_match, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 7, cat_match, -1, SQLITE_STATIC);
    sqlite3_bind_double(stmt, 8, min_price);
    sqlite3_bind_double(stmt, 9, max_price);
    sqlite3_bind_double(stmt, 10, min_rating);

    sb_append(res_json, "{\"freelancers\":[");
    int first = 1;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        if (!first) sb_append(res_json, ",");
        first = 0;
        sb_append(res_json, "{");
        sb_appendf(res_json, "\"id\":%d,", sqlite3_column_int(stmt, 0));
        sb_appendf(res_json, "\"user_id\":%d,", sqlite3_column_int(stmt, 1));
        sb_append(res_json, "\"full_name\":"); sb_append_json_str(res_json, (const char *)sqlite3_column_text(stmt, 2)); sb_append(res_json, ",");
        sb_append(res_json, "\"avatar_url\":"); sb_append_json_str(res_json, (const char *)sqlite3_column_text(stmt, 3)); sb_append(res_json, ",");
        sb_append(res_json, "\"profession\":"); sb_append_json_str(res_json, (const char *)sqlite3_column_text(stmt, 4)); sb_append(res_json, ",");
        sb_append(res_json, "\"bio\":"); sb_append_json_str(res_json, (const char *)sqlite3_column_text(stmt, 5)); sb_append(res_json, ",");
        sb_append(res_json, "\"skills\":"); sb_append_json_str(res_json, (const char *)sqlite3_column_text(stmt, 6)); sb_append(res_json, ",");
        sb_appendf(res_json, "\"starting_price\":%.2f,", sqlite3_column_double(stmt, 7));
        sb_append(res_json, "\"location\":"); sb_append_json_str(res_json, (const char *)sqlite3_column_text(stmt, 8)); sb_append(res_json, ",");
        sb_appendf(res_json, "\"rating_avg\":%.2f,", sqlite3_column_double(stmt, 9));
        sb_appendf(res_json, "\"review_count\":%d,", sqlite3_column_int(stmt, 10));
        sb_appendf(res_json, "\"completed_count\":%d,", sqlite3_column_int(stmt, 11));
        sb_appendf(res_json, "\"is_featured\":%d,", sqlite3_column_int(stmt, 12));
        sb_append(res_json, "\"cover_url\":"); sb_append_json_str(res_json, (const char *)sqlite3_column_text(stmt, 13));
        sb_append(res_json, "}");
    }
    sqlite3_finalize(stmt);
    sb_append(res_json, "]}");
    *status_code = 200;
    return 1;
}
