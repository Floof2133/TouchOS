// kernel/net/http.h
// Simple HTTP client for package downloads
// Created by: floof<3

#ifndef HTTP_H
#define HTTP_H

#include <stdint.h>
#include <stddef.h>
#include "network.h"

// HTTP methods
typedef enum {
    HTTP_GET,
    HTTP_POST,
    HTTP_PUT,
    HTTP_DELETE
} http_method_t;

// HTTP response
typedef struct {
    int status_code;
    char* headers;
    size_t headers_len;
    uint8_t* body;
    size_t body_len;
    size_t content_length;
} http_response_t;

// HTTP request
typedef struct {
    http_method_t method;
    char* host;
    uint16_t port;
    char* path;
    char* headers;
    uint8_t* body;
    size_t body_len;
} http_request_t;

// HTTP client functions
int http_get(const char* url, http_response_t* response);
int http_post(const char* url, const void* data, size_t len, http_response_t* response);
int http_download_file(const char* url, const char* output_path);

// Utility functions
int http_parse_url(const char* url, char** host, uint16_t* port, char** path);
void http_free_response(http_response_t* response);

#endif // HTTP_H
