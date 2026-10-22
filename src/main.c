#include <stdio.h>
#include <stdint.h>
#include <unistd.h>

#include "http.h"
#include "server.h"
#include "errors.h"

int8_t main() {
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

  char raw_request[HTTP_MAX_REQUEST_SIZE + STATUS_EXIT_FAILURE];
  size_t request_length = 0;
  if (http_receive_request(clientsock_fd, raw_request, sizeof(raw_request),
                           &request_length) < 0) {
    close(clientsock_fd);
    close(serversock_fd);
    return STATUS_EXIT_FAILURE;
  }

  HttpRequest *request = {0};
  if (http_parse_request_line((const char*)&raw_request, request_length, request) < 0) {
    close(clientsock_fd);
    close(serversock_fd);
    return STATUS_EXIT_FAILURE;
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