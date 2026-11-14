#include "authentication.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>

/* Pure C SHA-256 implementation for secure salted password hashing */
typedef struct {
    uint32_t state[8];
    uint64_t bitlen;
    uint8_t data[64];
    size_t datalen;
} SHA256_CTX;

static const uint32_t K256[64] = {
    0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
    0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
    0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
    0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
    0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
    0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
    0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
    0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2
};

#define ROTRIGHT(a,b) (((a) >> (b)) | ((a) << (32-(b))))
#define CH(x,y,z) (((x) & (y)) ^ (~(x) & (z)))
#define MAJ(x,y,z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
#define EP0(x) (ROTRIGHT(x,2) ^ ROTRIGHT(x,13) ^ ROTRIGHT(x,22))
#define EP1(x) (ROTRIGHT(x,6) ^ ROTRIGHT(x,11) ^ ROTRIGHT(x,25))
#define SIG0(x) (ROTRIGHT(x,7) ^ ROTRIGHT(x,18) ^ ((x) >> 3))
#define SIG1(x) (ROTRIGHT(x,17) ^ ROTRIGHT(x,19) ^ ((x) >> 10))

static void sha256_transform(SHA256_CTX *ctx, const uint8_t data[]) {
    uint32_t a, b, c, d, e, f, g, h, i, j, t1, t2, m[64];
    for (i = 0, j = 0; i < 16; ++i, j += 4)
        m[i] = ((uint32_t)data[j] << 24) | ((uint32_t)data[j + 1] << 16) |
               ((uint32_t)data[j + 2] << 8) | ((uint32_t)data[j + 3]);
    for (; i < 64; ++i)
        m[i] = SIG1(m[i - 2]) + m[i - 7] + SIG0(m[i - 15]) + m[i - 16];

    a = ctx->state[0]; b = ctx->state[1]; c = ctx->state[2]; d = ctx->state[3];
    e = ctx->state[4]; f = ctx->state[5]; g = ctx->state[6]; h = ctx->state[7];

    for (i = 0; i < 64; ++i) {
        t1 = h + EP1(e) + CH(e, f, g) + K256[i] + m[i];
        t2 = EP0(a) + MAJ(a, b, c);
        h = g; g = f; f = e; e = d + t1;
        d = c; c = b; b = a; a = t1 + t2;
    }

    ctx->state[0] += a; ctx->state[1] += b; ctx->state[2] += c; ctx->state[3] += d;
    ctx->state[4] += e; ctx->state[5] += f; ctx->state[6] += g; ctx->state[7] += h;
}

static void sha256_init(SHA256_CTX *ctx) {
    ctx->datalen = 0;
    ctx->bitlen = 0;
    ctx->state[0] = 0x6a09e667; ctx->state[1] = 0xbb67ae85;
    ctx->state[2] = 0x3c6ef372; ctx->state[3] = 0xa54ff53a;
    ctx->state[4] = 0x510e527f; ctx->state[5] = 0x9b05688c;
    ctx->state[6] = 0x1f83d9ab; ctx->state[7] = 0x5be0cd19;
}

static void sha256_update(SHA256_CTX *ctx, const uint8_t data[], size_t len) {
    for (size_t i = 0; i < len; ++i) {
        ctx->data[ctx->datalen] = data[i];
        ctx->datalen++;
        if (ctx->datalen == 64) {
            sha256_transform(ctx, ctx->data);
            ctx->bitlen += 512;
            ctx->datalen = 0;
        }
    }
}

