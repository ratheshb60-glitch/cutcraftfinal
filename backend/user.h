#ifndef USER_H
#define USER_H

#include "database.h"
#include "authentication.h"

int user_handle_update_profile(const AuthUser *user, const char *body, StrBuf *res_json, int *status_code);
int user_handle_get_by_id(int user_id, StrBuf *res_json, int *status_code);

#endif /* USER_H */
