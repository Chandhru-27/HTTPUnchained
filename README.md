# HTTPUnchained

A small HTTP server written in C, being developed from a minimal TCP-socket
prototype toward a bounded, tested HTTP/1.1 server with request routing and JSON
support. The repository is currently an early learning-stage prototype; it is
not production-ready and does not yet implement the HTTP/1.1 feature set below.

## Target and specifications

The target is a plaintext HTTP/1.1 server over TCP, with request routing and
JSON request-body support. Protocol behavior is pinned to these specifications:

| Specification | Scope |
| --- | --- |
| [RFC 9110](https://www.rfc-editor.org/rfc/rfc9110) | HTTP semantics: methods, status codes, fields, and content semantics |
| [RFC 9112](https://www.rfc-editor.org/rfc/rfc9112) | HTTP/1.1 message syntax, framing, connections, and request parsing |
| [RFC 8259](https://www.rfc-editor.org/rfc/rfc8259) | JSON syntax and interoperability |

### In scope

- TCP server with bounded request/header/body sizes and configurable timeouts.
- HTTP/1.1 request parsing, required `Host` validation, `Content-Length` and
	chunked transfer coding, and rejection of ambiguous framing.
- Persistent connections, sequential request handling and pipelining-safe
	message boundaries, with correct partial-read and partial-write handling.
- Method/path routing, `HEAD`, `OPTIONS`, `405 Method Not Allowed` with `Allow`,
	and deliberate URI decoding and trailing-slash policies.
- JSON body parsing with UTF-8, size, and nesting-depth validation; appropriate
	`Content-Type` handling.
- Clear error responses, operational logging, graceful shutdown, and test,
	sanitizer, fuzz, and load-test workflows.

### Out of scope for this target

TLS, HTTP/2, HTTP/3/QUIC, WebSockets, proxying, load balancing, caching,
compression, and UDP application protocols. These can be considered as later,
separate projects after the HTTP/1.1 target is complete.

## Current implementation status

The current program is a single-connection TCP prototype, not yet a complete
HTTP server:

- `src/server.c` creates an IPv4 TCP listening socket on `0.0.0.0:8000`, enables
	`SO_REUSEADDR`, binds, listens, and accepts one client.
- `src/main.c` accepts one connection, receives a request, parses its request
	line, sends one response, then closes both sockets and exits.
- `src/http.c` reads until it sees the end-of-headers marker or fills its
	8 KiB-sized buffer. Its request-line parser recognizes method, target, and
	HTTP/1.0 or HTTP/1.1 version. It does not parse or validate header fields,
	message bodies, or framing.
- The response is always a fixed `200 OK` plain-text response with
	`Connection: close`; it is not selected by method or path.
- `include/config.h` currently sets port `8000` and listen backlog `10`.
	`include/http.h` defines fixed request-line field sizes and the 8192-byte
	request buffer limit.
- There are no router, JSON parser, automated unit tests, fuzz targets, CI
	configuration, or runtime configuration yet.

The Makefile provides `compile`, `scompile`, `run`, and `test` targets. The
`scompile` target enables C11 and common warning flags, but does not currently
make warnings fatal. The `test` target is only a netcat smoke request; it is
not an assertion-based test suite and requires the server to be running in
another terminal.

## Repository layout

```text
.
├── include/
│   ├── config.h       # Current port/backlog constants
│   ├── errors.h       # Current process and function status constants
│   ├── http.h         # Current request structure and HTTP function API
│   └── server.h       # Current socket function API
├── src/
│   ├── http.c         # Current request receive/line parse and fixed response
│   ├── main.c         # Current single-client server flow
│   └── server.c       # Current IPv4 socket setup, bind/listen/accept
├── output/            # Local compiled executable (ignored by Git)
├── Makefile           # Current build/run/smoke-test targets
└── README.md
```

As the implementation grows, keep modules focused. A likely evolution is to
add separate parser, router, JSON, and test modules rather than expanding the
current socket and HTTP files into one large implementation. Proposed modules
and responsibilities are listed in the sprint plan.

## Build and run the current prototype

### Requirements

- Linux or another POSIX-like environment with BSD sockets.
- GCC and GNU Make.
- `nc` (netcat) to use the existing smoke-test target.

From the repository root:

```sh
make scompile
make run
```

The server listens on port `8000` and handles exactly one client before exiting.
To exercise the current smoke target, start `make run` in one terminal and,
while it is waiting for a connection, run `make test` in another terminal.
`make test` sends a single `GET / HTTP/1.1` request; it does not verify the
response or run a test suite. The compiled executable is `output/a.out`.

## Development roadmap

Sprints are ordered by dependency and start from the code described in
[Current implementation status](#current-implementation-status). Exit criteria
should be demonstrated with tests, not just implemented.

| Sprint | Focus | Core modules / work | Exit criteria |
| --- | --- | --- | --- |
| 0 — Baseline and build | Make the prototype easy to build and inspect | `Makefile`, `include/config.h`, `src/main.c`; enable `-std=c11 -Wall -Wextra -Wpedantic -Werror`, deterministic clean/build targets, socket cleanup on every error path | Clean build with warnings treated as errors; documented repeatable build and smoke test |
| 1 — HTTP parser correctness | Replace request-line-only handling with bounded message parsing | New `http_parser` module/API; request line, field syntax, exactly one valid `Host`, CRLF rules, request-target and version handling; structured parse errors | Table-driven parser tests cover valid/malformed lines and headers; enforce request-line/header/count/total-size limits; return appropriate `400`, `414`, `431`, and `505` outcomes |
| 2 — Framing and connection I/O | Safely consume complete requests and send complete responses | `http_framing` and `io` helpers; `Content-Length`, chunked coding, conflicting framing rejection, partial I/O, `SIGPIPE` handling, read/write deadlines | Body framing tests including truncation and malformed chunks; correct `411`/`413` handling; no hangs on incomplete or oversized input |
| 3 — HTTP semantics and routing | Dispatch methods and paths and implement connection behavior | New `router` and `response` modules; route registration/matching, `404`, `405` plus `Allow`, `HEAD`, `OPTIONS`, keep-alive, sequential requests/pipelining boundaries | Route/method tests; curl tests for supported semantics; unsupported methods and versions receive deliberate status responses |
| 4 — JSON request support | Validate JSON endpoints and content types | New `json` module or a small maintained C JSON library; `Content-Type` checks, UTF-8 validation, nesting and size limits, parser error mapping | RFC 8259 valid/invalid fixtures; wrong media type yields `415`; invalid JSON yields `400`; boundary/depth tests pass |
| 5 — Hardening and runtime behavior | Defend resource use and shut down reliably | `config`, timeout/timer, logging, and signal-handling modules; idle/header/body timeouts, graceful `SIGTERM`, bounded resources, safe path decoding policy | Timeout, shutdown, malformed-input and resource-limit tests; ASan/UBSan clean; parser fuzzing runs without crashes or hangs |
| 6 — Production-like verification | Verify behavior across clients and sustained use | Unit/integration test harness, fuzz harnesses, CI, operational docs; real-client, differential, load, and soak testing | Test matrix below passes; no sanitizer findings/leaks; documented protocol scope and known limitations |

## Core module responsibilities

| Module | Responsibility | Current state |
| --- | --- | --- |
| Server/socket layer | Create, bind, listen, accept, and close TCP sockets | Basic IPv4, one-client implementation in `src/server.c` |
| HTTP parser | Parse request line and fields with strict bounds and report parse errors | Partial: request line only in `src/http.c` |
| Framing and I/O | Read complete message bodies and write responses despite partial I/O; implement framing and deadlines | Not implemented; receive currently stops at end of headers |
| Router | Match method and normalized path; distinguish 404 from 405 | Not implemented |
| JSON | Parse valid JSON and enforce UTF-8, body-size, and depth constraints | Not implemented |
| Response | Construct status line, headers, and body consistently; support HEAD and connection policy | Fixed plain-text 200 response only |
| Runtime/configuration | Timeouts, graceful shutdown, logging, explicit configurable limits | Constants only; no timeout or graceful shutdown handling |
| Tests/tooling | Unit, integration, fuzz, sanitizer, client, and load tests | Only a manual netcat smoke request |

## HTTP behavior and acceptance checklist

The eventual server must either implement or explicitly reject each relevant
case with a correct response. It must not silently accept malformed or
ambiguous requests.

| Area | Required checks |
| --- | --- |
| Request syntax | Valid request line and field syntax; reject malformed whitespace, invalid line endings, control characters, and CR/LF injection |
| Host and target | Require exactly one valid `Host` for HTTP/1.1; bound request-target length; test origin-form targets and invalid targets |
| Message framing | Valid `Content-Length`; valid chunked transfer coding; reject `Content-Length` plus `Transfer-Encoding`; detect truncated and overlong bodies |
| Methods and routing | Supported route; unknown path `404`; known path with unsupported method `405` and `Allow`; `HEAD` response without a body; `OPTIONS` behavior |
| Connections | Keep-alive semantics; multiple sequential requests; pipelined bytes remain correctly separated; close behavior is deterministic |
| Expectations and versions | `Expect: 100-continue`; unsupported expectation response; malformed expectation; unsupported HTTP version response |
| Limits | Oversized request line (`414`), headers (`431`), and body (`413`); missing required body length where applicable (`411`); timeout handling |
| JSON | Valid RFC 8259 values; invalid syntax and UTF-8; nesting and size boundaries; unsupported/missing media type (`415`) for JSON-only routes |
| Error behavior | Appropriate `400`, `404`, `405`, `411`, `413`, `414`, `415`, `431`, `501`, and `505` responses; errors do not leave the connection in an unsafe parser state |

Status codes are not a substitute for request validation: each response must
also have correct framing and connection behavior for the chosen scope.

## Test and verification plan

The current `make test` target is a smoke test only. Build the following suites
as part of the roadmap:

1. **Unit tests:** request-line, header, framing, target normalization, router,
	 JSON parser, and response serialization. Include valid, invalid, and
	 boundary-value tables.
2. **Integration tests:** launch the server on a test port and verify status,
	 headers, body length, connection behavior, multiple requests, malformed
	 messages, and timeout behavior. Prefer a script/harness that asserts results
	 rather than relying on visual inspection.
3. **Security and robustness tests:** parser fuzzing for HTTP and JSON, repeated
	 malformed requests, partial reads/writes, oversized fields/bodies, and
	 request-framing ambiguity. Run the test suite under AddressSanitizer and
	 UndefinedBehaviorSanitizer; use Valgrind where available.
4. **Client compatibility:** exercise requests with `curl` and a browser for
	 supported paths/methods. Compare selected valid and invalid cases against a
	 known HTTP implementation, accounting for the explicitly chosen scope.
5. **Load and soak tests:** measure throughput and latency, keep-alive behavior,
	 connection/resource ceilings, and stability during sustained requests.

### Definition of done

- **Correct:** the documented HTTP/1.1 subset and JSON behavior pass unit and
	integration tests, including message framing and error cases.
- **Hardened:** strict configurable limits and timeouts are enforced; fuzzing
	and malformed-input tests reveal no crashes or hangs; sanitizer runs are
	clean.
- **Production-like for the stated scope:** graceful shutdown, useful structured
	logs, explicit configuration, CI, client compatibility checks, and documented
	load/soak results are in place. This is a learning project, not a claim of
	production equivalence or a security certification.

## License

This project is distributed under the MIT License; see [LICENSE](LICENSE).