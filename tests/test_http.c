/**
 * Phase 1 unit tests: request-line parsing, header parsing, and Host
 * validation. Run via `make unit-test`.
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>

#include "http/headers.h"
#include "http/host.h"
#include "http/parser.h"
#include "http/request.h"
#include "http/status.h"

static int tests_run = 0;
static int tests_failed = 0;

#define CHECK(name, condition)                        \
  do {                                                \
    tests_run++;                                      \
    if (condition) {                                  \
      printf("PASS: %s\n", name);                     \
    } else {                                          \
      printf("FAIL: %s (line %d)\n", name, __LINE__); \
      tests_failed++;                                 \
    }                                                 \
  } while (0)

static HttpParseStatus_T parse_line(const char* input, HttpRequest_T* request) {
  return http_parse_request_line(input, strlen(input), request).status;
}

static HttpParseStatus_T parse_headers(const char* input,
                                       HttpRequest_T* request) {
  return http_parse_headers(input, strlen(input), request).status;
}

static void test_request_line_valid(void) {
  HttpRequest_T r = {0};
  CHECK("GET / HTTP/1.1 parses ok",
        parse_line("GET / HTTP/1.1\r\n", &r) == HTTP_PARSE_OK);
  CHECK("method captured", strcmp(r.method, "GET") == 0);
  CHECK("target captured", strcmp(r.target, "/") == 0);
  CHECK("version captured", strcmp(r.version, "HTTP/1.1") == 0);

  HttpRequest_T r2 = {0};
  CHECK("HTTP/1.0 accepted",
        parse_line("POST /submit HTTP/1.0\r\n", &r2) == HTTP_PARSE_OK);
}

static void test_request_line_malformed(void) {
  HttpRequest_T r = {0};
  CHECK("missing version is bad request",
        parse_line("GET /\r\n", &r) == HTTP_PARSE_BAD_REQUEST);

  CHECK("extra field is bad request",
        parse_line("GET / HTTP/1.1 extra\r\n", &r) == HTTP_PARSE_BAD_REQUEST);

  CHECK("no CRLF is bad request",
        parse_line("GET / HTTP/1.1", &r) == HTTP_PARSE_BAD_REQUEST);

  CHECK("bare LF without CR is bad request",
        parse_line("GET / HTTP/1.1\n", &r) == HTTP_PARSE_BAD_REQUEST);

  CHECK("empty line is bad request",
        parse_line("\r\n", &r) == HTTP_PARSE_BAD_REQUEST);
}

static void test_request_line_grammar(void) {
  HttpRequest_T r = {0};
  CHECK("leading whitespace before method rejected",
        parse_line(" GET / HTTP/1.1\r\n", &r) == HTTP_PARSE_BAD_REQUEST);

  HttpRequest_T r2 = {0};
  CHECK("control character in target rejected",
        parse_line("GET /\x01 HTTP/1.1\r\n", &r2) == HTTP_PARSE_BAD_REQUEST);

  HttpRequest_T r3 = {0};
  char long_target[HTTP_MAX_TARGET_SIZE + 100];
  long_target[0] = '/';
  memset(long_target + 1, 'a', sizeof(long_target) - 2);
  long_target[sizeof(long_target) - 1] = '\0';

  char line[HTTP_MAX_REQUEST_SIZE] = "GET ";
  strcat(line, long_target);
  strcat(line, " HTTP/1.1\r\n");

  CHECK("oversized target reported as target-too-long, not bad-request",
        parse_line(line, &r3) == HTTP_PARSE_TARGET_TOO_LONG);
}

static void test_request_line_version(void) {
  HttpRequest_T r = {0};
  CHECK("unsupported version rejected",
        parse_line("GET / HTTP/2.0\r\n", &r) == HTTP_PARSE_VERSION_UNSUPPORTED);

  HttpRequest_T r2 = {0};
  CHECK("garbage version rejected",
        parse_line("GET / FOO\r\n", &r2) == HTTP_PARSE_VERSION_UNSUPPORTED);
}

static void test_headers_valid(void) {
  HttpRequest_T r = {0};
  const char* input =
      "GET / HTTP/1.1\r\n"
      "Host: example.com\r\n"
      "X-Custom:   value with spaces  \r\n"
      "\r\n";

  CHECK("valid headers parse ok", parse_headers(input, &r) == HTTP_PARSE_OK);
  CHECK("header count is 2", r.header_count == 2);
  CHECK("host value correct",
        strcmp(http_get_header(&r, "Host"), "example.com") == 0);
  CHECK("header lookup is case-insensitive",
        strcmp(http_get_header(&r, "hOsT"), "example.com") == 0);
  CHECK("value is trimmed",
        strcmp(http_get_header(&r, "X-Custom"), "value with spaces") == 0);
}

static void test_headers_malformed(void) {
  HttpRequest_T r = {0};
  CHECK("missing colon rejected",
        parse_headers("GET / HTTP/1.1\r\nHostexample.com\r\n\r\n", &r) ==
            HTTP_PARSE_BAD_REQUEST);

  HttpRequest_T r2 = {0};
  CHECK("empty header name rejected",
        parse_headers("GET / HTTP/1.1\r\n: value\r\n\r\n", &r2) ==
            HTTP_PARSE_BAD_REQUEST);

  HttpRequest_T r3 = {0};
  CHECK("control character in value rejected",
        parse_headers("GET / HTTP/1.1\r\nX: bad\x01value\r\n\r\n", &r3) ==
            HTTP_PARSE_BAD_REQUEST);

  HttpRequest_T r4 = {0};
  CHECK("header line without CRLF terminator rejected",
        parse_headers("GET / HTTP/1.1\r\nHost: example.com", &r4) ==
            HTTP_PARSE_BAD_REQUEST);
}

static void test_headers_limits(void) {
  char input[HTTP_MAX_REQUEST_SIZE] = "GET / HTTP/1.1\r\n";
  for (int i = 0; i < HTTP_MAX_HEADERS + 1; i++) {
    char line[64];
    snprintf(line, sizeof(line), "H%d: v\r\n", i);
    strcat(input, line);
  }
  strcat(input, "\r\n");

  HttpRequest_T r = {0};
  CHECK("too many headers rejected",
        parse_headers(input, &r) == HTTP_PARSE_HEADERS_TOO_LARGE);
}

static void test_validate_host(void) {
  HttpRequest_T r = {0};
  strcpy(r.version, "HTTP/1.1");
  CHECK("http/1.1 missing host rejected",
        http_validate_host(&r).status == HTTP_PARSE_BAD_REQUEST);

  HttpRequest_T r2 = {0};
  strcpy(r2.version, "HTTP/1.1");
  strcpy(r2.headers[0].key, "Host");
  strcpy(r2.headers[0].value, "example.com");
  r2.header_count = 1;
  CHECK("http/1.1 single host accepted",
        http_validate_host(&r2).status == HTTP_PARSE_OK);

  HttpRequest_T r3 = {0};
  strcpy(r3.version, "HTTP/1.1");
  strcpy(r3.headers[0].key, "Host");
  strcpy(r3.headers[0].value, "example.com");
  strcpy(r3.headers[1].key, "Host");
  strcpy(r3.headers[1].value, "other.com");
  r3.header_count = 2;
  CHECK("http/1.1 duplicate host rejected",
        http_validate_host(&r3).status == HTTP_PARSE_BAD_REQUEST);

  HttpRequest_T r4 = {0};
  strcpy(r4.version, "HTTP/1.1");
  strcpy(r4.headers[0].key, "Host");
  strcpy(r4.headers[0].value, "   ");
  r4.header_count = 1;
  CHECK("http/1.1 whitespace-only host rejected",
        http_validate_host(&r4).status == HTTP_PARSE_BAD_REQUEST);

  HttpRequest_T r5 = {0};
  strcpy(r5.version, "HTTP/1.0");
  CHECK("http/1.0 missing host accepted",
        http_validate_host(&r5).status == HTTP_PARSE_OK);
}

int main(void) {
  test_request_line_valid();
  test_request_line_malformed();
  test_request_line_grammar();
  test_request_line_version();
  test_headers_valid();
  test_headers_malformed();
  test_headers_limits();
  test_validate_host();

  printf("\n%d/%d tests passed\n", tests_run - tests_failed, tests_run);
  return tests_failed == 0 ? 0 : 1;
}