static void sha256_final(SHA256_CTX *ctx, uint8_t hash[32]) {
    size_t i = ctx->datalen;
    if (ctx->datalen < 56) {
        ctx->data[i++] = 0x80;
        while (i < 56) ctx->data[i++] = 0x00;
    } else {
        ctx->data[i++] = 0x80;
        while (i < 64) ctx->data[i++] = 0x00;
        sha256_transform(ctx, ctx->data);
        memset(ctx->data, 0, 56);
    }
    ctx->bitlen += ctx->datalen * 8;
    ctx->data[63] = ctx->bitlen;
    ctx->data[62] = ctx->bitlen >> 8;
    ctx->data[61] = ctx->bitlen >> 16;
    ctx->data[60] = ctx->bitlen >> 24;
    ctx->data[59] = ctx->bitlen >> 32;
    ctx->data[58] = ctx->bitlen >> 40;
    ctx->data[57] = ctx->bitlen >> 48;
    ctx->data[56] = ctx->bitlen >> 56;
    sha256_transform(ctx, ctx->data);

    for (i = 0; i < 4; ++i) {
        hash[i]      = (ctx->state[0] >> (24 - i * 8)) & 0xff;
        hash[i + 4]  = (ctx->state[1] >> (24 - i * 8)) & 0xff;
        hash[i + 8]  = (ctx->state[2] >> (24 - i * 8)) & 0xff;
        hash[i + 12] = (ctx->state[3] >> (24 - i * 8)) & 0xff;
        hash[i + 16] = (ctx->state[4] >> (24 - i * 8)) & 0xff;
        hash[i + 20] = (ctx->state[5] >> (24 - i * 8)) & 0xff;
        hash[i + 24] = (ctx->state[6] >> (24 - i * 8)) & 0xff;
        hash[i + 28] = (ctx->state[7] >> (24 - i * 8)) & 0xff;
    }
}

void auth_hash_password(const char *password, const char *salt, char out_hex[65]) {
    uint8_t digest[32];
    SHA256_CTX ctx;
    const char *pepper = getenv("CUTCRAFT_SECRET");
    if (!pepper || !pepper[0]) pepper = "cutcraft_default_pepper_2026";

    sha256_init(&ctx);
    sha256_update(&ctx, (const uint8_t *)salt, strlen(salt));
    sha256_update(&ctx, (const uint8_t *)":", 1);
    sha256_update(&ctx, (const uint8_t *)password, strlen(password));
    sha256_update(&ctx, (const uint8_t *)":", 1);
    sha256_update(&ctx, (const uint8_t *)pepper, strlen(pepper));
    sha256_final(&ctx, digest);

    /* Key stretching: 2048 rounds */
    for (int r = 0; r < 2048; r++) {
        sha256_init(&ctx);
        sha256_update(&ctx, digest, 32);
        sha256_update(&ctx, (const uint8_t *)salt, strlen(salt));
        sha256_final(&ctx, digest);
    }

    for (int i = 0; i < 32; i++) {
        sprintf(out_hex + i * 2, "%02x", digest[i]);
    }
    out_hex[64] = '\0';
}

void auth_generate_random_hex(char *out, size_t byte_len) {
    uint8_t *buf = (uint8_t *)malloc(byte_len);
    if (!buf) return;
    int fd = open("/dev/urandom", O_RDONLY);
    if (fd >= 0) {
        ssize_t n = read(fd, buf, byte_len);
        (void)n;
        close(fd);
    } else {
        for (size_t i = 0; i < byte_len; i++) buf[i] = (uint8_t)(rand() & 0xff);
    }
    for (size_t i = 0; i < byte_len; i++) {
        sprintf(out + i * 2, "%02x", buf[i]);
    }
    out[byte_len * 2] = '\0';
    free(buf);
}

int auth_extract_bearer(const char *auth_header, char *out_token, size_t max_len) {
    if (!auth_header || !out_token || max_len == 0) return 0;
    out_token[0] = '\0';
    const char *p = strstr(auth_header, "Bearer ");
    if (!p) p = strstr(auth_header, "bearer ");
    if (!p) return 0;
    p += 7;
    while (*p == ' ') p++;
    size_t i = 0;
    while (*p && !isspace((unsigned char)*p) && i + 1 < max_len) {
        out_token[i++] = *p++;
    }
    out_token[i] = '\0';
    return i > 0;
}

int auth_authenticate_token(const char *token, AuthUser *out_user) {
    if (!token || !token[0] || !out_user) return 0;
    sqlite3 *db = db_get_handle();
    sqlite3_stmt *stmt = NULL;
    const char *sql =
        "SELECT u.id, u.full_name, u.email, u.account_type, u.avatar_url, "
        "COALESCE(fp.id, 0) "
        "FROM sessions s "
        "JOIN users u ON s.user_id = u.id "
        "LEFT JOIN freelancer_profiles fp ON fp.user_id = u.id "
        "WHERE s.token = ? AND s.expires_at > datetime('now');";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) return 0;
    sqlite3_bind_text(stmt, 1, token, -1, SQLITE_STATIC);

    int ok = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        out_user->id = sqlite3_column_int(stmt, 0);
        snprintf(out_user->full_name, sizeof(out_user->full_name), "%s", (const char *)sqlite3_column_text(stmt, 1));
        snprintf(out_user->email, sizeof(out_user->email), "%s", (const char *)sqlite3_column_text(stmt, 2));
        snprintf(out_user->account_type, sizeof(out_user->account_type), "%s", (const char *)sqlite3_column_text(stmt, 3));
        const char *av = (const char *)sqlite3_column_text(stmt, 4);
        snprintf(out_user->avatar_url, sizeof(out_user->avatar_url), "%s", av ? av : "");
        out_user->freelancer_profile_id = sqlite3_column_int(stmt, 5);
        ok = 1;
    }
    sqlite3_finalize(stmt);
    return ok;
}

