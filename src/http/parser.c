/**
 * Implementation of request-line and header parsing declared in
 * http/parser.h, plus the private tokenizing helpers they rely on.
 */
#include "http/parser.h"

#include <ctype.h>
#include <string.h>

#include "http/status.h"

/**
 * @note This is a helper function and not declared in the source header.
 * @brief Strip leading/trailing whitespace from a string in place by
 *        advancing *str_ptr past leading whitespace and truncating
 *        trailing whitespace with a NUL.
 * @param str_ptr In/out pointer to the string to trim.
 */
static void trim(char** str_ptr) {
  while (isspace((unsigned char)**str_ptr)) {
    (*str_ptr)++;
  }

  if (**str_ptr == '\0') return;

  char* end = *str_ptr + strlen(*str_ptr) - 1;
  while (end > *str_ptr && isspace((unsigned char)*end)) {
    end--;
  }

  *(end + 1) = '\0';
}

/**
 * @note This is a helper function and not declared in the source header.
 * @brief Check whether a byte is a valid HTTP header-field token
 *        character (RFC 9110 tchar).
 * @param c The byte to check.
 * @return Non-zero if c is a valid token character.
 */
static int http_is_token_char(unsigned char c) {
  if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
      (c >= '0' && c <= '9')) {
    return 1;
  }

  return strchr("!#$%&'*+-.^_`|~", c) != NULL;
}

HttpResult_T http_parse_request_line(const char* buffer, size_t request_length,
                                     HttpRequest_T* request) {
  if (buffer == NULL || request == NULL || request_length == 0 ||
      request_length > HTTP_MAX_REQUEST_SIZE) {
    return get_result_from_status(HTTP_PARSE_BAD_REQUEST);
  }

  const char* line_end = strstr(buffer, "\r\n");
  if (line_end == NULL || (size_t)(line_end - buffer) >= request_length) {
    return get_result_from_status(HTTP_PARSE_BAD_REQUEST);
  }

  size_t line_length = (size_t)(line_end - buffer);
  char line[HTTP_MAX_REQUEST_SIZE + 1];

  if (line_length >= sizeof(line)) {
    return get_result_from_status(HTTP_PARSE_BAD_REQUEST);
  }

  memcpy(line, buffer, line_length);
  line[line_length] = '\0';

  char* line_ptr = line;
  trim(&line_ptr);

  if (line[0] == '\0' || line[0] == ' ' || line[0] == '\t')
    return get_result_from_status(HTTP_PARSE_BAD_REQUEST);

  char* token = strtok(line_ptr, " ");
  int fields = 0;
  char *method_tok = NULL, *target_tok = NULL, *version_tok = NULL;

  while (token != NULL) {
    fields++;

    if (fields == 1)
      method_tok = token;
    else if (fields == 2)
      target_tok = token;
    else if (fields == 3)
      version_tok = token;

    token = strtok(NULL, " ");
  }

  if (fields != 3 || method_tok == NULL || target_tok == NULL ||
      version_tok == NULL) {
    return get_result_from_status(HTTP_PARSE_BAD_REQUEST);
  }

  if (strlen(method_tok) >= sizeof(request->method)) {
    return get_result_from_status(HTTP_PARSE_BAD_REQUEST);
  }

  if (strlen(target_tok) >= sizeof(request->target)) {
    return get_result_from_status(HTTP_PARSE_TARGET_TOO_LONG);
  }

  if (strlen(version_tok) >= sizeof(request->version)) {
    return get_result_from_status(HTTP_PARSE_VERSION_UNSUPPORTED);
  }

  strcpy(request->method, method_tok);
  strcpy(request->target, target_tok);
  strcpy(request->version, version_tok);

  size_t target_length = strlen(request->target);
  for (size_t i = 0; i < target_length; i++) {
    unsigned char c = (unsigned char)request->target[i];
    if (c < 0x20 || c == 0x7F) {
      return get_result_from_status(HTTP_PARSE_BAD_REQUEST);
    }
  }

  if (strcmp(request->version, "HTTP/1.1") != 0 &&
      strcmp(request->version, "HTTP/1.0") != 0) {
    return get_result_from_status(HTTP_PARSE_VERSION_UNSUPPORTED);
  }

  return get_result_from_status(HTTP_PARSE_OK);
}

HttpResult_T http_parse_headers(const char* buffer, size_t request_length,
                                HttpRequest_T* request) {
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
