/**
 * Implementation of the per-connection request/response cycle declared
 * in server/connection.h.
 */
#include "server/connection.h"

#include <stdio.h>

#include "core/errors.h"
#include "http/headers.h"
#include "http/parser.h"
#include "http/request.h"
#include "http/response.h"
#include "http/status.h"

int handle_connection(int clientsock_fd) {
  char raw_request[HTTP_MAX_REQUEST_SIZE + 1];
  size_t request_length = 0;

  if (http_receive_request(clientsock_fd, raw_request, sizeof(raw_request),
                           &request_length) < 0) {
    return STATUS_FATAL_ERROR;
  }

  HttpRequest request = {0};
  if (http_parse_request_line(raw_request, request_length, &request).status !=
      HTTP_PARSE_OK) {
    return STATUS_FATAL_ERROR;
  }

  printf("Method: %s\nTarget: %s\nVersion: %s\n", request.method,
         request.target, request.version);

  if (http_parse_headers(raw_request, request_length, &request).status !=
      HTTP_PARSE_OK) {
    return STATUS_FATAL_ERROR;
  }

  for (size_t i = 0; i < request.header_count; i++) {
    printf("Header: %s: %s\n", request.headers[i].key,
           request.headers[i].value);
  }

  const char* value = http_get_header(&request, "HoST");
  if (value == NULL) {
    return STATUS_FATAL_ERROR;
  }

  printf("%s: %s\n", "HOST", value);

  if (http_send_response(clientsock_fd) < 0) {
    return STATUS_FATAL_ERROR;
  }

  return STATUS_EXEC_SUCCESS;
}
