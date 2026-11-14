#ifndef DATABASE_H
#define DATABASE_H

#include <sqlite3.h>
#include <stddef.h>

typedef struct {
    char *data;
    size_t len;
    size_t cap;
} StrBuf;

void sb_init(StrBuf *sb);
void sb_free(StrBuf *sb);
void sb_append(StrBuf *sb, const char *str);
void sb_appendf(StrBuf *sb, const char *fmt, ...);
void sb_append_json_str(StrBuf *sb, const char *str);

/* JSON extraction helpers */
int json_get_string(const char *json, const char *key, char *out, size_t out_size);
int json_get_double(const char *json, const char *key, double *out);
int json_get_int(const char *json, const char *key, int *out);

/* Database lifecycle */
int db_init(const char *db_path);
void db_close(void);
sqlite3 *db_get_handle(void);
void db_seed_initial_data(void);

#endif /* DATABASE_H */
