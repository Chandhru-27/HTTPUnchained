#include <stdio.h>
#include <unistd.h>

#include "http.h"
#include "server.h"

int main() {
  int serversock_fd = net_create_serversock();

  if (serversock_fd < 0) {
    return 1;
  }

  if (net_bind_serversock(serversock_fd) < 0) {
    close(serversock_fd);
    return 1;
  }

  if (net_listen_on_serversock(serversock_fd) < 0) {
    close(serversock_fd);
    return 1;
  }

  int clientsock_fd = net_create_clientsock(serversock_fd);

  if (clientsock_fd < 0) {
    close(serversock_fd);
    return 1;
  }

  char raw_request[HTTP_MAX_REQUEST_SIZE + 1];
  size_t request_length = 0;
  if (http_receive_request(clientsock_fd, raw_request, sizeof(raw_request),
                           &request_length) < 0) {
    close(clientsock_fd);
    close(serversock_fd);
    return 1;
  }

  if (http_send_response(clientsock_fd) < 0) {
    close(clientsock_fd);
    close(serversock_fd);
    return 1;
  }

  close(clientsock_fd);
  close(serversock_fd);
  return 0;
}