#include "server.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>

#include "config.h"
#include "errors.h"

int net_create_serversock(void) {
  int serversock_fd = socket(AF_INET, SOCK_STREAM, 0);

  if (serversock_fd < 0) {
    perror("Socket creation failed!");
    return STATUS_FATAL_ERROR;
  }

  printf("Socket succesfully created with descriptior: %d\n", serversock_fd);

  int opt = 1;
  if (setsockopt(serversock_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) <
      0) {
    perror("setsocketopt SO_REUSEADDR failed");
    return STATUS_FATAL_ERROR;
  }

  printf("Port reuse enabled successfully.\n");
  return serversock_fd;
}

int net_bind_serversock(int serversock_fd) {
  struct sockaddr_in server_address = {0};
  server_address.sin_family = AF_INET;
  server_address.sin_addr.s_addr = htonl(INADDR_ANY);
  server_address.sin_port = htons(PORT);

  printf("The address blueprint is filled out and ready to use!\n");

  if (bind(serversock_fd, (struct sockaddr*)&server_address,
           sizeof(server_address)) < 0) {
    perror("Bind failed! The port might already be in use.");
    return STATUS_FATAL_ERROR;
  }

  printf("Successfully bound to port %d!\n", PORT);
  return STATUS_EXEC_SUCCESS;
}

int net_listen_on_serversock(int serversock_fd) {
  if (listen(serversock_fd, BACKLOG) < 0) {
    perror("Listen failed!");
    return STATUS_FATAL_ERROR;
  }

  printf(
      "Server is now listening on port %d... Waiting room queue set to %d.\n",
      PORT, BACKLOG);
  return STATUS_EXEC_SUCCESS;
}

int net_create_clientsock(int serversock_fd) {
  struct sockaddr_in client_address = {0};

  socklen_t client_addrlen = sizeof(client_address);
  int clientsock_fd =
      accept(serversock_fd, (struct sockaddr*)&client_address, &client_addrlen);

  if (clientsock_fd < 0) {
    perror("Accept failed!");
    return STATUS_FATAL_ERROR;
  }

  printf("Client connected: %s:%d\n", inet_ntoa(client_address.sin_addr),
         ntohs(client_address.sin_port));
  return clientsock_fd;
}