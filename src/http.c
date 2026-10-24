#include "http.h"

#include <ctype.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>

#include "errors.h"

HttpResult_T get_result_from_status(HttpParseStatus_T status) {
  HttpResult_T result;
  result.status = status;

  switch (status) {
    case HTTP_PARSE_OK:
      result.http_status_code = 200;
      result.description = "OK";
      break;

    case HTTP_PARSE_BAD_REQUEST:
      result.http_status_code = 400;
      result.description = "Bad Request";
      break;

    case HTTP_PARSE_TARGET_TOO_LONG:
      result.http_status_code = 414;
      result.description = "URI Too Long";
      break;

    case HTTP_PARSE_HEADERS_TOO_LARGE:
      result.http_status_code = 431;
      result.description = "Request Header Fields Too Large";
      break;

    case HTTP_PARSE_VERSION_UNSUPPORTED:
      result.http_status_code = 505;
      result.description = "HTTP Version Not Supported";
      break;

    default:
      result.http_status_code = 500;
      result.description = "Internal Server Error";
      break;
  }

  return result;
}

int8_t http_receive_request(int clientsock_fd, char* buffer, size_t capacity,
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

HttpResult_T http_parse_request_line(const char* buffer, size_t request_length,
                                     HttpRequest* request) {
  if (buffer == NULL || request == NULL || request_length == 0 ||
      request_length > HTTP_MAX_REQUEST_SIZE)
    return get_result_from_status(HTTP_PARSE_BAD_REQUEST);

  const char* line_end = strstr(buffer, "\r\n");

  if (line_end == NULL || (size_t)(line_end - buffer) >= request_length)
    return get_result_from_status(HTTP_PARSE_BAD_REQUEST);

  size_t line_length = (size_t)(line_end - buffer);
  char line[HTTP_MAX_REQUEST_SIZE + 1];

  if (line_length >= sizeof(line)) {
    return get_result_from_status(HTTP_PARSE_TARGET_TOO_LONG);
  }

  memcpy(line, buffer, line_length);
  line[line_length] = '\0';

  if (line[0] == '\0' || line[0] == '\t')
    return get_result_from_status(HTTP_PARSE_BAD_REQUEST);

  char extra;
  int fields = sscanf(line, "%15s %2047s %15s %c", request->method,
                      request->target, request->version, &extra);

  if (fields != 3) {
    return get_result_from_status(HTTP_PARSE_BAD_REQUEST);
  }

  if (strcmp(request->version, "HTTP/1.1") != 0 &&
      strcmp(request->version, "HTTP/1.0") != 0) {
    return get_result_from_status(HTTP_PARSE_VERSION_UNSUPPORTED);
  }

  return get_result_from_status(HTTP_PARSE_OK);
}

static int http_is_token_char(unsigned char c) {
  if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
      (c >= '0' && c <= '9')) {
    return 1;
  }

  return strchr("!#$%&'*+-.^_`|~", c) != NULL;
}

