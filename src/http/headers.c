/**
 * Implementation of the header-lookup helper declared in
 * http/headers.h.
 */
#include "http/headers.h"

#include <strings.h>

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