static void append_user_json(StrBuf *sb, int id, const char *name, const char *email, const char *role, const char *avatar, int fp_id) {
    sb_append(sb, "{");
    sb_appendf(sb, "\"id\":%d,", id);
    sb_append(sb, "\"full_name\":"); sb_append_json_str(sb, name); sb_append(sb, ",");
    sb_append(sb, "\"email\":"); sb_append_json_str(sb, email); sb_append(sb, ",");
    sb_append(sb, "\"account_type\":"); sb_append_json_str(sb, role); sb_append(sb, ",");
    sb_append(sb, "\"avatar_url\":"); sb_append_json_str(sb, avatar); sb_append(sb, ",");
    sb_appendf(sb, "\"freelancer_profile_id\":%d", fp_id);
    sb_append(sb, "}");
}

static int create_session_token(int user_id, char out_token[65]) {
    sqlite3 *db = db_get_handle();
    auth_generate_random_hex(out_token, 32);
    sqlite3_stmt *stmt = NULL;
    const char *sql = "INSERT INTO sessions (token, user_id, expires_at) VALUES (?, ?, datetime('now', '+7 days'));";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) return 0;
    sqlite3_bind_text(stmt, 1, out_token, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, user_id);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE;
}

int auth_handle_register(const char *body, StrBuf *res_json, int *status_code) {
    char full_name[128], email[128], password[128], account_type[32], profession[128];
    if (!json_get_string(body, "full_name", full_name, sizeof(full_name)) ||
        !json_get_string(body, "email", email, sizeof(email)) ||
        !json_get_string(body, "password", password, sizeof(password)) ||
        !json_get_string(body, "account_type", account_type, sizeof(account_type))) {
        *status_code = 400;
        sb_append(res_json, "{\"error\":\"Full name, email, password, and account type are required.\"}");
        return 0;
    }

    if (strlen(full_name) < 2 || !strchr(email, '@') || strlen(password) < 6) {
        *status_code = 400;
        sb_append(res_json, "{\"error\":\"Invalid input. Password must be at least 6 characters and email must be valid.\"}");
        return 0;
    }
    if (strcmp(account_type, "client") != 0 && strcmp(account_type, "freelancer") != 0) {
        *status_code = 400;
        sb_append(res_json, "{\"error\":\"Account type must be client or freelancer.\"}");
        return 0;
    }

    /* Lowercase email */
    for (char *p = email; *p; p++) *p = (char)tolower((unsigned char)*p);

    if (!json_get_string(body, "profession", profession, sizeof(profession)) || !profession[0]) {
        snprintf(profession, sizeof(profession), "Video Editor");
    }

    char salt[33], hash[65];
    auth_generate_random_hex(salt, 16);
    auth_hash_password(password, salt, hash);

    sqlite3 *db = db_get_handle();
    sqlite3_stmt *stmt = NULL;
    const char *sql = "INSERT INTO users (full_name, email, password_hash, salt, account_type, avatar_url) VALUES (?, ?, ?, ?, ?, '/assets/generated/creator_portrait_designer.jpg');";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        *status_code = 500;
        sb_append(res_json, "{\"error\":\"Database error creating account.\"}");
        return 0;
    }
    sqlite3_bind_text(stmt, 1, full_name, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, email, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, hash, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, salt, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 5, account_type, -1, SQLITE_STATIC);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        *status_code = 409;
        sb_append(res_json, "{\"error\":\"An account with this email already exists.\"}");
        return 0;
    }

    int user_id = (int)sqlite3_last_insert_rowid(db);
    int fp_id = 0;

    if (strcmp(account_type, "freelancer") == 0) {
        sqlite3_stmt *fstmt = NULL;
        const char *fsql = "INSERT INTO freelancer_profiles (user_id, profession, bio, skills, starting_price, location) VALUES (?, ?, 'Creative specialist on CUTCRAFT.', ?, 2000, 'Remote');";
        if (sqlite3_prepare_v2(db, fsql, -1, &fstmt, NULL) == SQLITE_OK) {
            sqlite3_bind_int(fstmt, 1, user_id);
            sqlite3_bind_text(fstmt, 2, profession, -1, SQLITE_STATIC);
            sqlite3_bind_text(fstmt, 3, profession, -1, SQLITE_STATIC);
            if (sqlite3_step(fstmt) == SQLITE_DONE) {
                fp_id = (int)sqlite3_last_insert_rowid(db);
            }
            sqlite3_finalize(fstmt);
        }
    }

    char token[65];
    create_session_token(user_id, token);

    *status_code = 201;
    sb_append(res_json, "{\"token\":");
    sb_append_json_str(res_json, token);
    sb_append(res_json, ",\"user\":");
    append_user_json(res_json, user_id, full_name, email, account_type, "/assets/generated/creator_portrait_designer.jpg", fp_id);
    sb_append(res_json, "}");
    return 1;
}

