#ifndef PROJECTS_H
#define PROJECTS_H

#include "database.h"
#include "authentication.h"

int projects_handle_list(const AuthUser *user, StrBuf *res_json, int *status_code);
int projects_handle_update_status(const AuthUser *user, int project_id, const char *body, StrBuf *res_json, int *status_code);
int projects_handle_create_review(const AuthUser *user, int project_id, const char *body, StrBuf *res_json, int *status_code);

/* Client-Freelancer Messaging */
int projects_handle_list_conversations(const AuthUser *user, StrBuf *res_json, int *status_code);
int projects_handle_get_messages(const AuthUser *user, int peer_user_id, StrBuf *res_json, int *status_code);
int projects_handle_send_message(const AuthUser *user, const char *body, StrBuf *res_json, int *status_code);

#endif /* PROJECTS_H */
