#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

#include "errors.h"
#include "http.h"
#include "server.h"

int main(void) {
  int serversock_fd = net_create_serversock();

  if (serversock_fd < 0) {
    return STATUS_EXIT_FAILURE;
  }

  if (net_bind_serversock(serversock_fd) < 0) {
    close(serversock_fd);
    return STATUS_EXIT_FAILURE;
  }

  if (net_listen_on_serversock(serversock_fd) < 0) {
    close(serversock_fd);
    return STATUS_EXIT_FAILURE;
  }

  int clientsock_fd = net_create_clientsock(serversock_fd);

  if (clientsock_fd < 0) {
    close(serversock_fd);
    return STATUS_EXIT_FAILURE;
  }

  char raw_request[HTTP_MAX_REQUEST_SIZE + 1];
  size_t request_length = 0;
  if (http_receive_request(clientsock_fd, raw_request, sizeof(raw_request),
                           &request_length) < 0) {
    close(clientsock_fd);
    close(serversock_fd);
    return STATUS_EXIT_FAILURE;
  }

  HttpRequest request = {0};
  if (http_parse_request_line(raw_request, request_length, &request) < 0) {
    close(clientsock_fd);
    close(serversock_fd);
    return STATUS_EXIT_FAILURE;
  }

  printf("Method: %s\nTarget: %s\nVersion: %s\n", request.method,
         request.target, request.version);

  if (http_parse_headers(raw_request, request_length, &request) < 0) {
    close(clientsock_fd);
    close(serversock_fd);
    return STATUS_EXIT_FAILURE;
  }

  for (size_t i = 0; i < request.header_count; i++) {
    printf("Header: %s: %s\n", request.headers[i].key,
           request.headers[i].value);
  }

  if (http_send_response(clientsock_fd) < 0) {
    close(clientsock_fd);
    close(serversock_fd);
    return STATUS_EXIT_FAILURE;
  }

  close(clientsock_fd);
  close(serversock_fd);
  return STATUS_EXIT_SUCCESS;
}