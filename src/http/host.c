/**
 * Implementation of Host-header validation declared in http/host.h.
 */
#include "http/host.h"

#include <string.h>
#include <strings.h>

#include "http/headers.h"
#include "http/status.h"

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
