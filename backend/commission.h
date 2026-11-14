#ifndef COMMISSION_H
#define COMMISSION_H

#include "database.h"
#include "authentication.h"

typedef struct {
    double budget;
    double commission_rate;
    double platform_earnings;
    double freelancer_earnings;
} CommissionBreakdown;

double commission_get_rate(void);
int commission_set_rate(double rate);
CommissionBreakdown commission_calculate(double budget, double rate);

int commission_handle_preview(const char *query_string, StrBuf *res_json, int *status_code);
int commission_handle_update_rate(const AuthUser *user, const char *body, StrBuf *res_json, int *status_code);
int commission_handle_pay_project(const AuthUser *user, int project_id, const char *body, StrBuf *res_json, int *status_code);
int commission_handle_earnings_summary(const AuthUser *user, StrBuf *res_json, int *status_code);

#endif /* COMMISSION_H */
