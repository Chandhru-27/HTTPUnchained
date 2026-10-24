# HTTPUnchained

HTTPUnchained is a small HTTP server written in C, being developed from
a minimal TCP-socket prototype into a bounded, tested HTTP/1.1 server
with request routing and JSON request-body support.

> **Project status:** Early learning-stage implementation. The current
> server is not production-ready and does not yet implement the full
> HTTP/1.1 target described below. Progress labels distinguish code that
> exists from behavior verified by tests.

## Target and specifications

The target is a plaintext HTTP/1.1 server over TCP, with request routing
and JSON request-body support. Protocol behavior is guided by these
specifications:

  -----------------------------------------------------------------------------------
  Specification                                   Scope
  ----------------------------------------------- -----------------------------------
  [RFC                                            HTTP semantics: methods, status
  9110](https://www.rfc-editor.org/rfc/rfc9110)   codes, fields, and content
                                                  semantics

  [RFC                                            HTTP/1.1 message syntax, framing,
  9112](https://www.rfc-editor.org/rfc/rfc9112)   connections, and request parsing

  [RFC                                            JSON syntax and interoperability
  8259](https://www.rfc-editor.org/rfc/rfc8259)   
  -----------------------------------------------------------------------------------

### In scope

-   TCP server with bounded request, header, and body sizes and
    configurable timeouts.
-   HTTP/1.1 request parsing, required `Host` validation,
    `Content-Length` and chunked transfer coding, and rejection of
    ambiguous framing.
-   Persistent connections, sequential request handling, and
    pipelining-safe message boundaries, with correct partial-read and
    partial-write handling.
-   Method/path routing, `HEAD`, `OPTIONS`, `405 Method Not Allowed`
    with `Allow`, and deliberate URI decoding and trailing-slash
    policies.
-   JSON body parsing with UTF-8, size, and nesting-depth validation,
    plus appropriate `Content-Type` handling.
-   Clear error responses, operational logging, graceful shutdown, and
    test, sanitizer, fuzz, and load-test workflows.

### Out of scope for this target

TLS, HTTP/2, HTTP/3/QUIC, WebSockets, proxying, load balancing, caching,
compression, and UDP application protocols. These may be considered as
separate projects after the HTTP/1.1 target is complete.

## Progress dashboard

Use this table as the current project tracker. "Implemented" means code
has been written; it does not imply full RFC compliance or that all
listed edge cases have been tested.

  ---------------------------------------------------------------------------------
  Area / API                     Status                  Notes / remaining work
  ------------------------------ ----------------------- --------------------------
  `net_create_serversock()`      Implemented and         Creates the IPv4 TCP
                                 manually exercised      listening socket.

  `net_bind_serversock()`        Implemented and         Binds to the configured
                                 manually exercised      port; `SO_REUSEADDR` is
                                                         enabled.

  `net_listen_on_serversock()`   Implemented and         Starts listening with
                                 manually exercised      configured backlog.

  `net_create_clientsock()`      Implemented and         Accepts one client
                                 manually exercised      connection. No
                                                         multi-client loop yet.

  `http_receive_request()`       Implemented and         Bounded receive into a
                                 manually exercised      buffer; reads until the
                                                         header terminator or the
                                                         buffer/read condition
                                                         ends. Must preserve any
                                                         body or subsequent-request
                                                         bytes already read.

  `http_parse_request_line()`    Implemented and         Extracts method, target,
                                 manually exercised      and version; strict
                                                         grammar and complete
                                                         malformed-input coverage
                                                         remain to be verified.

  `http_parse_headers()`         Implemented; six manual Parses bounded header
                                 cases passed            names/values and rejects
                                                         malformed headers and
                                                         excess headers. Full
                                                         protocol semantics and
                                                         structured error mapping
                                                         remain.

  `http_get_header()`            Implemented             Case-insensitive lookup
                                                         returning the first
                                                         matching value. It cannot
                                                         detect duplicates; callers
                                                         needing duplicate
                                                         detection must scan all
                                                         headers.

  `http_validate_host()`         Implemented; tests      Checks HTTP/1.0 vs
                                 pending                 HTTP/1.1 and, for
                                                         HTTP/1.1, requires exactly
                                                         one
                                                         non-empty/non-whitespace
                                                         `Host` value. Full
                                                         authority syntax
                                                         validation and proper HTTP
                                                         error mapping remain.

  `http_send_response()`         Implemented and         Sends a basic `200 OK`
                                 manually exercised      text response with
                                                         `Content-Length` and
                                                         `Connection: close`;
                                                         response selection and
                                                         complete HTTP semantics
                                                         are not implemented.

  `Content-Length` validation    Not started             Detect duplicates, invalid
                                                         decimal syntax, overflow,
                                                         and framing conflicts.

  `Transfer-Encoding` validation Not started             Validate coding rules,
  / chunked decoding                                     decode chunks, enforce
                                                         limits, and reject
                                                         ambiguous framing.

  Request-body reader / framing  Not started             Consume exactly one body,
  API                                                    handle truncation and
                                                         partial reads, and
                                                         preserve bytes after the
                                                         current message.

  Header semantic validation     Not started             Add relevant checks such
  beyond `Host`                                          as `Content-Type`,
                                                         `Expect`, and connection
                                                         semantics as scope
                                                         requires.

  Router and method semantics    Not started             Route
                                                         registration/matching,
                                                         `404`, `405` plus `Allow`,
                                                         `HEAD`, and `OPTIONS`.

  JSON support                   Not started             RFC 8259 parsing, UTF-8,
                                                         content type, size, and
                                                         nesting-depth limits.

  Persistent connections /       Not started             Sequential requests,
  pipelining                                             deterministic connection
                                                         policy, and safe message
                                                         boundaries.

  Structured HTTP error          Not started             Map parser/validation
  responses                                              failures to appropriate
                                                         statuses such as `400`,
                                                         `411`, `413`, `414`,
                                                         `415`, `431`, and `505`.

  Timeouts / graceful shutdown / Not started or partial  Add explicit runtime
  logging                                                policy, bounded resource
                                                         use, useful logs, and
                                                         signal-driven shutdown.

  Automated unit/integration     Not started             Current checks are manual;
  tests                                                  build assertion-based
                                                         tests.

  Sanitizers / fuzzing / load    Not started             Add repeatable
  testing / CI                                           verification workflows.
  ---------------------------------------------------------------------------------

### Manual tests completed so far

The user reports that these six header-parser cases passed:

-   Multiple valid headers.
-   Header-name capitalization is preserved as received.
-   Missing colon is rejected.
-   Empty header name is rejected.
-   Leading/trailing spaces and tabs around a header value are trimmed.
-   Too many headers are rejected.

The basic response was also exercised with `curl -v`, and request
reception / parsing was manually tested with `nc`. The
`http_validate_host()` function has been written, but its dedicated test
matrix still needs to be run.

## Current implementation details

The current program is a single-connection TCP prototype:

-   `src/server.c` creates an IPv4 TCP listening socket on
    `0.0.0.0:8000`, enables `SO_REUSEADDR`, binds, listens, and accepts
    one client.
-   `src/main.c` accepts one connection, receives a request, parses the
    request line and headers, prints parsed data, sends one response,
    then closes both sockets and exits.
-   `src/http.c` contains request reception, request-line parsing,
    header parsing, header lookup, Host validation, and basic response
    sending.
-   `http_receive_request()` may read beyond `\r\n\r\n` into the same
    buffer. Future body/framing code must not discard those extra bytes.
-   The response is currently a basic `200 OK` plain-text response with
    `Connection: close`; it is not selected by method or path.
-   `include/config.h` currently sets port `8000` and listen backlog
    `10`.
-   `include/http.h` defines the `HttpRequest` / `HttpHeader` structures
    and HTTP API declarations. Keep the header-name field consistent
    (`key` or `name`) across the struct, parser, lookup, validator, and
    logging code.
-   There is no router, JSON parser, automated unit-test suite, fuzz
    target, CI configuration, or runtime configuration yet.

The Makefile provides `compile`, `scompile`, `run`, and `test` targets.
The `scompile` target enables C11 and common warning flags, but warnings
are not necessarily treated as errors. The `test` target is a netcat
smoke request, not an assertion-based test suite, and requires the
server to be running in another terminal.

## Repository layout

``` text
.
├── include/
│   ├── config.h       # Port and backlog constants
│   ├── errors.h       # Current process/function status constants
│   ├── http.h         # Request structures and HTTP API declarations
│   └── server.h       # Socket API declarations
├── src/
│   ├── http.c         # Receive, parsing, lookup, Host validation, response
│   ├── main.c         # Single-client server flow
│   └── server.c       # IPv4 socket setup, bind/listen/accept
├── output/            # Local compiled executable (ignored by Git)
├── Makefile           # Build/run/smoke-test targets
└── README.md
```

As the implementation grows, keep modules focused. Likely future modules
include framing/I/O, routing, JSON, and automated tests rather than
expanding the current source files into one large implementation.

## Build and run the current prototype

### Requirements

-   Linux or another POSIX-like environment with BSD sockets.
-   GCC and GNU Make.
-   `nc` (netcat) for the existing smoke-test target.
-   `curl` is useful for inspecting responses.

From the repository root:

``` sh
make scompile
make run
```

The server listens on port `8000` and handles exactly one client before
exiting. Start `make run` in one terminal, then run `make test` in
another while the server is waiting for a connection. The existing
`make test` target sends one `GET / HTTP/1.1` request; it does not
verify the response or run a test suite. The compiled executable is
`output/a.out`.

Example manual request:

``` sh
printf 'GET /products/42 HTTP/1.1\r\nHost: localhost:8000\r\n\r\n' \
  | nc -N 127.0.0.1 8000
```

## API implementation tracker

This is the intended sequence of APIs and behavior to implement to
approach HTTP/1.1 readiness. Names are proposed where the API does not
exist yet; adjust names to match the project's conventions.

### Phase 1 --- Request validation

-   [x] `http_receive_request()` --- bounded initial request reception.
-   [x] `http_parse_request_line()` --- parse method, target, and
    version.
-   [x] `http_parse_headers()` --- parse header fields with basic syntax
    and bounds.
-   [x] `http_get_header()` --- case-insensitive lookup of the first
    matching field.
-   [x] `http_validate_host()` --- basic HTTP/1.0/1.1 Host
    presence/uniqueness/empty-value checks; **dedicated tests pending**.
-   [ ] Add tests for valid/missing/duplicate/empty/whitespace-only
    `Host` fields and unsupported versions.
-   [ ] Add complete request-line, header grammar, CRLF,
    control-character, and length/count boundary tests.
-   [ ] Return structured parser/validation errors so the server can
    send deliberate HTTP responses.

### Phase 2 --- Framing and request-body I/O

-   [ ] `http_get_header_count()` or equivalent duplicate-aware header
    inspection helper, if useful.
-   [ ] `http_validate_content_length()` --- validate decimal syntax,
    duplicates, conflicting values, and integer overflow.
-   [ ] `http_validate_transfer_encoding()` --- parse transfer-coding
    values and enforce supported coding rules.
-   [ ] `http_validate_framing()` --- reject ambiguous framing,
    including `Content-Length` together with `Transfer-Encoding`.
-   [ ] `http_read_content_length_body()` --- read exactly the declared
    body length under a configured size limit.
-   [ ] `http_decode_chunked_body()` --- decode chunk sizes, chunk data,
    CRLF delimiters, and trailers with limits.
-   [ ] Preserve bytes already read after the header terminator; do not
    assume one `recv()` corresponds to one HTTP message.
-   [ ] Add read/write loops for partial I/O, interrupted system calls,
    deadlines, and safe handling of broken connections / `SIGPIPE`.

### Phase 3 --- HTTP semantics and routing

-   [ ] `http_validate_request_target()` --- validate supported target
    forms and enforce target length.
-   [ ] `router_register()` / `router_match()` or equivalent ---
    register method/path handlers.
-   [ ] Build response helpers for status line, headers, body length,
    and connection policy.
-   [ ] Implement `404 Not Found`, `405 Method Not Allowed` with
    `Allow`, `HEAD`, and `OPTIONS`.
-   [ ] Define URI percent-decoding and trailing-slash policies.
-   [ ] Implement persistent connections, sequential request processing,
    and pipelining-safe message boundaries.

### Phase 4 --- JSON request support

-   [ ] Select a small maintained C JSON parser or implement a clearly
    scoped parser if that is an explicit learning goal.
-   [ ] Validate `Content-Type` for JSON-only endpoints.
-   [ ] Enforce body size, UTF-8, and nesting-depth limits.
-   [ ] Map invalid JSON to `400 Bad Request` and unsupported media
    types to `415 Unsupported Media Type`.
-   [ ] Add valid/invalid RFC 8259 fixtures and boundary tests.

### Phase 5 --- Hardening and runtime behavior

-   [ ] Add configurable request/header/body limits and idle/header/body
    timeouts.
-   [ ] Add operational logging and graceful shutdown on
    `SIGTERM`/`SIGINT`.
-   [ ] Ensure descriptors and request buffers are cleaned up on every
    error path.
-   [ ] Add AddressSanitizer and UndefinedBehaviorSanitizer builds; use
    Valgrind where available.
-   [ ] Add HTTP and JSON fuzz harnesses and ensure malformed input does
    not crash or hang the server.

### Phase 6 --- Verification

-   [ ] Unit tests for parser, framing, router, JSON, and response
    serialization.
-   [ ] Integration tests that assert status codes, headers, bodies,
    connection behavior, and timeouts.
-   [ ] Tests for partial reads/writes, truncated bodies, oversized
    fields/bodies, malformed chunks, and ambiguous framing.
-   [ ] `curl` and browser checks for supported behavior.
-   [ ] Load/soak tests for latency, throughput, connection/resource
    limits, and sustained stability.
-   [ ] CI workflow for build and automated tests.

## HTTP/1.1 acceptance checklist

The eventual server must implement or explicitly reject relevant cases
with a correct response. It must not silently accept malformed or
ambiguous requests.

  -----------------------------------------------------------------------
  Area                                Required checks
  ----------------------------------- -----------------------------------
  Request syntax                      Valid request line and field
                                      syntax; reject malformed
                                      whitespace, invalid line endings,
                                      prohibited control characters, and
                                      CR/LF injection

  Host and target                     Exactly one valid `Host` for
                                      HTTP/1.1; bounded request-target
                                      length; test origin-form and
                                      invalid targets

  Message framing                     Valid `Content-Length`; valid
                                      chunked transfer coding; reject
                                      `Content-Length` plus
                                      `Transfer-Encoding`; detect
                                      truncated and oversized bodies

  Methods and routing                 Supported route; unknown path
                                      `404`; known path with unsupported
                                      method `405` and `Allow`; `HEAD`
                                      without a response body; deliberate
                                      `OPTIONS` behavior

  Connections                         Keep-alive semantics; multiple
                                      sequential requests; pipelined
                                      bytes remain correctly separated;
                                      deterministic close behavior

  Expectations and versions           `Expect: 100-continue`; unsupported
                                      expectation handling; malformed
                                      expectation; unsupported HTTP
                                      version response

  Limits                              Oversized request line (`414`),
                                      headers (`431`), and body (`413`);
                                      missing required body length where
                                      applicable (`411`); timeout
                                      handling

  JSON                                Valid RFC 8259 values; invalid
                                      syntax and UTF-8; nesting and size
                                      boundaries; unsupported/missing
                                      media type (`415`) for JSON-only
                                      routes

  Error behavior                      Appropriate `400`, `404`, `405`,
                                      `411`, `413`, `414`, `415`, `417`,
                                      `431`, `501`, and `505` responses,
                                      with correct response framing and
                                      connection behavior
  -----------------------------------------------------------------------

Status codes alone are not sufficient: every error response must have
correct framing and a deliberate connection policy.

## Test and verification plan

The current `make test` target is a smoke test only. The roadmap should
add:

1.  **Unit tests:** request line, headers, framing, target validation,
    router, JSON, and response serialization. Include valid, invalid,
    and boundary-value cases.
2.  **Integration tests:** launch the server on a test port and assert
    status, headers, body length, connection behavior, multiple
    requests, malformed messages, and timeouts.
3.  **Security and robustness tests:** fuzz HTTP and JSON parsers; test
    repeated malformed requests, partial reads/writes, oversized
    fields/bodies, and framing ambiguity. Run tests under ASan/UBSan and
    Valgrind where available.
4.  **Client compatibility:** use `curl` and a browser for supported
    paths/methods; compare selected cases against a known HTTP
    implementation while accounting for the documented scope.
5.  **Load and soak tests:** measure throughput, latency, keep-alive
    behavior, connection/resource ceilings, and sustained stability.

## Definition of done

-   **Correct:** the documented HTTP/1.1 subset and JSON behavior pass
    unit and integration tests, including framing and error cases.
-   **Hardened:** configurable limits and timeouts are enforced; fuzzing
    and malformed-input tests reveal no crashes or hangs; sanitizer runs
    are clean.
-   **Production-like for the stated scope:** graceful shutdown, useful
    logs, explicit configuration, CI, client compatibility checks, and
    documented load/soak results are in place.

This remains a learning project, not a claim of production equivalence
or a security certification.

## License

This project is distributed under the MIT License; see
[`LICENSE`](LICENSE).
