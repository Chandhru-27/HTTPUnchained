/**
 * Structured status reporting for HTTP request parsing and validation.
 * Every parsing/validation function returns an HttpResult_T so callers
 * can map a failure directly onto an HTTP response, instead of a bare
 * success/failure code that loses the reason.
 */
#ifndef HTTP_STATUS_H
#define HTTP_STATUS_H

#include <stdint.h>

typedef enum {
  HTTP_PARSE_OK = 0,
  HTTP_PARSE_BAD_REQUEST,
  HTTP_PARSE_TARGET_TOO_LONG,
  HTTP_PARSE_HEADERS_TOO_LARGE,
  HTTP_PARSE_VERSION_UNSUPPORTED,
} HttpParseStatus_T;

typedef struct {
  HttpParseStatus_T status;
  uint32_t http_status_code;
  const char* description;
} HttpResult_T;

/**
 * @brief Map a parse status onto its HTTP status code and description.
 * @param status The parse outcome to map.
 * @return An HttpResult_T with http_status_code/description filled in
 *         for the given status.
 */
HttpResult_T get_result_from_status(HttpParseStatus_T status);

#endif
