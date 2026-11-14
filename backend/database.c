#include "database.h"
#include "authentication.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>

static sqlite3 *g_db = NULL;

void sb_init(StrBuf *sb) {
    sb->cap = 1024;
    sb->len = 0;
    sb->data = (char *)malloc(sb->cap);
    if (sb->data) sb->data[0] = '\0';
}

void sb_free(StrBuf *sb) {
    if (sb->data) {
        free(sb->data);
        sb->data = NULL;
    }
    sb->len = 0;
    sb->cap = 0;
}

static void sb_ensure(StrBuf *sb, size_t extra) {
    if (!sb->data) sb_init(sb);
    if (sb->len + extra + 1 >= sb->cap) {
        while (sb->len + extra + 1 >= sb->cap) {
            sb->cap *= 2;
        }
        char *next = (char *)realloc(sb->data, sb->cap);
        if (next) sb->data = next;
    }
}

void sb_append(StrBuf *sb, const char *str) {
    if (!str) return;
    size_t slen = strlen(str);
    sb_ensure(sb, slen);
    memcpy(sb->data + sb->len, str, slen);
    sb->len += slen;
    sb->data[sb->len] = '\0';
}

void sb_appendf(StrBuf *sb, const char *fmt, ...) {
    va_list args, copy;
    va_start(args, fmt);
    va_copy(copy, args);
    int needed = vsnprintf(NULL, 0, fmt, args);
    va_end(args);
    if (needed > 0) {
        sb_ensure(sb, (size_t)needed);
        vsnprintf(sb->data + sb->len, (size_t)needed + 1, fmt, copy);
        sb->len += (size_t)needed;
    }
    va_end(copy);
}

void sb_append_json_str(StrBuf *sb, const char *str) {
    sb_append(sb, "\"");
    if (str) {
        for (const unsigned char *p = (const unsigned char *)str; *p; p++) {
            switch (*p) {
                case '\"': sb_append(sb, "\\\""); break;
                case '\\': sb_append(sb, "\\\\"); break;
                case '\b': sb_append(sb, "\\b"); break;
                case '\f': sb_append(sb, "\\f"); break;
                case '\n': sb_append(sb, "\\n"); break;
                case '\r': sb_append(sb, "\\r"); break;
                case '\t': sb_append(sb, "\\t"); break;
                default:
                    if (*p < 0x20) {
                        sb_appendf(sb, "\\u%04x", *p);
                    } else {
                        char ch[2] = {(char)*p, '\0'};
                        sb_append(sb, ch);
                    }
                    break;
            }
        }
    }
    sb_append(sb, "\"");
}

static const char *find_json_key(const char *json, const char *key) {
    if (!json || !key) return NULL;
    char pattern[256];
    snprintf(pattern, sizeof(pattern), "\"%s\"", key);
    const char *p = json;
    while ((p = strstr(p, pattern)) != NULL) {
        const char *after = p + strlen(pattern);
        while (*after && isspace((unsigned char)*after)) after++;
        if (*after == ':') {
            after++;
            while (*after && isspace((unsigned char)*after)) after++;
            return after;
        }
        p += strlen(pattern);
    }
    return NULL;
}

int json_get_string(const char *json, const char *key, char *out, size_t out_size) {
    if (!out || out_size == 0) return 0;
    out[0] = '\0';
    const char *val = find_json_key(json, key);
    if (!val || *val != '\"') return 0;
    val++;
    size_t i = 0;
    while (*val && *val != '\"' && i + 1 < out_size) {
        if (*val == '\\' && *(val + 1)) {
            val++;
            switch (*val) {
                case 'n': out[i++] = '\n'; break;
                case 'r': out[i++] = '\r'; break;
                case 't': out[i++] = '\t'; break;
                case '\"': out[i++] = '\"'; break;
                case '\\': out[i++] = '\\'; break;
                case '/': out[i++] = '/'; break;
                default: out[i++] = *val; break;
            }
        } else {
            out[i++] = *val;
        }
        val++;
    }
    out[i] = '\0';
    return 1;
}

int json_get_double(const char *json, const char *key, double *out) {
    const char *val = find_json_key(json, key);
    if (!val) return 0;
    if (*val == '\"') val++;
    char *end = NULL;
    double v = strtod(val, &end);
    if (end == val) return 0;
    *out = v;
    return 1;
}

int json_get_int(const char *json, const char *key, int *out) {
    const char *val = find_json_key(json, key);
    if (!val) return 0;
    if (*val == '\"') val++;
    char *end = NULL;
    long v = strtol(val, &end, 10);
    if (end == val) return 0;
    *out = (int)v;
    return 1;
}

