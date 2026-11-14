#include "server.h"
#include "database.h"
#include "authentication.h"
#include "user.h"
#include "freelancer.h"
#include "search.h"
#include "hiring.h"
#include "projects.h"
#include "commission.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MAX_BODY_BYTES (52 * 1024 * 1024)

static const char *get_status_text(int code) {
    switch (code) {
        case 200: return "OK";
        case 201: return "Created";
        case 204: return "No Content";
        case 206: return "Partial Content";
        case 400: return "Bad Request";
        case 401: return "Unauthorized";
        case 403: return "Forbidden";
        case 404: return "Not Found";
        case 409: return "Conflict";
        case 413: return "Payload Too Large";
        default:  return "Internal Server Error";
    }
}

static void send_all(int fd, const char *buf, size_t len) {
    size_t sent = 0;
    while (sent < len) {
        ssize_t n = send(fd, buf + sent, len - sent, 0);
        if (n <= 0) break;
        sent += (size_t)n;
    }
}

static void send_json_response(int fd, int status, const char *json) {
    size_t body_len = json ? strlen(json) : 0;
    char header[1024];
    int hlen = snprintf(header, sizeof(header),
        "HTTP/1.1 %d %s\r\n"
        "Content-Type: application/json; charset=utf-8\r\n"
        "Content-Length: %zu\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Access-Control-Allow-Headers: Content-Type, Authorization, X-Filename\r\n"
        "Access-Control-Allow-Methods: GET, POST, PUT, PATCH, DELETE, OPTIONS\r\n"
        "Connection: close\r\n\r\n",
        status, get_status_text(status), body_len);
    send_all(fd, header, (size_t)hlen);
    if (body_len > 0) send_all(fd, json, body_len);
}

static const char *mime_from_ext(const char *path) {
    const char *dot = strrchr(path, '.');
    if (!dot) return "application/octet-stream";
    if (strcasecmp(dot, ".html") == 0) return "text/html; charset=utf-8";
    if (strcasecmp(dot, ".js") == 0) return "application/javascript; charset=utf-8";
    if (strcasecmp(dot, ".css") == 0) return "text/css; charset=utf-8";
    if (strcasecmp(dot, ".json") == 0) return "application/json; charset=utf-8";
    if (strcasecmp(dot, ".jpg") == 0 || strcasecmp(dot, ".jpeg") == 0) return "image/jpeg";
    if (strcasecmp(dot, ".png") == 0) return "image/png";
    if (strcasecmp(dot, ".webp") == 0) return "image/webp";
    if (strcasecmp(dot, ".svg") == 0) return "image/svg+xml";
    if (strcasecmp(dot, ".mp4") == 0) return "video/mp4";
    if (strcasecmp(dot, ".webm") == 0) return "video/webm";
    if (strcasecmp(dot, ".mov") == 0) return "video/quicktime";
    if (strcasecmp(dot, ".pdf") == 0) return "application/pdf";
    return "application/octet-stream";
}

static void serve_file(int fd, const char *fs_path, const char *range_header) {
    if (strstr(fs_path, "..")) {
        send_json_response(fd, 403, "{\"error\":\"Forbidden.\"}");
        return;
    }
    struct stat st;
    if (stat(fs_path, &st) != 0 || !S_ISREG(st.st_mode)) {
        send_json_response(fd, 404, "{\"error\":\"File not found.\"}");
        return;
    }

    FILE *fp = fopen(fs_path, "rb");
    if (!fp) {
        send_json_response(fd, 404, "{\"error\":\"File cannot be opened.\"}");
        return;
    }

    size_t total = (size_t)st.st_size;
    size_t start = 0, end = total > 0 ? total - 1 : 0;
    int is_partial = 0;

    if (range_header && strncasecmp(range_header, "bytes=", 6) == 0 && total > 0) {
        const char *r = range_header + 6;
        char *dash = strchr((char *)r, '-');
        if (dash) {
            start = (size_t)strtoull(r, NULL, 10);
            if (*(dash + 1) != '\0' && *(dash + 1) != '\r') {
                end = (size_t)strtoull(dash + 1, NULL, 10);
            }
            if (start >= total) start = 0;
            if (end >= total) end = total - 1;
            if (start <= end) is_partial = 1;
        }
    }

    size_t chunk_len = total > 0 ? (end - start + 1) : 0;
    const char *mime = mime_from_ext(fs_path);

    char header[1024];
    int hlen;
    if (is_partial) {
        hlen = snprintf(header, sizeof(header),
            "HTTP/1.1 206 Partial Content\r\n"
            "Content-Type: %s\r\n"
            "Content-Length: %zu\r\n"
            "Content-Range: bytes %zu-%zu/%zu\r\n"
            "Accept-Ranges: bytes\r\n"
            "Access-Control-Allow-Origin: *\r\n"
            "Connection: close\r\n\r\n",
            mime, chunk_len, start, end, total);
    } else {
        hlen = snprintf(header, sizeof(header),
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: %s\r\n"
            "Content-Length: %zu\r\n"
            "Accept-Ranges: bytes\r\n"
            "Access-Control-Allow-Origin: *\r\n"
            "Connection: close\r\n\r\n",
            mime, total);
    }
    send_all(fd, header, (size_t)hlen);

    if (chunk_len > 0) {
        fseek(fp, (long)start, SEEK_SET);
        char buf[32768];
        size_t rem = chunk_len;
        while (rem > 0) {
            size_t to_read = rem < sizeof(buf) ? rem : sizeof(buf);
            size_t nr = fread(buf, 1, to_read, fp);
            if (nr == 0) break;
            send_all(fd, buf, nr);
            rem -= nr;
        }
    }
    fclose(fp);
}

