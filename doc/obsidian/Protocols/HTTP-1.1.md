---
title: HTTP-1.1
tags:
  - coroute
  - http
  - http1
  - protocol
aliases:
  - HTTP 1.1
  - Default HTTP Path
---

# HTTP-1.1

> [!abstract]
> HTTP/1.1 is the default request/response path in Coroute. Even when optional protocols such as HTTP/2 or WebSocket are available, the framework's baseline server behavior still starts from the HTTP/1.1 listener, request parser, router, middleware chain, and response writer.

## Main role in Coroute

HTTP/1.1 is the baseline protocol path used when:

- TLS/ALPN does not switch the connection into HTTP/2
- the request is not upgraded to WebSocket
- the application is serving ordinary browser or API traffic

Relevant files:

- `[[include/coroute/core/app.hpp]]`
- `[[src/core/app.cpp]]`
- `[[include/coroute/core/request.hpp]]`
- `[[include/coroute/core/response.hpp]]`

## Lifecycle in practice

Observed runtime stages:

1. accept connection
2. parse HTTP request
3. match route
4. execute middleware chain
5. call handler or not-found fallback
6. send response
7. optionally keep the connection alive

Relevant files:

- `[[src/core/app.cpp]]`
- `[[include/coroute/net/io_context.hpp]]`
- `[[include/coroute/core/router.hpp]]`
- `[[src/core/router.cpp]]`

## Features visible from the current code

### Request/response core

Coroute has first-class abstractions for:

- request method, path, query, body, and headers
- response status, headers, and body
- JSON and HTML response helpers
- route parameter extraction

Relevant files:

- `[[include/coroute/core/request.hpp]]`
- `[[include/coroute/core/response.hpp]]`
- `[[include/coroute/core/router.hpp]]`

### Keep-alive behavior

The main runtime path in `app.cpp` manages connection persistence and updates `Connection` / `Keep-Alive` headers based on the per-connection request count and shutdown state.

Relevant files:

- `[[src/core/app.cpp]]`

### View routes over HTTP/1.1

View routes share the same transport path, but can return either:

- rendered HTML for browsers
- `{ templates, model }` JSON for Flutter-identified requests

Relevant files:

- `[[src/core/app.cpp]]`
- `[[include/coroute/core/app.hpp]]`
- `[[include/coroute/view/view_types.hpp]]`

### Middleware support

Cross-cutting concerns such as logging, auth, and compression run through the same HTTP/1.1 path.

Relevant files:

- `[[include/coroute/core/app.hpp]]`
- `[[src/core/compression.cpp]]`
- `[[README.md]]`

## Example usage sites

- `[[examples/Samples/hello_world/main.cpp]]`
- `[[examples/Project/src/app/server.cpp]]`
- `[[examples/FlutterProject/src/app/server.cpp]]`

## Testing and evidence

During this pass, I found strong coverage around the pieces HTTP/1.1 depends on:

- routing: `[[tests/test_router.cpp]]`
- responses: `[[tests/test_response.cpp]]`
- chunked/static/range support: `[[tests/test_chunked.cpp]]`, `[[tests/test_static_files.cpp]]`, `[[tests/test_range.cpp]]`
- shared view contract: `[[tests/test_view.cpp]]`
- compression middleware: `[[tests/test_compression.cpp]]`

I did not find a dedicated file named specifically for HTTP/1.1 integration tests during this pass.

## Status

### Current status

- **Implemented**: HTTP/1.1 is the default request/response path in the server runtime.
- **Implemented**: request parsing, routing, middleware execution, keep-alive handling, and view/API dispatch all hang off this path.
- **Indirectly test-backed**: many supporting behaviors are tested even though I did not find a dedicated `http1`-named test file.

## Related notes

- [[Protocols/Protocols Index]]
- [[Architecture/Server Runtime and OS Backends]]
- [[Architecture/View and API Abstractions]]
- [[Protocols/WebSocket]]
- [[Protocols/TLS]]
