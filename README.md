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

  `http_parse_request_line()`    Implemented and tested  Extracts method, target,
                                                         and version via explicit
                                                         tokenizing; rejects
                                                         leading whitespace,
                                                         control characters, extra
                                                         fields, and maps oversized
                                                         targets to
                                                         `HTTP_PARSE_TARGET_TOO_LONG`.

  `http_parse_headers()`         Implemented and tested  Parses bounded header
                                                         names/values and rejects
                                                         malformed headers,
                                                         control characters, and
                                                         excess headers; returns
                                                         structured
                                                         `HttpResult_T` status.

  `http_get_header()`            Implemented             Case-insensitive lookup
                                                         returning the first
                                                         matching value. It cannot
                                                         detect duplicates; callers
                                                         needing duplicate
                                                         detection must scan all
                                                         headers.

  `http_validate_host()`         Implemented and tested  Checks HTTP/1.0 vs
                                                         HTTP/1.1 and, for
                                                         HTTP/1.1, requires exactly
                                                         one
                                                         non-empty/non-whitespace
                                                         `Host` value. Returns
                                                         structured `HttpResult_T`
                                                         status.

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

  Automated unit/integration     Implemented for Phase 1 `tests/test_http.c`
  tests                                                  covers request-line,
                                                         header, and Host
                                                         validation; run via
                                                         `make unit-test`. Framing,
                                                         router, and JSON coverage
                                                         remain.

  Sanitizers / fuzzing / load    Not started             Add repeatable
  testing / CI                                           verification workflows.
  ---------------------------------------------------------------------------------

### Phase 1 test coverage

`tests/test_http.c` is an assertion-based suite (run with `make
unit-test`) covering:

-   Valid request lines (`HTTP/1.1` and `HTTP/1.0`).
-   Malformed request lines: missing version, extra fields, missing
    CRLF, bare LF, empty line.
-   Request-line grammar: leading whitespace before the method,
    control characters in the target, and oversized targets mapped to
    `HTTP_PARSE_TARGET_TOO_LONG` rather than a generic bad request.
-   Unsupported/garbage HTTP versions.
-   Valid headers, case-insensitive lookup, and value trimming.
-   Malformed headers: missing colon, empty header name, control
    characters in a value, missing CRLF terminator.
-   Excess headers rejected once past `HTTP_MAX_HEADERS`.
-   `http_validate_host()`: missing/duplicate/whitespace-only `Host`
    for HTTP/1.1, single valid `Host` accepted, and HTTP/1.0 without
    `Host` accepted.

The basic response was also exercised with `curl -v`, and request
reception was manually tested with `nc`.

## Current implementation details

The current program accepts clients in a loop, handling one HTTP
exchange per connection:

-   `src/net/socket.c` creates an IPv4 TCP listening socket on
    `0.0.0.0:8000`, enables `SO_REUSEADDR`, binds, listens, and accepts
    client connections.
-   `src/main.c` is the process entry point: it sets up the server
    socket, then loops forever accepting a client, handing it to
    `handle_connection()`, and closing it. It has no HTTP-protocol
    knowledge of its own.
-   `src/server/connection.c` runs one request/response cycle on an
    already-accepted client socket: receive, parse the request line,
    parse the headers, look up `Host`, send the response.
-   `src/http/` splits request handling by responsibility:
    `response.c` (receive/send wire I/O), `parser.c` (request-line and
    header parsing), `headers.c` (header lookup), `host.c` (Host
    validation), `status.c` (parse-status to HTTP-status mapping).
-   `http_receive_request()` may read beyond `\r\n\r\n` into the same
    buffer. Future body/framing code must not discard those extra bytes.
-   The response is currently a basic `200 OK` plain-text response with
    `Connection: close`; it is not selected by method or path.
-   Each accepted connection is still a fresh TCP connection handling
    exactly one request; there is no HTTP keep-alive or pipelining yet
    (Phase 3).
-   `include/core/config.h` currently sets port `8000` and listen
    backlog `10`.
-   `include/http/request.h` defines the `HttpRequest` / `HttpHeader`
    structures and the buffer-size macros every parser enforces
    against them.
-   There is no router, JSON parser, fuzz target, CI configuration, or
    runtime configuration yet.

