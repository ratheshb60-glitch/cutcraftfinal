#ifndef FREELANCER_H
#define FREELANCER_H

#include "database.h"
#include "authentication.h"

int freelancer_handle_list(int featured_only, StrBuf *res_json, int *status_code);
int freelancer_handle_get_detail(int freelancer_id, StrBuf *res_json, int *status_code);
int freelancer_handle_upsert_profile(const AuthUser *user, const char *body, StrBuf *res_json, int *status_code);
int freelancer_handle_trending_portfolios(StrBuf *res_json, int *status_code);
int freelancer_handle_add_portfolio(const AuthUser *user, const char *body, StrBuf *res_json, int *status_code);
int freelancer_handle_update_portfolio(const AuthUser *user, int item_id, const char *body, StrBuf *res_json, int *status_code);
int freelancer_handle_delete_portfolio(const AuthUser *user, int item_id, StrBuf *res_json, int *status_code);

#endif /* FREELANCER_H */
