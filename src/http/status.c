/**
 * Implementation of the parse-status-to-HTTP-result mapping declared
 * in http/status.h.
 */
#include "http/status.h"

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