sqlite3 *db_get_handle(void) {
    return g_db;
}

int db_init(const char *db_path) {
    int rc = sqlite3_open(db_path, &g_db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot open SQLite database: %s\n", sqlite3_errmsg(g_db));
        return 0;
    }

    sqlite3_busy_timeout(g_db, 5000);
    char *err = NULL;

    const char *schema_sql =
        "PRAGMA foreign_keys = ON;"
        "PRAGMA journal_mode = WAL;"
        "CREATE TABLE IF NOT EXISTS users ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  full_name TEXT NOT NULL,"
        "  email TEXT NOT NULL UNIQUE,"
        "  password_hash TEXT NOT NULL,"
        "  salt TEXT NOT NULL,"
        "  account_type TEXT NOT NULL CHECK(account_type IN ('client', 'freelancer')),"
        "  avatar_url TEXT DEFAULT '',"
        "  created_at DATETIME DEFAULT CURRENT_TIMESTAMP"
        ");"
        "CREATE TABLE IF NOT EXISTS sessions ("
        "  token TEXT PRIMARY KEY,"
        "  user_id INTEGER NOT NULL,"
        "  expires_at DATETIME NOT NULL,"
        "  created_at DATETIME DEFAULT CURRENT_TIMESTAMP,"
        "  FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE"
        ");"
        "CREATE TABLE IF NOT EXISTS freelancer_profiles ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  user_id INTEGER NOT NULL UNIQUE,"
        "  profession TEXT NOT NULL,"
        "  bio TEXT DEFAULT '',"
        "  skills TEXT DEFAULT '',"
        "  starting_price REAL NOT NULL DEFAULT 1500,"
        "  location TEXT DEFAULT 'Remote',"
        "  rating_avg REAL NOT NULL DEFAULT 5.0,"
        "  review_count INTEGER NOT NULL DEFAULT 0,"
        "  completed_count INTEGER NOT NULL DEFAULT 0,"
        "  is_featured INTEGER NOT NULL DEFAULT 0,"
        "  updated_at DATETIME DEFAULT CURRENT_TIMESTAMP,"
        "  FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE"
        ");"
        "CREATE TABLE IF NOT EXISTS portfolio_items ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  freelancer_id INTEGER NOT NULL,"
        "  title TEXT NOT NULL,"
        "  description TEXT DEFAULT '',"
        "  category TEXT NOT NULL,"
        "  media_type TEXT NOT NULL CHECK(media_type IN ('image', 'video')),"
        "  file_path TEXT NOT NULL,"
        "  thumbnail_path TEXT DEFAULT '',"
        "  original_filename TEXT DEFAULT '',"
        "  file_size_bytes INTEGER DEFAULT 0,"
        "  mime_type TEXT DEFAULT '',"
        "  storage_provider TEXT DEFAULT 'local_original',"
        "  r2_object_key TEXT DEFAULT '',"
        "  mux_asset_id TEXT DEFAULT '',"
        "  mux_playback_id TEXT DEFAULT '',"
        "  is_trending INTEGER NOT NULL DEFAULT 0,"
        "  created_at DATETIME DEFAULT CURRENT_TIMESTAMP,"
        "  FOREIGN KEY (freelancer_id) REFERENCES freelancer_profiles(id) ON DELETE CASCADE"
        ");"
        "CREATE TABLE IF NOT EXISTS project_requests ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  client_id INTEGER NOT NULL,"
        "  freelancer_id INTEGER NOT NULL,"
        "  title TEXT NOT NULL,"
        "  description TEXT NOT NULL,"
        "  budget REAL NOT NULL,"
        "  deadline TEXT NOT NULL,"
        "  attachment_path TEXT DEFAULT '',"
        "  status TEXT NOT NULL DEFAULT 'pending' CHECK(status IN ('pending', 'accepted', 'rejected')),"
        "  created_at DATETIME DEFAULT CURRENT_TIMESTAMP,"
        "  updated_at DATETIME DEFAULT CURRENT_TIMESTAMP,"
        "  FOREIGN KEY (client_id) REFERENCES users(id) ON DELETE CASCADE,"
        "  FOREIGN KEY (freelancer_id) REFERENCES freelancer_profiles(id) ON DELETE CASCADE"
        ");"
        "CREATE TABLE IF NOT EXISTS projects ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  client_id INTEGER NOT NULL,"
        "  freelancer_id INTEGER NOT NULL,"
        "  request_id INTEGER,"
        "  title TEXT NOT NULL,"
        "  description TEXT NOT NULL,"
        "  budget REAL NOT NULL,"
        "  commission_rate REAL NOT NULL DEFAULT 10.0,"
        "  platform_fee REAL NOT NULL,"
        "  freelancer_earnings REAL NOT NULL,"
        "  deadline TEXT NOT NULL,"
        "  status TEXT NOT NULL DEFAULT 'accepted' CHECK(status IN ('accepted', 'in_progress', 'completed', 'cancelled')),"
        "  progress_percent INTEGER NOT NULL DEFAULT 10,"
        "  payment_status TEXT NOT NULL DEFAULT 'unpaid' CHECK(payment_status IN ('unpaid', 'simulated_paid')),"
        "  created_at DATETIME DEFAULT CURRENT_TIMESTAMP,"
        "  updated_at DATETIME DEFAULT CURRENT_TIMESTAMP,"
        "  FOREIGN KEY (client_id) REFERENCES users(id) ON DELETE CASCADE,"
        "  FOREIGN KEY (freelancer_id) REFERENCES freelancer_profiles(id) ON DELETE CASCADE,"
        "  FOREIGN KEY (request_id) REFERENCES project_requests(id) ON DELETE SET NULL"
        ");"
        "CREATE TABLE IF NOT EXISTS messages ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  sender_id INTEGER NOT NULL,"
        "  receiver_id INTEGER NOT NULL,"
        "  project_id INTEGER,"
        "  content TEXT NOT NULL,"
        "  is_read INTEGER NOT NULL DEFAULT 0,"
        "  created_at DATETIME DEFAULT CURRENT_TIMESTAMP,"
        "  FOREIGN KEY (sender_id) REFERENCES users(id) ON DELETE CASCADE,"
        "  FOREIGN KEY (receiver_id) REFERENCES users(id) ON DELETE CASCADE,"
        "  FOREIGN KEY (project_id) REFERENCES projects(id) ON DELETE SET NULL"
        ");"
        "CREATE TABLE IF NOT EXISTS reviews ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  project_id INTEGER NOT NULL UNIQUE,"
        "  client_id INTEGER NOT NULL,"
        "  freelancer_id INTEGER NOT NULL,"
        "  rating INTEGER NOT NULL CHECK(rating BETWEEN 1 AND 5),"
        "  comment TEXT NOT NULL,"
        "  created_at DATETIME DEFAULT CURRENT_TIMESTAMP,"
        "  FOREIGN KEY (project_id) REFERENCES projects(id) ON DELETE CASCADE,"
        "  FOREIGN KEY (client_id) REFERENCES users(id) ON DELETE CASCADE,"
        "  FOREIGN KEY (freelancer_id) REFERENCES freelancer_profiles(id) ON DELETE CASCADE"
        ");"
        "CREATE TABLE IF NOT EXISTS transactions ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  project_id INTEGER NOT NULL,"
        "  client_id INTEGER NOT NULL,"
        "  freelancer_id INTEGER NOT NULL,"
        "  gross_amount REAL NOT NULL,"
        "  commission_rate REAL NOT NULL,"
        "  platform_earnings REAL NOT NULL,"
        "  freelancer_earnings REAL NOT NULL,"
        "  payment_method TEXT NOT NULL DEFAULT 'Simulated Escrow (v1)',"
        "  payment_status TEXT NOT NULL DEFAULT 'simulated_paid',"
        "  is_simulated INTEGER NOT NULL DEFAULT 1,"
        "  reference_code TEXT NOT NULL,"
        "  created_at DATETIME DEFAULT CURRENT_TIMESTAMP,"
        "  FOREIGN KEY (project_id) REFERENCES projects(id) ON DELETE CASCADE,"
        "  FOREIGN KEY (client_id) REFERENCES users(id) ON DELETE CASCADE,"
        "  FOREIGN KEY (freelancer_id) REFERENCES freelancer_profiles(id) ON DELETE CASCADE"
        ");"
        "CREATE TABLE IF NOT EXISTS app_config ("
        "  key TEXT PRIMARY KEY,"
        "  value TEXT NOT NULL"
        ");"
        "INSERT OR IGNORE INTO app_config (key, value) VALUES ('commission_rate', '10.0');";

    rc = sqlite3_exec(g_db, schema_sql, NULL, NULL, &err);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQLite schema error: %s\n", err);
        sqlite3_free(err);
        return 0;
    }
    return 1;
}