HttpResult_T http_parse_headers(const char* buffer, size_t request_length,
                                HttpRequest* request) {
  if (buffer == NULL || request == NULL || request_length == 0 ||
      request_length > HTTP_MAX_REQUEST_SIZE) {
    return get_result_from_status(HTTP_PARSE_BAD_REQUEST);
  }

  request->header_count = 0;

  const char* request_line_end = memchr(buffer, '\r', request_length);

  if (request_line_end == NULL ||
      request_line_end + 1 >= buffer + request_length ||
      request_line_end[1] != '\n') {
    return get_result_from_status(HTTP_PARSE_BAD_REQUEST);
  }

  size_t offset = (size_t)(request_line_end - buffer) + 2;

  while (offset < request_length) {
    const char* line = buffer + offset;
    size_t remaining = request_length - offset;

    const char* line_end = memchr(line, '\r', remaining);

    if (line_end == NULL || line_end + 1 >= buffer + request_length ||
        line_end[1] != '\n') {
      return get_result_from_status(HTTP_PARSE_BAD_REQUEST);
    }

    size_t line_length = (size_t)(line_end - line);

    if (line_length == 0) {
      return get_result_from_status(HTTP_PARSE_OK);
    }

    if (request->header_count >= HTTP_MAX_HEADERS) {
      return get_result_from_status(HTTP_PARSE_HEADERS_TOO_LARGE);
    }

    const char* colon = memchr(line, ':', line_length);

    if (colon == NULL || colon == line) {
      return get_result_from_status(HTTP_PARSE_BAD_REQUEST);
    }

    size_t key_length = (size_t)(colon - line);

    if (key_length >= HTTP_MAX_HEADER_KEY_SIZE) {
      return get_result_from_status(HTTP_PARSE_HEADERS_TOO_LARGE);
    }

    for (size_t i = 0; i < key_length; i++) {
      if (!http_is_token_char((unsigned char)line[i])) {
        return get_result_from_status(HTTP_PARSE_BAD_REQUEST);
      }
    }

    const char* value = colon + 1;
    const char* header_end = line + line_length;

    while (value < header_end && (*value == ' ' || *value == '\t')) value++;

    while (header_end > value &&
           (header_end[-1] == ' ' || header_end[-1] == '\t'))
      header_end--;

    size_t value_length = (size_t)(header_end - value);

    if (value_length >= HTTP_MAX_HEADER_VALUE_SIZE) {
      return get_result_from_status(HTTP_PARSE_HEADERS_TOO_LARGE);
    }

    for (size_t i = 0; i < value_length; i++) {
      unsigned char c = (unsigned char)value[i];

      if ((c < 32 && c != '\t') || c == 127) {
        return get_result_from_status(HTTP_PARSE_BAD_REQUEST);
      }
    }

    size_t index = request->header_count;

    memcpy(request->headers[index].key, line, key_length);
    request->headers[index].key[key_length] = '\0';

    memcpy(request->headers[index].value, value, value_length);
    request->headers[index].value[value_length] = '\0';

    request->header_count++;

    offset += line_length + 2;
  }

  return get_result_from_status(HTTP_PARSE_OK);
}

const char* http_get_header(const HttpRequest* request, const char* key) {
  if (request == NULL || key == NULL || key[0] == '\0') return NULL;

  size_t size = request->header_count;

  if (size == 0 || size > HTTP_MAX_HEADERS) return NULL;

  for (size_t i = 0; i < size; i++) {
    if (strcasecmp(request->headers[i].key, key) == 0) {
      return request->headers[i].value;
    }
  }

  return NULL;
}

HttpResult_T http_validate_host(const HttpRequest* request) {
  if (request == NULL || request->header_count > HTTP_MAX_HEADERS) {
    return get_result_from_status(HTTP_PARSE_BAD_REQUEST);
  }

  if (strcmp(request->version, "HTTP/1.0") == 0) {
    const char* host = http_get_header(request, "Host");

    if (host == NULL) {
      return get_result_from_status(HTTP_PARSE_OK);
    }

    return get_result_from_status(HTTP_PARSE_OK);
  }

  if (strcmp(request->version, "HTTP/1.1") != 0) {
    return get_result_from_status(HTTP_PARSE_VERSION_UNSUPPORTED);
  }

  size_t host_count = 0;
  const char* host_value = NULL;

  for (size_t i = 0; i < request->header_count; i++) {
    if (strcasecmp(request->headers[i].key, "Host") == 0) {
      host_count++;
      host_value = request->headers[i].value;
    }
  }

  if (host_count != 1) {
    return get_result_from_status(HTTP_PARSE_BAD_REQUEST);
  }

  const char* p = host_value;

  while (*p == ' ' || *p == '\t') {
    p++;
  }

  if (*p == '\0') {
    return get_result_from_status(HTTP_PARSE_BAD_REQUEST);
  }

  return get_result_from_status(HTTP_PARSE_OK);
}

int8_t http_send_response(int clientsock_fd) {
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