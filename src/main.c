/**
 * Process entry point. Owns server-socket setup and the top-level
 * accept loop; all HTTP-protocol knowledge lives in
 * server/connection.c and below. Each accepted client is handled by
 * one call to handle_connection() and then closed; the server itself
 * keeps accepting new clients until it is killed.
 */
#include <unistd.h>

#include "core/errors.h"
#include "net/socket.h"
#include "server/connection.h"

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

  while (1) {
    int clientsock_fd = net_create_clientsock(serversock_fd);

    if (clientsock_fd < 0) {
      continue;
    }

    handle_connection(clientsock_fd);
    close(clientsock_fd);
  }
}