void db_close(void) {
    if (g_db) {
        sqlite3_close(g_db);
        g_db = NULL;
    }
}

void db_seed_initial_data(void) {
    sqlite3_stmt *stmt = NULL;
    int user_count = 0;
    if (sqlite3_prepare_v2(g_db, "SELECT COUNT(*) FROM users;", -1, &stmt, NULL) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            user_count = sqlite3_column_int(stmt, 0);
        }
        sqlite3_finalize(stmt);
    }
    if (user_count > 0) return;

    const char *salt1 = "cc_salt_a1b2c3d4";
    char hash1[65];
    auth_hash_password("Cutcraft@2026", salt1, hash1);

    /* Seed realistic initial creators, client, portfolios, project, and chat */

    /* Note: avoid invalid row 6 since user_id is UNIQUE; build clean SQL */
    char *sql = (char *)malloc(16384);
    if (!sql) return;

    snprintf(sql, 16384,
        "INSERT INTO users (id, full_name, email, password_hash, salt, account_type, avatar_url) VALUES "
        "(1, 'Elena Vance', 'elena@cutcraft.studio', '%s', '%s', 'freelancer', '/assets/generated/creator_portrait_studio.jpg'),"
        "(2, 'Marcus Reed', 'marcus@cutcraft.studio', '%s', '%s', 'freelancer', '/assets/generated/creator_portrait_designer.jpg'),"
        "(3, 'Riya Nair', 'riya@cutcraft.studio', '%s', '%s', 'freelancer', '/assets/generated/portfolio_fashion_shoot.jpg'),"
        "(4, 'Kabir Malhotra', 'kabir@cutcraft.studio', '%s', '%s', 'freelancer', '/assets/generated/portfolio_cinema_edit.jpg'),"
        "(5, 'Tara Sen', 'tara@cutcraft.studio', '%s', '%s', 'freelancer', '/assets/generated/portfolio_motion_3d.jpg'),"
        "(6, 'Devraj Kulkarni', 'devraj@cutcraft.studio', '%s', '%s', 'freelancer', '/assets/generated/creator_portrait_designer.jpg'),"
        "(7, 'Aarav Mehta', 'client@cutcraft.studio', '%s', '%s', 'client', '/assets/generated/creator_portrait_studio.jpg');"

        "INSERT INTO freelancer_profiles (id, user_id, profession, bio, skills, starting_price, location, rating_avg, review_count, completed_count, is_featured) VALUES "
        "(1, 1, 'Video Editor', 'Senior narrative and commercial video editor specializing in DaVinci Resolve color grading, pacing, and 4K documentary storytelling.', 'DaVinci Resolve,Premiere Pro,Color Grading,Sound Design,4K Mastering', 3500, 'Mumbai, IN', 4.9, 2, 42, 1),"
        "(2, 2, 'Motion Graphics Designer', 'Broadcast title designer and Cinema 4D motion artist crafting kinetic typography, product reveals, and brand films.', 'After Effects,Cinema 4D,Octane Render,Kinetic Type,3D Compositing', 4200, 'Bengaluru, IN', 4.8, 1, 31, 1),"
        "(3, 3, 'Photographer', 'Editorial fashion and commercial studio photographer working with medium-format digital systems and dramatic architectural lighting.', 'Editorial Photography,Lightroom,Capture One,Studio Lighting,Retouching', 2800, 'Delhi, IN', 5.0, 1, 37, 1),"
        "(4, 4, 'Videographer', 'Director of photography and gimbal operator producing music videos, brand commercials, and travel docuseries.', 'RED Komodo,Sony FX6,Cinematography,Drone FPV,Lighting Direction', 5500, 'Goa, IN', 4.9, 1, 22, 1),"
        "(5, 5, 'Thumbnail Designer', 'High-CTR YouTube thumbnail designer and visual strategist blending photomanipulation, lighting contrast, and typography.', 'Photoshop,Thumbnail Strategy,Color Contrast,Visual Storytelling,Brand Systems', 1200, 'Pune, IN', 4.7, 0, 68, 0),"
        "(6, 6, 'Photo Editor', 'Commercial beauty and automotive retoucher delivering non-destructive frequency separation and filmic color toning.', 'Photoshop,Lightroom,Frequency Separation,Color Toning,RAW Processing', 1800, 'Hyderabad, IN', 4.8, 0, 29, 0);"

        "INSERT INTO portfolio_items (freelancer_id, title, description, category, media_type, file_path, thumbnail_path, original_filename, file_size_bytes, mime_type, storage_provider, is_trending) VALUES "
        "(1, 'Dune Horizon Commercial Cut', 'Full 4K editorial cut and HDR amber color grade for an automotive desert campaign.', 'Video Editors', 'image', '/assets/generated/portfolio_cinema_edit.jpg', '/assets/generated/portfolio_cinema_edit.jpg', 'dune_horizon_grade.jpg', 842100, 'image/jpeg', 'local_original', 1),"
        "(2, 'Architectural Visions Title Sequence', 'Titanium and molten gold 3D ribbon title sequence rendered in Octane at 60fps.', 'Motion Graphics Designers', 'image', '/assets/generated/portfolio_motion_3d.jpg', '/assets/generated/portfolio_motion_3d.jpg', 'architectural_visions_3d.jpg', 791400, 'image/jpeg', 'local_original', 1),"
        "(3, 'Brutalist Concrete Editorial', 'Medium-format fashion lookbook photographed in natural architectural shadow.', 'Photographers', 'image', '/assets/generated/portfolio_fashion_shoot.jpg', '/assets/generated/portfolio_fashion_shoot.jpg', 'brutalist_editorial.jpg', 912040, 'image/jpeg', 'local_original', 1),"
        "(4, 'Golden Hour Studio Documentary', 'Behind-the-lens cinematography reel for independent filmmakers and sound stages.', 'Videographers', 'image', '/assets/generated/portfolio_cinema_edit.jpg', '/assets/generated/portfolio_cinema_edit.jpg', 'docu_reel_frame.jpg', 842100, 'image/jpeg', 'local_original', 0),"
        "(5, 'Titanium & Gold Launch Pack', 'High-contrast creator launch graphics and visual identity system.', 'Thumbnail Designers', 'image', '/assets/generated/portfolio_motion_3d.jpg', '/assets/generated/portfolio_motion_3d.jpg', 'launch_pack.jpg', 791400, 'image/jpeg', 'local_original', 1);"

        "INSERT INTO project_requests (id, client_id, freelancer_id, title, description, budget, deadline, status) VALUES "
        "(1, 7, 1, 'Launch Film 90-Second Master Cut', 'Need pacing edit, sound mix, and warm cinema color grade for our studio launch reel.', 12000, '2026-12-05', 'accepted'),"
        "(2, 7, 2, '3D Logo Bumper & Title Kit', 'Create a 10-second metallic gold intro bumper and lower-thirds template.', 8500, '2026-12-12', 'pending');"

        "INSERT INTO projects (id, client_id, freelancer_id, request_id, title, description, budget, commission_rate, platform_fee, freelancer_earnings, deadline, status, progress_percent, payment_status) VALUES "
        "(1, 7, 1, 1, 'Launch Film 90-Second Master Cut', 'Need pacing edit, sound mix, and warm cinema color grade for our studio launch reel.', 12000, 10.0, 1200.0, 10800.0, '2026-12-05', 'in_progress', 65, 'simulated_paid'),"
        "(2, 7, 3, NULL, 'Lookbook Studio Photography Set', '12 retouched editorial frames for winter collection.', 9000, 10.0, 900.0, 8100.0, '2026-11-10', 'completed', 100, 'simulated_paid');"

        "INSERT INTO transactions (project_id, client_id, freelancer_id, gross_amount, commission_rate, platform_earnings, freelancer_earnings, payment_method, payment_status, is_simulated, reference_code) VALUES "
        "(1, 7, 1, 12000.0, 10.0, 1200.0, 10800.0, 'Simulated Escrow (v1)', 'simulated_paid', 1, 'CC-SIM-1001'),"
        "(2, 7, 3, 9000.0, 10.0, 900.0, 8100.0, 'Simulated Escrow (v1)', 'simulated_paid', 1, 'CC-SIM-1002');"

        "INSERT INTO reviews (project_id, client_id, freelancer_id, rating, comment) VALUES "
        "(2, 7, 3, 5, 'Extraordinary lighting discipline and fast turnaround. Every frame matched our brief.');"

        "INSERT INTO messages (sender_id, receiver_id, project_id, content) VALUES "
        "(7, 1, 1, 'Hi Elena, I uploaded the raw ProRes selects. Looking forward to your first assembly cut!'),"
        "(1, 7, 1, 'Received the selects! Rough assembly is locked and I am starting the warm DaVinci grade tonight.');",
        hash1, salt1,
        hash1, salt1,
        hash1, salt1,
        hash1, salt1,
        hash1, salt1,
        hash1, salt1,
        hash1, salt1
    );

    char *err = NULL;
    if (sqlite3_exec(g_db, sql, NULL, NULL, &err) != SQLITE_OK) {
        fprintf(stderr, "Seed error: %s\n", err);
        sqlite3_free(err);
    }
    free(sql);
}