The Makefile provides `compile`, `scompile`, `run`, `test`, and
`unit-test` targets. The `scompile` target enables C11 and common
warning flags, but warnings are not necessarily treated as errors. The
`test` target is a netcat smoke request, not an assertion-based test
suite, and requires the server to be running in another terminal.
`unit-test` builds and runs `tests/test_http.c`, the assertion-based
suite covering Phase 1 request-line, header, and Host validation.

## Code style and documentation

-   **Formatting:** C source is formatted with the Google C/C++ style
    (`clang-format -style=google`), 2-space indentation, 80-column
    guideline. Run it on changed files before committing.
-   **Naming:** `snake_case` for functions and variables,
    `UpperCamelCase` with a `_T` suffix for typedef'd types
    (`HttpResult_T`), `UPPER_SNAKE_CASE` for macros/constants.
-   **File-level comments:** every `.h`/`.c` file opens with a short
    block comment describing what the file is responsible for — not a
    changelog, not a restatement of every function, just the single
    responsibility the file exists to cover.
-   **Function-level comments:** every function declared in a header
    gets a Doxygen-style comment (`@brief`, `@param`, `@return`) at its
    declaration; the matching `.c` definition is not re-commented.
    Static helpers local to a single `.c` file are documented the same
    way at their definition, since they have no header declaration.
-   **No inline narration:** avoid one-line comments scattered through
    function bodies explaining *what* a line does — well-named
    variables and functions should make that unnecessary. A comment
    earns its place only when it explains a non-obvious *why* (a
    workaround, an invariant, a spec citation).
-   **One responsibility per file:** a `.c` file implements one
    cohesive piece of behavior (e.g. "request-line and header
    parsing", "Host validation") and its paired `.h` declares exactly
    that file's public API — not a catch-all declaring everything the
    module happens to contain. Internal helpers (e.g. `trim()` in
    `parser.c`) stay `static` and undeclared in any header.

## Project structure

``` text
.
├── include/
│   ├── core/
│   │   ├── config.h        # Port and backlog constants
│   │   └── errors.h        # Process/function status constants
│   ├── net/
│   │   └── socket.h        # Socket lifecycle API (create/bind/listen/accept)
│   ├── http/
│   │   ├── status.h        # HttpParseStatus_T, HttpResult_T, status mapping
│   │   ├── request.h       # HttpRequest / HttpHeader structs and size macros
│   │   ├── parser.h        # Request-line and header parsing API
│   │   ├── headers.h       # Header lookup API
│   │   ├── host.h          # Host-header validation API
│   │   └── response.h      # Request reception / response sending API
│   └── server/
│       └── connection.h    # Per-connection request/response cycle API
├── src/
│   ├── net/
│   │   └── socket.c
│   ├── http/
│   │   ├── status.c
│   │   ├── parser.c        # Includes internal trim()/http_is_token_char() helpers
│   │   ├── headers.c
│   │   ├── host.c
│   │   └── response.c
│   ├── server/
│   │   └── connection.c
│   └── main.c               # Entry point: socket setup + accept loop
├── tests/
│   └── test_http.c         # Phase 1 unit tests (request line, headers, Host)
├── output/                 # Local compiled executable (ignored by Git)
├── Makefile                # Build/run/smoke-test/unit-test targets
└── README.md
```

Each module (`net/`, `http/`, `server/`) mirrors between `include/` and
`src/`: a `.c` file's declarations live at the same relative path under
`include/`, included as `"http/parser.h"` etc. (the compiler's include
path is `include/`, set via `-Iinclude`). As the implementation grows,
keep this one-file-one-responsibility pattern rather than letting any
single file accumulate unrelated concerns again.

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

### Phase 1 --- Request validation (complete)

-   [x] `http_receive_request()` --- bounded initial request reception.
-   [x] `http_parse_request_line()` --- parse method, target, and
    version.
-   [x] `http_parse_headers()` --- parse header fields with basic syntax
    and bounds.
-   [x] `http_get_header()` --- case-insensitive lookup of the first
    matching field.
-   [x] `http_validate_host()` --- basic HTTP/1.0/1.1 Host
    presence/uniqueness/empty-value checks; **dedicated tests pending**.
-   [x] Add tests for valid/missing/duplicate/empty/whitespace-only
    `Host` fields and unsupported versions.
-   [x] Add complete request-line, header grammar, CRLF,
    control-character, and length/count boundary tests.
-   [x] Return structured parser/validation errors so the server can
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
