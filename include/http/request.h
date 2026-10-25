/**
 * The HttpRequest/HttpHeader data model and the fixed-size bounds every
 * parser enforces against it. These sizes bound memory use per request
 * and are the single source of truth for buffer limits elsewhere.
 */
#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H

#include <stddef.h>

#define HTTP_MAX_HEADERS 32
#define HTTP_MAX_HEADER_KEY_SIZE 64
#define HTTP_MAX_HEADER_VALUE_SIZE 1024

#define HTTP_MAX_REQUEST_SIZE 8192
#define HTTP_MAX_METHOD_SIZE 16
#define HTTP_MAX_TARGET_SIZE 2048
#define HTTP_MAX_VERSION_SIZE 16

typedef struct {
  char key[HTTP_MAX_HEADER_KEY_SIZE];
  char value[HTTP_MAX_HEADER_VALUE_SIZE];
} HttpHeader;

typedef struct {
  char method[HTTP_MAX_METHOD_SIZE];
  char target[HTTP_MAX_TARGET_SIZE];
  char version[HTTP_MAX_VERSION_SIZE];

  size_t header_count;
  HttpHeader headers[HTTP_MAX_HEADERS];
} HttpRequest;

#endif
