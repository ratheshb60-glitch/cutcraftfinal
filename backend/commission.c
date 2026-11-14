#include "commission.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

double commission_get_rate(void) {
    sqlite3 *db = db_get_handle();
    sqlite3_stmt *stmt = NULL;
    double rate = 10.0;
    if (sqlite3_prepare_v2(db, "SELECT value FROM app_config WHERE key = 'commission_rate';", -1, &stmt, NULL) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            const char *val = (const char *)sqlite3_column_text(stmt, 0);
            if (val) {
                double parsed = atof(val);
                if (parsed >= 0.0 && parsed <= 50.0) rate = parsed;
            }
        }
        sqlite3_finalize(stmt);
    }
    return rate;
}

int commission_set_rate(double rate) {
    if (rate < 0.0 || rate > 50.0) return 0;
    char buf[32];
    snprintf(buf, sizeof(buf), "%.2f", rate);
    sqlite3 *db = db_get_handle();
    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(db, "INSERT OR REPLACE INTO app_config (key, value) VALUES ('commission_rate', ?);", -1, &stmt, NULL) != SQLITE_OK) {
        return 0;
    }
    sqlite3_bind_text(stmt, 1, buf, -1, SQLITE_STATIC);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE;
}

CommissionBreakdown commission_calculate(double budget, double rate) {
    CommissionBreakdown cb;
    cb.budget = budget < 0.0 ? 0.0 : budget;
    cb.commission_rate = rate;
    cb.platform_earnings = round((cb.budget * (rate / 100.0)) * 100.0) / 100.0;
    cb.freelancer_earnings = round((cb.budget - cb.platform_earnings) * 100.0) / 100.0;
    return cb;
}

int commission_handle_preview(const char *query_string, StrBuf *res_json, int *status_code) {
    double budget = 1000.0;
    if (query_string) {
        const char *b = strstr(query_string, "budget=");
        if (b) budget = atof(b + 7);
    }
    double rate = commission_get_rate();
    CommissionBreakdown cb = commission_calculate(budget, rate);

    *status_code = 200;
    sb_appendf(res_json,
        "{\"budget\":%.2f,\"commission_rate\":%.2f,\"platform_earnings\":%.2f,\"freelancer_earnings\":%.2f,\"is_simulated\":true}",
        cb.budget, cb.commission_rate, cb.platform_earnings, cb.freelancer_earnings);
    return 1;
}

int commission_handle_update_rate(const AuthUser *user, const char *body, StrBuf *res_json, int *status_code) {
    (void)user;
    double rate = 10.0;
    if (!json_get_double(body, "commission_rate", &rate) || rate < 0.0 || rate > 50.0) {
        *status_code = 400;
        sb_append(res_json, "{\"error\":\"Commission rate must be between 0 and 50 percent.\"}");
        return 0;
    }
    if (!commission_set_rate(rate)) {
        *status_code = 500;
        sb_append(res_json, "{\"error\":\"Failed to update commission rate.\"}");
        return 0;
    }
    *status_code = 200;
    sb_appendf(res_json, "{\"commission_rate\":%.2f,\"message\":\"Platform commission rate updated.\"}", rate);
    return 1;
}

