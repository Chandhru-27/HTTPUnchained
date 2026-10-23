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
int8_t http_parse_request_line(const char* buffer, size_t request_length,
                               HttpRequest* request);
int8_t http_parse_headers(const char* buffer, size_t request_length,
                          HttpRequest* request);
int8_t http_send_response(int clientsock_fd);

#endif