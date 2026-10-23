#ifndef HTTP_H
#define HTTP_H

#include <stddef.h>

#define HTTP_MAX_REQUEST_SIZE 8192
#define HTTP_MAX_METHOD_SIZE 16
#define HTTP_MAX_TARGET_SIZE 2048
#define HTTP_MAX_VERSION_SIZE 16

typedef struct {
  char method[HTTP_MAX_METHOD_SIZE];
  char target[HTTP_MAX_TARGET_SIZE];
  char version[HTTP_MAX_VERSION_SIZE];
} HttpRequest;

int http_receive_request(int clientsock_fd, char* buffer, size_t capacity,
                         size_t* request_length);
int http_parse_request_line(const char* buffer, size_t request_length,
                            HttpRequest* request);
int http_send_response(int clientsock_fd);

#endif