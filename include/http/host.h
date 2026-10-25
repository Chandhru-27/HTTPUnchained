/**
 * Host-header validation per HTTP version, as required by RFC 9112.
 */
#ifndef HTTP_HOST_H
#define HTTP_HOST_H

#include "http/request.h"
#include "http/status.h"

/**
 * @brief Validate the Host header against the request's HTTP version.
 *        HTTP/1.1 requires exactly one non-empty, non-whitespace-only
 *        Host value; HTTP/1.0 does not require Host at all.
 * @param request The parsed request to validate.
 * @return HTTP_PARSE_OK on success; HTTP_PARSE_BAD_REQUEST or
 *         HTTP_PARSE_VERSION_UNSUPPORTED on failure.
 */
HttpResult_T http_validate_host(const HttpRequest_T* request);

#endif
