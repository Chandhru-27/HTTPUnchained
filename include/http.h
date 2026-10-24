#ifndef HTTP_H
#define HTTP_H

#include <stddef.h>
#include <stdint.h>
#include <unistd.h>

#define HTTP_MAX_HEADERS 32
#define HTTP_MAX_HEADER_KEY_SIZE 64
#define HTTP_MAX_HEADER_VALUE_SIZE 1024

#define HTTP_MAX_REQUEST_SIZE 8192
#define HTTP_MAX_METHOD_SIZE 16
#define HTTP_MAX_TARGET_SIZE 2048
#define HTTP_MAX_VERSION_SIZE 16

typedef enum {
  HTTP_PARSE_OK = 0,
  HTTP_PARSE_BAD_REQUEST,
  HTTP_PARSE_TARGET_TOO_LONG,
  HTTP_PARSE_HEADERS_TOO_LARGE,
  HTTP_PARSE_VERSION_UNSUPPORTED,
} HttpParseStatus_T;

typedef struct {
  HttpParseStatus_T status;
  uint32_t http_status_code;
  const char* description;
} HttpResult_T;

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

int8_t http_receive_request(int clientsock_fd, char* buffer, size_t capacity,
                            size_t* request_length);
HttpResult_T http_parse_request_line(const char* buffer, size_t request_length,
                                     HttpRequest* request);
HttpResult_T http_parse_headers(const char* buffer, size_t request_length,
                                HttpRequest* request);
const char* http_get_header(const HttpRequest* request, const char* key);
HttpResult_T http_validate_host(const HttpRequest* request);
int8_t http_send_response(int clientsock_fd);

HttpResult_T get_result_from_status(HttpParseStatus_T status);

#endif