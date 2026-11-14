#ifndef AUTHENTICATION_H
#define AUTHENTICATION_H

#include "database.h"

typedef struct {
    int id;
    char full_name[128];
    char email[128];
    char account_type[32];
    char avatar_url[256];
    int freelancer_profile_id;
} AuthUser;

void auth_hash_password(const char *password, const char *salt, char out_hex[65]);
void auth_generate_random_hex(char *out, size_t byte_len);

int auth_authenticate_token(const char *token, AuthUser *out_user);
int auth_extract_bearer(const char *auth_header, char *out_token, size_t max_len);

int auth_handle_register(const char *body, StrBuf *res_json, int *status_code);
int auth_handle_login(const char *body, StrBuf *res_json, int *status_code);
int auth_handle_logout(const char *token, StrBuf *res_json, int *status_code);
int auth_handle_me(const AuthUser *user, StrBuf *res_json, int *status_code);
int auth_handle_change_password(const AuthUser *user, const char *body, StrBuf *res_json, int *status_code);
int auth_handle_forgot_password(const char *body, StrBuf *res_json, int *status_code);

#endif /* AUTHENTICATION_H */