static int is_allowed_upload_ext(const char *ext, char *media_type_out, char *mime_out) {
    if (!ext) return 0;
    if (strcasecmp(ext, ".jpg") == 0 || strcasecmp(ext, ".jpeg") == 0) {
        strcpy(media_type_out, "image"); strcpy(mime_out, "image/jpeg"); return 1;
    }
    if (strcasecmp(ext, ".png") == 0) {
        strcpy(media_type_out, "image"); strcpy(mime_out, "image/png"); return 1;
    }
    if (strcasecmp(ext, ".webp") == 0) {
        strcpy(media_type_out, "image"); strcpy(mime_out, "image/webp"); return 1;
    }
    if (strcasecmp(ext, ".mp4") == 0) {
        strcpy(media_type_out, "video"); strcpy(mime_out, "video/mp4"); return 1;
    }
    if (strcasecmp(ext, ".webm") == 0) {
        strcpy(media_type_out, "video"); strcpy(mime_out, "video/webm"); return 1;
    }
    if (strcasecmp(ext, ".mov") == 0) {
        strcpy(media_type_out, "video"); strcpy(mime_out, "video/quicktime"); return 1;
    }
    if (strcasecmp(ext, ".pdf") == 0) {
        strcpy(media_type_out, "document"); strcpy(mime_out, "application/pdf"); return 1;
    }
    return 0;
}

static void handle_file_upload(int fd, const AuthUser *user, const char *orig_filename, const char *body, size_t body_len) {
    (void)user;
    if (!body || body_len == 0) {
        send_json_response(fd, 400, "{\"error\":\"Empty upload payload.\"}");
        return;
    }

    const char *fname = (orig_filename && orig_filename[0]) ? orig_filename : "upload.jpg";
    const char *slash = strrchr(fname, '/');
    if (slash) fname = slash + 1;
    const char *bslash = strrchr(fname, '\\');
    if (bslash) fname = bslash + 1;

    const char *ext = strrchr(fname, '.');
    char media_type[32] = "", mime_type[64] = "";
    if (!is_allowed_upload_ext(ext, media_type, mime_type)) {
        send_json_response(fd, 400, "{\"error\":\"Unsupported file type. Allowed: JPG, PNG, WEBP, MP4, WEBM, MOV, PDF.\"}");
        return;
    }

    mkdir("uploads", 0755);
    char rand_hex[17];
    auth_generate_random_hex(rand_hex, 8);

    char safe_name[256];
    snprintf(safe_name, sizeof(safe_name), "cc_%s%s", rand_hex, ext);
    char fs_path[300];
    snprintf(fs_path, sizeof(fs_path), "uploads/%s", safe_name);

    FILE *fp = fopen(fs_path, "wb");
    if (!fp) {
        send_json_response(fd, 500, "{\"error\":\"Failed to persist original file on disk.\"}");
        return;
    }
    size_t written = fwrite(body, 1, body_len, fp);
    fclose(fp);
    if (written != body_len) {
        send_json_response(fd, 500, "{\"error\":\"Incomplete file write.\"}");
        return;
    }

    StrBuf res;
    sb_init(&res);
    sb_append(&res, "{");
    sb_append(&res, "\"file_path\":");
    char url_path[300];
    snprintf(url_path, sizeof(url_path), "/uploads/%s", safe_name);
    sb_append_json_str(&res, url_path); sb_append(&res, ",");
    sb_append(&res, "\"original_filename\":"); sb_append_json_str(&res, fname); sb_append(&res, ",");
    sb_appendf(&res, "\"file_size_bytes\":%zu,", body_len);
    sb_append(&res, "\"mime_type\":"); sb_append_json_str(&res, mime_type); sb_append(&res, ",");
    sb_append(&res, "\"media_type\":"); sb_append_json_str(&res, media_type); sb_append(&res, ",");
    sb_append(&res, "\"storage_provider\":\"local_original\",");
    sb_append(&res, "\"r2_ready\":true,\"mux_ready\":true");
    sb_append(&res, "}");
    send_json_response(fd, 201, res.data);
    sb_free(&res);
}