int auth_handle_login(const char *body, StrBuf *res_json, int *status_code) {
    char email[128], password[128];
    if (!json_get_string(body, "email", email, sizeof(email)) ||
        !json_get_string(body, "password", password, sizeof(password))) {
        *status_code = 400;
        sb_append(res_json, "{\"error\":\"Email and password are required.\"}");
        return 0;
    }
    for (char *p = email; *p; p++) *p = (char)tolower((unsigned char)*p);

    sqlite3 *db = db_get_handle();
    sqlite3_stmt *stmt = NULL;
    const char *sql =
        "SELECT u.id, u.full_name, u.email, u.password_hash, u.salt, u.account_type, u.avatar_url, COALESCE(fp.id, 0) "
        "FROM users u LEFT JOIN freelancer_profiles fp ON fp.user_id = u.id "
        "WHERE lower(u.email) = ?;";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        *status_code = 500;
        sb_append(res_json, "{\"error\":\"Database error.\"}");
        return 0;
    }
    sqlite3_bind_text(stmt, 1, email, -1, SQLITE_STATIC);

    if (sqlite3_step(stmt) != SQLITE_ROW) {
        sqlite3_finalize(stmt);
        *status_code = 401;
        sb_append(res_json, "{\"error\":\"Invalid email or password.\"}");
        return 0;
    }

    int user_id = sqlite3_column_int(stmt, 0);
    const char *full_name = (const char *)sqlite3_column_text(stmt, 1);
    const char *db_email = (const char *)sqlite3_column_text(stmt, 2);
    const char *db_hash = (const char *)sqlite3_column_text(stmt, 3);
    const char *db_salt = (const char *)sqlite3_column_text(stmt, 4);
    const char *account_type = (const char *)sqlite3_column_text(stmt, 5);
    const char *avatar_url = (const char *)sqlite3_column_text(stmt, 6);
    int fp_id = sqlite3_column_int(stmt, 7);

    char computed_hash[65];
    auth_hash_password(password, db_salt, computed_hash);

    if (strcmp(computed_hash, db_hash) != 0) {
        sqlite3_finalize(stmt);
        *status_code = 401;
        sb_append(res_json, "{\"error\":\"Invalid email or password.\"}");
        return 0;
    }

    char name_copy[128], email_copy[128], role_copy[32], avatar_copy[256];
    snprintf(name_copy, sizeof(name_copy), "%s", full_name ? full_name : "");
    snprintf(email_copy, sizeof(email_copy), "%s", db_email ? db_email : "");
    snprintf(role_copy, sizeof(role_copy), "%s", account_type ? account_type : "");
    snprintf(avatar_copy, sizeof(avatar_copy), "%s", avatar_url ? avatar_url : "");
    sqlite3_finalize(stmt);

    char token[65];
    create_session_token(user_id, token);

    *status_code = 200;
    sb_append(res_json, "{\"token\":");
    sb_append_json_str(res_json, token);
    sb_append(res_json, ",\"user\":");
    append_user_json(res_json, user_id, name_copy, email_copy, role_copy, avatar_copy, fp_id);
    sb_append(res_json, "}");
    return 1;
}

