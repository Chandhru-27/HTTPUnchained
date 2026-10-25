/**
 * Parsing of the raw request-line and header bytes received from a
 * client into a validated HttpRequest. Both functions assume `buffer`
 * holds at least one full request (as produced by
 * http_receive_request()) and only inspect the first `request_length`
 * bytes.
 */
#ifndef HTTP_PARSER_H
#define HTTP_PARSER_H

#include <stddef.h>

#include "http/request.h"
#include "http/status.h"

/**
 * @brief Parse the request line (method, target, version) from the
 *        start of a raw HTTP request.
 * @param buffer Raw request bytes, not required to be NUL-terminated.
 * @param request_length Number of valid bytes in buffer.
 * @param request Output; method/target/version are filled in on
 *        success.
 * @return HTTP_PARSE_OK on success; HTTP_PARSE_BAD_REQUEST,
 *         HTTP_PARSE_TARGET_TOO_LONG, or HTTP_PARSE_VERSION_UNSUPPORTED
 *         on failure.
 */
HttpResult_T http_parse_request_line(const char* buffer, size_t request_length,
                                     HttpRequest* request);

/**
 * @brief Parse the header section following the request line into
 *        request->headers.
 * @param buffer Raw request bytes, not required to be NUL-terminated.
 * @param request_length Number of valid bytes in buffer.
 * @param request Output; header_count/headers are filled in on
 *        success.
 * @return HTTP_PARSE_OK on success; HTTP_PARSE_BAD_REQUEST or
 *         HTTP_PARSE_HEADERS_TOO_LARGE on failure.
 */
HttpResult_T http_parse_headers(const char* buffer, size_t request_length,
                                HttpRequest* request);

#endif