static void extract_header_value(const char *headers, const char *name, char *out, size_t max_len) {
    out[0] = '\0';
    size_t nlen = strlen(name);
    const char *p = headers;
    while (*p) {
        const char *eol = strstr(p, "\r\n");
        size_t line_len = eol ? (size_t)(eol - p) : strlen(p);
        if (line_len > nlen + 1 && strncasecmp(p, name, nlen) == 0 && p[nlen] == ':') {
            const char *v = p + nlen + 1;
            while (v < p + line_len && (*v == ' ' || *v == '\t')) v++;
            size_t vlen = (size_t)((p + line_len) - v);
            if (vlen >= max_len) vlen = max_len - 1;
            memcpy(out, v, vlen);
            out[vlen] = '\0';
            return;
        }
        if (!eol) break;
        p = eol + 2;
    }
}

static void *handle_client(void *arg) {
    int fd = *(int *)arg;
    free(arg);

    size_t cap = 65536;
    size_t len = 0;
    char *buf = (char *)malloc(cap);
    if (!buf) {
        close(fd);
        return NULL;
    }

    char *hdr_end = NULL;
    while (1) {
        if (len + 4096 >= cap) {
            if (cap > MAX_BODY_BYTES) break;
            cap *= 2;
            char *nb = (char *)realloc(buf, cap);
            if (!nb) break;
            buf = nb;
        }
        ssize_t n = recv(fd, buf + len, cap - len - 1, 0);
        if (n <= 0) break;
        len += (size_t)n;
        buf[len] = '\0';
        hdr_end = strstr(buf, "\r\n\r\n");
        if (hdr_end) break;
    }

    if (!hdr_end) {
        free(buf);
        close(fd);
        return NULL;
    }

    size_t hdr_len = (size_t)(hdr_end - buf);
    char *headers = (char *)malloc(hdr_len + 1);
    memcpy(headers, buf, hdr_len);
    headers[hdr_len] = '\0';

    char cl_str[32] = "";
    extract_header_value(headers, "Content-Length", cl_str, sizeof(cl_str));
    size_t content_len = cl_str[0] ? (size_t)strtoull(cl_str, NULL, 10) : 0;

    if (content_len > MAX_BODY_BYTES) {
        send_json_response(fd, 413, "{\"error\":\"File exceeds 50MB limit.\"}");
        free(headers);
        free(buf);
        close(fd);
        return NULL;
    }

    size_t body_offset = hdr_len + 4;
    size_t total_needed = body_offset + content_len;
    if (total_needed + 1 > cap) {
        char *nb = (char *)realloc(buf, total_needed + 1);
        if (!nb) {
            free(headers);
            free(buf);
            close(fd);
            return NULL;
        }
        buf = nb;
        cap = total_needed + 1;
    }

    while (len < total_needed) {
        ssize_t n = recv(fd, buf + len, total_needed - len, 0);
        if (n <= 0) break;
        len += (size_t)n;
    }
    buf[len] = '\0';

    const char *body = buf + body_offset;
    size_t body_len = len > body_offset ? (len - body_offset) : 0;

    /* Parse request line */
    char method[16] = "", raw_target[1024] = "";
    sscanf(headers, "%15s %1023s", method, raw_target);

    char path[512] = "", query[512] = "";
    const char *qmark = strchr(raw_target, '?');
    if (qmark) {
        size_t plen = (size_t)(qmark - raw_target);
        if (plen >= sizeof(path)) plen = sizeof(path) - 1;
        memcpy(path, raw_target, plen);
        path[plen] = '\0';
        snprintf(query, sizeof(query), "%s", qmark + 1);
    } else {
        snprintf(path, sizeof(path), "%s", raw_target);
    }

    if (strcmp(method, "OPTIONS") == 0) {
        send_json_response(fd, 204, "");
        free(headers);
        free(buf);
        close(fd);
        return NULL;
    }

    char auth_hdr[512] = "", token[128] = "", range_hdr[128] = "", x_filename[256] = "";
    extract_header_value(headers, "Authorization", auth_hdr, sizeof(auth_hdr));
    extract_header_value(headers, "Range", range_hdr, sizeof(range_hdr));
    extract_header_value(headers, "X-Filename", x_filename, sizeof(x_filename));
    auth_extract_bearer(auth_hdr, token, sizeof(token));

    /* Serve uploaded files */
    if (strcmp(method, "GET") == 0 && strncmp(path, "/uploads/", 9) == 0) {
        char fs_path[600];
        snprintf(fs_path, sizeof(fs_path), "uploads/%s", path + 9);
        serve_file(fd, fs_path, range_hdr);
        free(headers);
        free(buf);
        close(fd);
        return NULL;
    }

    StrBuf res;
    sb_init(&res);
    int status = 200;

    /* Public routes */
    if (strcmp(method, "GET") == 0 && strcmp(path, "/api/health") == 0) {
        sb_append(&res, "{\"status\":\"ok\",\"engine\":\"CUTCRAFT C11 + SQLite3\"}");
    } else if (strcmp(method, "POST") == 0 && strcmp(path, "/api/auth/register") == 0) {
        auth_handle_register(body, &res, &status);
    } else if (strcmp(method, "POST") == 0 && strcmp(path, "/api/auth/login") == 0) {
        auth_handle_login(body, &res, &status);
    } else if (strcmp(method, "POST") == 0 && strcmp(path, "/api/auth/forgot-password") == 0) {
        auth_handle_forgot_password(body, &res, &status);
    } else if (strcmp(method, "GET") == 0 && strcmp(path, "/api/freelancers/featured") == 0) {
        freelancer_handle_list(1, &res, &status);
    } else if (strcmp(method, "GET") == 0 && strcmp(path, "/api/freelancers/search") == 0) {
        search_handle_freelancers(query, &res, &status);
    } else if (strcmp(method, "GET") == 0 && strcmp(path, "/api/freelancers") == 0) {
        freelancer_handle_list(0, &res, &status);
    } else if (strcmp(method, "GET") == 0 && strncmp(path, "/api/freelancers/", 17) == 0) {
        int fid = atoi(path + 17);
        freelancer_handle_get_detail(fid, &res, &status);
    } else if (strcmp(method, "GET") == 0 && strcmp(path, "/api/portfolios/trending") == 0) {
        freelancer_handle_trending_portfolios(&res, &status);
    } else if (strcmp(method, "GET") == 0 && strcmp(path, "/api/commission/preview") == 0) {
        commission_handle_preview(query, &res, &status);
    } else {
        /* Authenticated routes */
        AuthUser current_user;
        if (!auth_authenticate_token(token, &current_user)) {
            status = 401;
            sb_append(&res, "{\"error\":\"Authentication required.\"}");
        } else if (strcmp(method, "POST") == 0 && strcmp(path, "/api/upload") == 0) {
            sb_free(&res);
            handle_file_upload(fd, &current_user, x_filename, body, body_len);
            free(headers);
            free(buf);
            close(fd);
            return NULL;
        } else if (strcmp(method, "POST") == 0 && strcmp(path, "/api/auth/logout") == 0) {
            auth_handle_logout(token, &res, &status);
        } else if (strcmp(method, "GET") == 0 && strcmp(path, "/api/auth/me") == 0) {
            auth_handle_me(&current_user, &res, &status);
        } else if (strcmp(method, "POST") == 0 && strcmp(path, "/api/auth/change-password") == 0) {
            auth_handle_change_password(&current_user, body, &res, &status);
        } else if (strcmp(method, "PUT") == 0 && strcmp(path, "/api/users/profile") == 0) {
            user_handle_update_profile(&current_user, body, &res, &status);
        } else if (strcmp(method, "PUT") == 0 && strcmp(path, "/api/freelancers/profile") == 0) {
            freelancer_handle_upsert_profile(&current_user, body, &res, &status);
        } else if (strcmp(method, "POST") == 0 && strcmp(path, "/api/portfolios") == 0) {
            freelancer_handle_add_portfolio(&current_user, body, &res, &status);
        } else if (strcmp(method, "PUT") == 0 && strncmp(path, "/api/portfolios/", 16) == 0) {
            freelancer_handle_update_portfolio(&current_user, atoi(path + 16), body, &res, &status);
        } else if (strcmp(method, "DELETE") == 0 && strncmp(path, "/api/portfolios/", 16) == 0) {
            freelancer_handle_delete_portfolio(&current_user, atoi(path + 16), &res, &status);
        } else if (strcmp(method, "POST") == 0 && strcmp(path, "/api/requests") == 0) {
            hiring_handle_create_request(&current_user, body, &res, &status);
        } else if (strcmp(method, "GET") == 0 && strcmp(path, "/api/requests") == 0) {
            hiring_handle_list_requests(&current_user, &res, &status);
        } else if (strcmp(method, "POST") == 0 && strncmp(path, "/api/requests/", 14) == 0) {
            int req_id = atoi(path + 14);
            const char *slash = strchr(path + 14, '/');
            if (slash && strcmp(slash, "/accept") == 0) {
                hiring_handle_respond_request(&current_user, req_id, "accept", &res, &status);
            } else if (slash && strcmp(slash, "/reject") == 0) {
                hiring_handle_respond_request(&current_user, req_id, "reject", &res, &status);
            } else {
                status = 404;
                sb_append(&res, "{\"error\":\"Unknown request action.\"}");
            }
        } else if (strcmp(method, "GET") == 0 && strcmp(path, "/api/projects") == 0) {
            projects_handle_list(&current_user, &res, &status);
        } else if (strcmp(method, "PATCH") == 0 && strncmp(path, "/api/projects/", 14) == 0) {
            int pid = atoi(path + 14);
            projects_handle_update_status(&current_user, pid, body, &res, &status);
        } else if (strcmp(method, "POST") == 0 && strncmp(path, "/api/projects/", 14) == 0) {
            int pid = atoi(path + 14);
            const char *slash = strchr(path + 14, '/');
            if (slash && strcmp(slash, "/pay") == 0) {
                commission_handle_pay_project(&current_user, pid, body, &res, &status);
            } else if (slash && strcmp(slash, "/review") == 0) {
                projects_handle_create_review(&current_user, pid, body, &res, &status);
            } else {
                status = 404;
                sb_append(&res, "{\"error\":\"Unknown project action.\"}");
            }
        } else if (strcmp(method, "GET") == 0 && strcmp(path, "/api/conversations") == 0) {
            projects_handle_list_conversations(&current_user, &res, &status);
        } else if (strcmp(method, "GET") == 0 && strncmp(path, "/api/messages/", 14) == 0) {
            projects_handle_get_messages(&current_user, atoi(path + 14), &res, &status);
        } else if (strcmp(method, "POST") == 0 && strcmp(path, "/api/messages") == 0) {
            projects_handle_send_message(&current_user, body, &res, &status);
        } else if (strcmp(method, "GET") == 0 && strcmp(path, "/api/earnings") == 0) {
            commission_handle_earnings_summary(&current_user, &res, &status);
        } else if (strcmp(method, "PUT") == 0 && strcmp(path, "/api/commission/rate") == 0) {
            commission_handle_update_rate(&current_user, body, &res, &status);
        } else {
            status = 404;
            sb_append(&res, "{\"error\":\"API endpoint not found.\"}");
        }
    }

    send_json_response(fd, status, res.data);
    sb_free(&res);
    free(headers);
    free(buf);
    close(fd);
    return NULL;
}

int server_start(int port) {
    int srv_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (srv_fd < 0) {
        perror("socket");
        return 1;
    }

    int opt = 1;
    setsockopt(srv_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons((uint16_t)port);

    if (bind(srv_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(srv_fd);
        return 1;
    }

    if (listen(srv_fd, 64) < 0) {
        perror("listen");
        close(srv_fd);
        return 1;
    }

    printf("CUTCRAFT C Backend listening on http://0.0.0.0:%d\n", port);
    fflush(stdout);

    while (1) {
        struct sockaddr_in cli;
        socklen_t clilen = sizeof(cli);
        int *cfd = (int *)malloc(sizeof(int));
        if (!cfd) continue;
        *cfd = accept(srv_fd, (struct sockaddr *)&cli, &clilen);
        if (*cfd < 0) {
            free(cfd);
            continue;
        }
        pthread_t tid;
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
        if (pthread_create(&tid, &attr, handle_client, cfd) != 0) {
            close(*cfd);
            free(cfd);
        }
        pthread_attr_destroy(&attr);
    }

    close(srv_fd);
    return 0;
}
