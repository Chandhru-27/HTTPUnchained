#include "http.h"

#include <stdio.h>
#include <string.h>
#include <sys/socket.h>

int http_receive_request(int clientsock_fd) {
  char buffer[1024];
  size_t total_received = 0;

  while (total_received < sizeof(buffer) - 1) {
    ssize_t bytes_read = recv(clientsock_fd, buffer + total_received,
                             sizeof(buffer) - total_received - 1, 0);

    if (bytes_read < 0) {
      perror("recv failed");
      return -1;
    }

    if (bytes_read == 0) {
      printf("Client disconnected\n");
      return -1;
    }

    total_received += (size_t)bytes_read;
    buffer[total_received] = '\0';

    if (strstr(buffer, "\r\n\r\n") != NULL) {
      printf("End of HTTP headers found!\n");
      break;
    }
  }

  if (strstr(buffer, "\r\n\r\n") == NULL) {
    fprintf(stderr, "Request headers exceed buffer capacity.\n");
    return -1;
  }

  printf("Received from client:\n%s\n", buffer);
  return 0;
}

int http_send_response(int clientsock_fd) {
  char* reply = "Message received loud and clear!\n";
  size_t total_sent = 0;
  size_t reply_length = strlen(reply);

  while (total_sent < reply_length) {
    ssize_t bytes_sent =
        send(clientsock_fd, reply + total_sent, strlen(reply) - total_sent, 0);

    if (bytes_sent < 0) {
      perror("send failed");
      return -1;
    }

    if (bytes_sent == 0) {
      fprintf(stderr, "send returned zero bytes.\n");
      return -1;
    }

    total_sent += (size_t)bytes_sent;
  }

  printf("Reply sent successfully.\n");
  return 0;
}