int auth_handle_logout(const char *token, StrBuf *res_json, int *status_code) {
    if (token && token[0]) {
        sqlite3 *db = db_get_handle();
        sqlite3_stmt *stmt = NULL;
        if (sqlite3_prepare_v2(db, "DELETE FROM sessions WHERE token = ?;", -1, &stmt, NULL) == SQLITE_OK) {
            sqlite3_bind_text(stmt, 1, token, -1, SQLITE_STATIC);
            sqlite3_step(stmt);
            sqlite3_finalize(stmt);
        }
    }
    *status_code = 200;
    sb_append(res_json, "{\"ok\":true}");
    return 1;
}

int auth_handle_me(const AuthUser *user, StrBuf *res_json, int *status_code) {
    *status_code = 200;
    sb_append(res_json, "{\"user\":");
    append_user_json(res_json, user->id, user->full_name, user->email, user->account_type, user->avatar_url, user->freelancer_profile_id);
    sb_append(res_json, "}");
    return 1;
}

int auth_handle_change_password(const AuthUser *user, const char *body, StrBuf *res_json, int *status_code) {
    char current_pw[128], new_pw[128];
    if (!json_get_string(body, "current_password", current_pw, sizeof(current_pw)) ||
        !json_get_string(body, "new_password", new_pw, sizeof(new_pw))) {
        *status_code = 400;
        sb_append(res_json, "{\"error\":\"Current and new passwords are required.\"}");
        return 0;
    }
    if (strlen(new_pw) < 6) {
        *status_code = 400;
        sb_append(res_json, "{\"error\":\"New password must be at least 6 characters.\"}");
        return 0;
    }

    sqlite3 *db = db_get_handle();
    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(db, "SELECT password_hash, salt FROM users WHERE id = ?;", -1, &stmt, NULL) != SQLITE_OK) {
        *status_code = 500;
        sb_append(res_json, "{\"error\":\"Database error.\"}");
        return 0;
    }
    sqlite3_bind_int(stmt, 1, user->id);
    if (sqlite3_step(stmt) != SQLITE_ROW) {
        sqlite3_finalize(stmt);
        *status_code = 404;
        sb_append(res_json, "{\"error\":\"User not found.\"}");
        return 0;
    }

    char check_hash[65];
    const char *db_hash = (const char *)sqlite3_column_text(stmt, 0);
    const char *db_salt = (const char *)sqlite3_column_text(stmt, 1);
    auth_hash_password(current_pw, db_salt, check_hash);
    int matches = (strcmp(check_hash, db_hash) == 0);
    sqlite3_finalize(stmt);

    if (!matches) {
        *status_code = 401;
        sb_append(res_json, "{\"error\":\"Current password is incorrect.\"}");
        return 0;
    }

    char new_salt[33], new_hash[65];
    auth_generate_random_hex(new_salt, 16);
    auth_hash_password(new_pw, new_salt, new_hash);

    sqlite3_stmt *ustmt = NULL;
    if (sqlite3_prepare_v2(db, "UPDATE users SET password_hash = ?, salt = ? WHERE id = ?;", -1, &ustmt, NULL) == SQLITE_OK) {
        sqlite3_bind_text(ustmt, 1, new_hash, -1, SQLITE_STATIC);
        sqlite3_bind_text(ustmt, 2, new_salt, -1, SQLITE_STATIC);
        sqlite3_bind_int(ustmt, 3, user->id);
        sqlite3_step(ustmt);
        sqlite3_finalize(ustmt);
    }

    *status_code = 200;
    sb_append(res_json, "{\"message\":\"Password updated successfully.\"}");
    return 1;
}

int auth_handle_forgot_password(const char *body, StrBuf *res_json, int *status_code) {
    char email[128];
    if (!json_get_string(body, "email", email, sizeof(email)) || !strchr(email, '@')) {
        *status_code = 400;
        sb_append(res_json, "{\"error\":\"Please enter a valid email address.\"}");
        return 0;
    }
    *status_code = 200;
    sb_append(res_json, "{\"message\":\"If an account matches this email, password reset instructions have been queued. Contact your project administrator for immediate reset in local college demo mode.\"}");
    return 1;
}