int commission_handle_pay_project(const AuthUser *user, int project_id, const char *body, StrBuf *res_json, int *status_code) {
    char payment_method[128];
    if (!json_get_string(body, "payment_method", payment_method, sizeof(payment_method)) || !payment_method[0]) {
        snprintf(payment_method, sizeof(payment_method), "Simulated Escrow (v1)");
    }

    sqlite3 *db = db_get_handle();
    sqlite3_stmt *stmt = NULL;
    const char *sql = "SELECT id, client_id, freelancer_id, budget, commission_rate, platform_fee, freelancer_earnings, payment_status FROM projects WHERE id = ?;";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        *status_code = 500;
        sb_append(res_json, "{\"error\":\"Database error.\"}");
        return 0;
    }
    sqlite3_bind_int(stmt, 1, project_id);
    if (sqlite3_step(stmt) != SQLITE_ROW) {
        sqlite3_finalize(stmt);
        *status_code = 404;
        sb_append(res_json, "{\"error\":\"Project not found.\"}");
        return 0;
    }

    int client_id = sqlite3_column_int(stmt, 1);
    int freelancer_id = sqlite3_column_int(stmt, 2);
    double budget = sqlite3_column_double(stmt, 3);
    double rate = sqlite3_column_double(stmt, 4);
    double platform_fee = sqlite3_column_double(stmt, 5);
    double freelancer_earnings = sqlite3_column_double(stmt, 6);
    const char *pstatus = (const char *)sqlite3_column_text(stmt, 7);
    int already_paid = (pstatus && strcmp(pstatus, "simulated_paid") == 0);
    sqlite3_finalize(stmt);

    if (client_id != user->id) {
        *status_code = 403;
        sb_append(res_json, "{\"error\":\"Only the project client can authorize simulated payment.\"}");
        return 0;
    }
    if (already_paid) {
        *status_code = 400;
        sb_append(res_json, "{\"error\":\"Simulated payment has already been recorded for this project.\"}");
        return 0;
    }

    char ref_suffix[17];
    auth_generate_random_hex(ref_suffix, 4);
    char ref_code[48];
    snprintf(ref_code, sizeof(ref_code), "CC-SIM-%s", ref_suffix);

    sqlite3_stmt *ustmt = NULL;
    if (sqlite3_prepare_v2(db, "UPDATE projects SET payment_status = 'simulated_paid', updated_at = CURRENT_TIMESTAMP WHERE id = ?;", -1, &ustmt, NULL) == SQLITE_OK) {
        sqlite3_bind_int(ustmt, 1, project_id);
        sqlite3_step(ustmt);
        sqlite3_finalize(ustmt);
    }

    sqlite3_stmt *tstmt = NULL;
    const char *tsql =
        "INSERT INTO transactions "
        "(project_id, client_id, freelancer_id, gross_amount, commission_rate, platform_earnings, freelancer_earnings, payment_method, payment_status, is_simulated, reference_code) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, 'simulated_paid', 1, ?);";
    if (sqlite3_prepare_v2(db, tsql, -1, &tstmt, NULL) == SQLITE_OK) {
        sqlite3_bind_int(tstmt, 1, project_id);
        sqlite3_bind_int(tstmt, 2, client_id);
        sqlite3_bind_int(tstmt, 3, freelancer_id);
        sqlite3_bind_double(tstmt, 4, budget);
        sqlite3_bind_double(tstmt, 5, rate);
        sqlite3_bind_double(tstmt, 6, platform_fee);
        sqlite3_bind_double(tstmt, 7, freelancer_earnings);
        sqlite3_bind_text(tstmt, 8, payment_method, -1, SQLITE_STATIC);
        sqlite3_bind_text(tstmt, 9, ref_code, -1, SQLITE_STATIC);
        sqlite3_step(tstmt);
        sqlite3_finalize(tstmt);
    }

    *status_code = 200;
    sb_append(res_json, "{");
    sb_append(res_json, "\"payment_status\":\"simulated_paid\",");
    sb_append(res_json, "\"is_simulated\":true,");
    sb_append(res_json, "\"notice\":\"Simulated payment recorded for v1 demonstration. No real money was transferred.\",");
    sb_append(res_json, "\"reference_code\":"); sb_append_json_str(res_json, ref_code); sb_append(res_json, ",");
    sb_appendf(res_json, "\"gross_amount\":%.2f,\"commission_rate\":%.2f,\"platform_earnings\":%.2f,\"freelancer_earnings\":%.2f",
        budget, rate, platform_fee, freelancer_earnings);
    sb_append(res_json, "}");
    return 1;
}

