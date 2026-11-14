#ifndef HIRING_H
#define HIRING_H

#include "database.h"
#include "authentication.h"

int hiring_handle_create_request(const AuthUser *user, const char *body, StrBuf *res_json, int *status_code);
int hiring_handle_list_requests(const AuthUser *user, StrBuf *res_json, int *status_code);
int hiring_handle_respond_request(const AuthUser *user, int request_id, const char *action, StrBuf *res_json, int *status_code);

#endif /* HIRING_H */
