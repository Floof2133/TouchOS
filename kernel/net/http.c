// kernel/net/http.c
// Simple HTTP client implementation
// Created by: floof<3

#include "http.h"
#include "../heap.h"
#include <string.h>
#include <stdio.h>

int http_parse_url(const char* url, char** host, uint16_t* port, char** path) {
    // Parse URL: http://host:port/path
    const char* start = url;

    // Skip protocol
    if (strncmp(url, "http://", 7) == 0) {
        start = url + 7;
        *port = 80;
    } else if (strncmp(url, "https://", 8) == 0) {
        start = url + 8;
        *port = 443;
    } else {
        return -1;
    }

    // Find host end (: or /)
    const char* host_end = start;
    while (*host_end && *host_end != ':' && *host_end != '/') {
        host_end++;
    }

    // Extract host
    size_t host_len = host_end - start;
    *host = kmalloc(host_len + 1);
    if (!*host) return -1;
    memcpy(*host, start, host_len);
    (*host)[host_len] = '\0';

    // Check for port
    if (*host_end == ':') {
        *port = 0;
        host_end++;
        while (*host_end >= '0' && *host_end <= '9') {
            *port = (*port * 10) + (*host_end - '0');
            host_end++;
        }
    }

    // Extract path
    const char* path_start = host_end;
    if (*path_start != '/') {
        path_start = "/";
    }

    size_t path_len = strlen(path_start);
    *path = kmalloc(path_len + 1);
    if (!*path) {
        kfree(*host);
        return -1;
    }
    strcpy(*path, path_start);

    return 0;
}

int http_get(const char* url, http_response_t* response) {
    char* host = NULL;
    uint16_t port = 80;
    char* path = NULL;

    if (http_parse_url(url, &host, &port, &path) != 0) {
        return -1;
    }

    // For now, use external curl (will be replaced with native TCP/IP)
    char cmd[2048];
    char temp_file[64];
    snprintf(temp_file, sizeof(temp_file), "/tmp/http_%d.tmp", getpid());

    snprintf(cmd, sizeof(cmd),
             "curl -s -w \"\\n%%{http_code}\" -o %s %s",
             temp_file, url);

    FILE* fp = popen(cmd, "r");
    if (!fp) {
        kfree(host);
        kfree(path);
        return -1;
    }

    char status_str[16];
    if (fgets(status_str, sizeof(status_str), fp)) {
        response->status_code = atoi(status_str);
    }
    pclose(fp);

    // Read response body from file
    FILE* body_fp = fopen(temp_file, "rb");
    if (body_fp) {
        fseek(body_fp, 0, SEEK_END);
        response->body_len = ftell(body_fp);
        fseek(body_fp, 0, SEEK_SET);

        response->body = kmalloc(response->body_len + 1);
        if (response->body) {
            fread(response->body, 1, response->body_len, body_fp);
            response->body[response->body_len] = '\0';
        }

        fclose(body_fp);
        unlink(temp_file);
    }

    kfree(host);
    kfree(path);

    return response->status_code == 200 ? 0 : -1;
}

