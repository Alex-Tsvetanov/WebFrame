---
title: HTTP-2
tags:
  - coroute
  - http2
  - protocol
  - hpack
aliases:
  - HTTP 2
  - H2 Support
---

# HTTP-2

> [!abstract]
> Coroute contains a dedicated HTTP/2 module with connection, stream, frame, and HPACK support. In the current runtime, HTTP/2 is enabled through TLS/ALPN when built with `COROUTE_HAS_HTTP2`.

## Main module layout

Relevant files:

- `[[include/coroute/http2/connection.hpp]]`
- `[[include/coroute/http2/frame.hpp]]`
- `[[include/coroute/http2/hpack.hpp]]`
- `[[include/coroute/http2/stream.hpp]]`
- `[[src/http2/connection.cpp]]`
- `[[src/http2/frame.cpp]]`
- `[[src/http2/hpack.cpp]]`
- `[[src/http2/stream.cpp]]`

## Implementation Details

### 1. ALPN Selection
Coroute leverages the TLS **ALPN (Application-Layer Protocol Negotiation)** extension to decide between HTTP/1.1 and HTTP/2.
- **Priority**: If `http2_enabled_` is set, Coroute lists `{"h2", "http/1.1"}` in order of preference.
- **Branching**: After the TLS handshake, `App` inspects the negotiated protocol. If it is `"h2"`, the connection is wrapped in an `Http2Connection`.

### 2. HTTP/2 Connection & Streams
An `Http2Connection` acts as a multiplexer. 
- **Binary Frames**: Unlike the text-based HTTP/1.1, HTTP/2 uses binary frames (DATA, HEADERS, SETTINGS, etc.).
- **Multiplexing**: Multiple streams (requests/responses) can be active simultaneously over a single TCP connection, eliminating Head-of-Line blocking at the application level.
- **HPACK**: Headers are compressed using the HPACK algorithm, which uses a dynamic table to avoid sending redundant headers.

### 3. h2c (Cleartext) Upgrade
While most production traffic uses HTTPS/h2, Coroute also supports **h2c** (HTTP/2 over cleartext) via the `Upgrade` header mechanism.
- **Process**: The client sends a special HTTP/1.1 request with `Upgrade: h2c`.
- **Switching**: If supported, the server responds with `101 Switching Protocols` and immediately begins the HTTP/2 preface exchange.

Relevant files:
- `[[src/core/app.cpp]]`
- `[[include/coroute/http2/connection.hpp]]`
- `[[include/coroute/net/tls.hpp]]`

## Major feature areas visible in code

### Connection management

The connection layer owns the HTTP/2 session and coordinates stream handling.

Relevant files:

- `[[include/coroute/http2/connection.hpp]]`
- `[[src/http2/connection.cpp]]`

### Frames

The frame layer defines HTTP/2 frame headers, serialization, and parsing helpers.

Relevant files:

- `[[include/coroute/http2/frame.hpp]]`
- `[[src/http2/frame.cpp]]`

### HPACK

The HPACK layer provides header compression/decompression and header validation utilities.

Relevant files:

- `[[include/coroute/http2/hpack.hpp]]`
- `[[src/http2/hpack.cpp]]`

### Streams

The stream layer models per-stream state and flow inside a connection.

Relevant files:

- `[[include/coroute/http2/stream.hpp]]`
- `[[src/http2/stream.cpp]]`

## Build-time enablement

HTTP/2 is optional at build time.

Observed build behavior in `[[CMakeLists.txt]]`:

- controlled by `COROUTE_ENABLE_HTTP2`
- fetches and links nghttp2
- adds the HTTP/2 source files to the build
- defines `COROUTE_HAS_HTTP2` when enabled

## Example usage

- `[[examples/Samples/http2_server/main.cpp]]`
- `[[README.md]]`

## Testing and implementation evidence

The strongest current protocol-specific test evidence I found is here:

- `[[tests/http2_tests.cpp]]`
- `[[tests/test_http2_advanced.cpp]]`

Observed test coverage areas include:

- frame-header serialization and parsing
- SETTINGS, PING, GOAWAY, WINDOW_UPDATE, RST_STREAM, DATA, and HEADERS frames
- HPACK encode/decode and validation utilities
- HTTP/2 preface detection
- some advanced-feature placeholders such as flow-control related behavior

> [!note]
> `test_http2_advanced.cpp` suggests that some advanced behaviors are recognized but not exhaustively validated in the current test suite.

## Status

### Current status

- **Implemented**: dedicated HTTP/2 module with connection/frame/stream/HPACK layers.
- **Implemented**: ALPN-based runtime selection into the HTTP/2 path.
- **Test-backed**: substantial dedicated coverage exists in `[[tests/http2_tests.cpp]]`.
- **Partially validated in advanced areas**: `[[tests/test_http2_advanced.cpp]]` contains advanced-feature coverage, but some sections explicitly acknowledge that certain behaviors are hard to test in the current form.

## Related notes

- [[Protocols/Protocols Index]]
- [[Architecture/Server Runtime and OS Backends]]
- [[Protocols/TLS]]
- [[Protocols/HTTP-1.1]]
