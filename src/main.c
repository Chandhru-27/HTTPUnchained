#include <stdio.h>

#include "server.h"

int main() {
  int serversock_fd = net_create_serversock();

  if (serversock_fd < 0) {
    return 1;
  }

  if (net_bind_serversock(serversock_fd) < 0) {
    return 1;
  }

  if (net_listen_on_serversock(serversock_fd) < 0) {
    return 1;
  }

  if (net_create_clientsock(serversock_fd) < 0) {
    return 1;
  }

  return 0;
}