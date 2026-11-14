#ifndef SEARCH_H
#define SEARCH_H

#include "database.h"

int search_handle_freelancers(const char *query_string, StrBuf *res_json, int *status_code);

#endif /* SEARCH_H */