int commission_handle_earnings_summary(const AuthUser *user, StrBuf *res_json, int *status_code) {
    sqlite3 *db = db_get_handle();
    sqlite3_stmt *stmt = NULL;
    double total_gross = 0, total_commission = 0, total_net = 0;
    int tx_count = 0;

    const char *sum_sql = strcmp(user->account_type, "freelancer") == 0
        ? "SELECT COALESCE(SUM(gross_amount),0), COALESCE(SUM(platform_earnings),0), COALESCE(SUM(freelancer_earnings),0), COUNT(*) FROM transactions WHERE freelancer_id = ?;"
        : "SELECT COALESCE(SUM(gross_amount),0), COALESCE(SUM(platform_earnings),0), COALESCE(SUM(freelancer_earnings),0), COUNT(*) FROM transactions WHERE client_id = ?;";

    if (sqlite3_prepare_v2(db, sum_sql, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, strcmp(user->account_type, "freelancer") == 0 ? user->freelancer_profile_id : user->id);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            total_gross = sqlite3_column_double(stmt, 0);
            total_commission = sqlite3_column_double(stmt, 1);
            total_net = sqlite3_column_double(stmt, 2);
            tx_count = sqlite3_column_int(stmt, 3);
        }
        sqlite3_finalize(stmt);
    }

    sb_append(res_json, "{");
    sb_appendf(res_json, "\"commission_rate\":%.2f,", commission_get_rate());
    sb_appendf(res_json, "\"total_gross\":%.2f,", total_gross);
    sb_appendf(res_json, "\"total_commission\":%.2f,", total_commission);
    sb_appendf(res_json, "\"total_net_earnings\":%.2f,", total_net);
    sb_appendf(res_json, "\"transaction_count\":%d,", tx_count);
    sb_append(res_json, "\"transactions\":[");

    const char *list_sql = strcmp(user->account_type, "freelancer") == 0
        ? "SELECT t.id, t.project_id, p.title, t.gross_amount, t.commission_rate, t.platform_earnings, t.freelancer_earnings, t.payment_method, t.payment_status, t.reference_code, t.created_at "
          "FROM transactions t JOIN projects p ON t.project_id = p.id WHERE t.freelancer_id = ? ORDER BY t.id DESC;"
        : "SELECT t.id, t.project_id, p.title, t.gross_amount, t.commission_rate, t.platform_earnings, t.freelancer_earnings, t.payment_method, t.payment_status, t.reference_code, t.created_at "
          "FROM transactions t JOIN projects p ON t.project_id = p.id WHERE t.client_id = ? ORDER BY t.id DESC;";

    sqlite3_stmt *lstmt = NULL;
    if (sqlite3_prepare_v2(db, list_sql, -1, &lstmt, NULL) == SQLITE_OK) {
        sqlite3_bind_int(lstmt, 1, strcmp(user->account_type, "freelancer") == 0 ? user->freelancer_profile_id : user->id);
        int first = 1;
        while (sqlite3_step(lstmt) == SQLITE_ROW) {
            if (!first) sb_append(res_json, ",");
            first = 0;
            sb_append(res_json, "{");
            sb_appendf(res_json, "\"id\":%d,", sqlite3_column_int(lstmt, 0));
            sb_appendf(res_json, "\"project_id\":%d,", sqlite3_column_int(lstmt, 1));
            sb_append(res_json, "\"project_title\":"); sb_append_json_str(res_json, (const char *)sqlite3_column_text(lstmt, 2)); sb_append(res_json, ",");
            sb_appendf(res_json, "\"gross_amount\":%.2f,", sqlite3_column_double(lstmt, 3));
            sb_appendf(res_json, "\"commission_rate\":%.2f,", sqlite3_column_double(lstmt, 4));
            sb_appendf(res_json, "\"platform_earnings\":%.2f,", sqlite3_column_double(lstmt, 5));
            sb_appendf(res_json, "\"freelancer_earnings\":%.2f,", sqlite3_column_double(lstmt, 6));
            sb_append(res_json, "\"payment_method\":"); sb_append_json_str(res_json, (const char *)sqlite3_column_text(lstmt, 7)); sb_append(res_json, ",");
            sb_append(res_json, "\"payment_status\":"); sb_append_json_str(res_json, (const char *)sqlite3_column_text(lstmt, 8)); sb_append(res_json, ",");
            sb_append(res_json, "\"reference_code\":"); sb_append_json_str(res_json, (const char *)sqlite3_column_text(lstmt, 9)); sb_append(res_json, ",");
            sb_append(res_json, "\"created_at\":"); sb_append_json_str(res_json, (const char *)sqlite3_column_text(lstmt, 10));
            sb_append(res_json, "}");
        }
        sqlite3_finalize(lstmt);
    }

    sb_append(res_json, "]}");
    *status_code = 200;
    return 1;
}