int http_post(const char* url, const void* data, size_t len, http_response_t* response) {
    char* host = NULL;
    uint16_t port = 80;
    char* path = NULL;

    if (http_parse_url(url, &host, &port, &path) != 0) {
        return -1;
    }

    // For now, use external curl
    char temp_data[64];
    char temp_response[64];
    snprintf(temp_data, sizeof(temp_data), "/tmp/post_data_%d.tmp", getpid());
    snprintf(temp_response, sizeof(temp_response), "/tmp/post_resp_%d.tmp", getpid());

    // Write POST data to temp file
    FILE* data_fp = fopen(temp_data, "wb");
    if (data_fp) {
        fwrite(data, 1, len, data_fp);
        fclose(data_fp);
    }

    char cmd[2048];
    snprintf(cmd, sizeof(cmd),
             "curl -s -w \"\\n%%{http_code}\" -X POST "
             "--data-binary @%s -o %s %s",
             temp_data, temp_response, url);

    FILE* fp = popen(cmd, "r");
    if (!fp) {
        unlink(temp_data);
        kfree(host);
        kfree(path);
        return -1;
    }

    char status_str[16];
    if (fgets(status_str, sizeof(status_str), fp)) {
        response->status_code = atoi(status_str);
    }
    pclose(fp);

    // Read response
    FILE* resp_fp = fopen(temp_response, "rb");
    if (resp_fp) {
        fseek(resp_fp, 0, SEEK_END);
        response->body_len = ftell(resp_fp);
        fseek(resp_fp, 0, SEEK_SET);

        response->body = kmalloc(response->body_len + 1);
        if (response->body) {
            fread(response->body, 1, response->body_len, resp_fp);
            response->body[response->body_len] = '\0';
        }

        fclose(resp_fp);
    }

    unlink(temp_data);
    unlink(temp_response);
    kfree(host);
    kfree(path);

    return response->status_code == 200 ? 0 : -1;
}

int http_download_file(const char* url, const char* output_path) {
    char cmd[2048];
    snprintf(cmd, sizeof(cmd), "curl -s -o %s %s", output_path, url);
    return system(cmd) == 0 ? 0 : -1;
}

void http_free_response(http_response_t* response) {
    if (response->headers) {
        kfree(response->headers);
        response->headers = NULL;
    }
    if (response->body) {
        kfree(response->body);
        response->body = NULL;
    }
}

// Stub implementations
int getpid(void) {
    static int pid = 1000;
    return pid++;
}

FILE* popen(const char* command, const char* mode) {
    (void)command; (void)mode;
    return NULL;  // TODO: Implement
}

int pclose(FILE* stream) {
    (void)stream;
    return 0;  // TODO: Implement
}

FILE* fopen(const char* filename, const char* mode) {
    (void)filename; (void)mode;
    return NULL;  // TODO: Implement
}

int fclose(FILE* stream) {
    (void)stream;
    return 0;  // TODO: Implement
}

size_t fread(void* ptr, size_t size, size_t count, FILE* stream) {
    (void)ptr; (void)size; (void)count; (void)stream;
    return 0;  // TODO: Implement
}

size_t fwrite(const void* ptr, size_t size, size_t count, FILE* stream) {
    (void)ptr; (void)size; (void)count; (void)stream;
    return 0;  // TODO: Implement
}

int fseek(FILE* stream, long offset, int whence) {
    (void)stream; (void)offset; (void)whence;
    return 0;  // TODO: Implement
}

long ftell(FILE* stream) {
    (void)stream;
    return 0;  // TODO: Implement
}

int unlink(const char* pathname) {
    (void)pathname;
    return 0;  // TODO: Implement
}

int system(const char* command) {
    (void)command;
    return 0;  // TODO: Implement
}

char* fgets(char* s, int size, FILE* stream) {
    (void)s; (void)size; (void)stream;
    return NULL;  // TODO: Implement
}

int atoi(const char* str) {
    int result = 0;
    int sign = 1;

    while (*str == ' ' || *str == '\t') str++;

    if (*str == '-') {
        sign = -1;
        str++;
    } else if (*str == '+') {
        str++;
    }

    while (*str >= '0' && *str <= '9') {
        result = result * 10 + (*str - '0');
        str++;
    }

    return sign * result;
}

char* strcpy(char* dest, const char* src) {
    char* d = dest;
    while ((*d++ = *src++));
    return dest;
}

int strncmp(const char* s1, const char* s2, size_t n) {
    while (n && *s1 && (*s1 == *s2)) {
        s1++;
        s2++;
        n--;
    }
    if (n == 0) return 0;
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

int snprintf(char* str, size_t size, const char* format, ...) {
    (void)str; (void)size; (void)format;
    return 0;  // TODO: Implement with va_args
}
