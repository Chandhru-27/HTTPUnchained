/**
 * Lookup helpers over an already-parsed HttpRequest_T's headers.
 */
#ifndef HTTP_HEADERS_H
#define HTTP_HEADERS_H

#include "http/request.h"

/**
 * @brief Case-insensitively find the first header matching `key`.
 * @param request The parsed request to search.
 * @param key The header name to look up.
 * @return The matching header's value, or NULL if absent or if a
 *         duplicate-sensitive caller needs to scan all headers
 *         directly.
 */
const char* http_get_header(const HttpRequest_T* request, const char* key);

#endif
