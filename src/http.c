#include "http.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

#include "errors.h"

int http_receive_request(int clientsock_fd, char* buffer, size_t capacity,
                         size_t* request_length) {
  if (buffer == NULL || request_length == NULL || capacity < 4)
    return STATUS_FATAL_ERROR;

  buffer[0] = '\0';
  *request_length = 0;

  size_t total_received = 0;
  while (total_received < capacity - 1) {
    ssize_t bytes_read = recv(clientsock_fd, buffer + total_received,
                              capacity - total_received - 1, 0);

    if (bytes_read < 0) {
      if (errno == EINTR) continue;

      perror("recv failed");
      return STATUS_FATAL_ERROR;
    }

    if (bytes_read == 0) {
      printf("Client disconnected before headers completed.\n");
      return STATUS_FATAL_ERROR;
    }

    total_received += (size_t)bytes_read;
    buffer[total_received] = '\0';

    if (strstr(buffer, "\r\n\r\n") != NULL) {
      *request_length = total_received;
      printf("Received from client:\n%s\n", buffer);
      return STATUS_EXEC_SUCCESS;
    }
  }

  fprintf(stderr, "Request headers exceed buffer capacity.\n");
  return STATUS_FATAL_ERROR;
}

int http_parse_request_line(const char* buffer, size_t request_length,
                            HttpRequest* request) {
  if (buffer == NULL || request == NULL || request_length == 0 ||
      request_length > HTTP_MAX_REQUEST_SIZE)
    return STATUS_FATAL_ERROR;

  const char* line_end = strstr(buffer, "\r\n");

  if (line_end == NULL || (size_t)(line_end - buffer) >= request_length)
    return STATUS_FATAL_ERROR;

  size_t line_length = (size_t)(line_end - buffer);
  char line[HTTP_MAX_REQUEST_SIZE + 1];

  if (line_length >= sizeof(line)) {
    return STATUS_FATAL_ERROR;
  }

  memcpy(line, buffer, line_length);
  line[line_length] = '\0';

  char extra;
  int fields = sscanf(line, "%15s %2047s %15s %c", request->method,
                      request->target, request->version, &extra);

  if (fields != 3) {
    return STATUS_FATAL_ERROR;
  }

  if (strcmp(request->version, "HTTP/1.1") != 0 &&
      strcmp(request->version, "HTTP/1.0") != 0) {
    return STATUS_FATAL_ERROR;
  }

  return STATUS_EXEC_SUCCESS;
}

int http_send_response(int clientsock_fd) {
  const char* body = "Message received loud and clear!\n";
  size_t body_length = strlen(body);

  char headers[256];
  ssize_t header_length = snprintf(headers, sizeof(headers),
                                   "HTTP/1.1 200 OK\r\n"
                                   "Content-Type: text/plain; charset=utf-8\r\n"
                                   "Content-Length: %zu\r\n"
                                   "Connection: close\r\n"
                                   "\r\n",
                                   body_length);

  if (header_length < 0 || (size_t)header_length >= sizeof(headers)) {
    fprintf(stderr, "Failed to construct HTTP headers.\n");
    return STATUS_FATAL_ERROR;
  }

  size_t total_sent = 0;
  while (total_sent < (size_t)header_length) {
    ssize_t bytes_sent = send(clientsock_fd, headers + total_sent,
                              (size_t)header_length - total_sent, 0);

    if (bytes_sent < 0) {
      perror("Headers: send failed");
      return STATUS_FATAL_ERROR;
    }

    if (bytes_sent == 0) {
      fprintf(stderr, "Headers: send returned zero bytes.\n");
      return STATUS_FATAL_ERROR;
    }

    total_sent += (size_t)bytes_sent;
  }

  total_sent = 0;
  while (total_sent < body_length) {
    ssize_t bytes_sent =
        send(clientsock_fd, body + total_sent, body_length - total_sent, 0);

    if (bytes_sent < 0) {
      perror("Body: send failed");
      return STATUS_FATAL_ERROR;
    }

    if (bytes_sent == 0) {
      fprintf(stderr, "Body: send returned zero bytes.\n");
      return STATUS_FATAL_ERROR;
    }

    total_sent += (size_t)bytes_sent;
  }

  printf("Reply sent successfully.\n");
  return STATUS_EXEC_SUCCESS;
}