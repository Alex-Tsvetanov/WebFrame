---
title: Server Runtime and OS Backends
tags:
  - coroute
  - runtime
  - networking
  - backends
aliases:
  - Runtime and Backends
  - IoContext Architecture
---

# Server Runtime and OS Backends

> [!abstract]
> This note describes the runtime path from `App::run()` down into the operating-system specific event loops. It covers how Coroute accepts connections, routes work into detached coroutines, and hides Linux, macOS, and Windows backend differences behind `IoContext`.

## Runtime entry point

The server/runtime story starts in `App`.

Relevant files:

- `[[include/coroute/core/app.hpp]]`
- `[[src/core/app.cpp]]`

The same class can run in two broad modes:

- **server mode** via `run(RunOptions{.port = ...})`
- **client/embedded mode** via `run(RunOptions{.api_domain = ...})`

This note focuses on server mode, but the shared runtime machinery is reused by the embedded path.

## HTTP request lifecycle

At a high level, the request path is:

1. create or select the `IoContext`
2. create a listener
3. accept a connection
4. start `handle_connection(...)` as a detached coroutine
5. parse the request
6. route and execute middleware/handler logic
7. write the response
8. repeat for keep-alive when applicable

Relevant files:

- `[[src/core/app.cpp]]`
- `[[include/coroute/core/app.hpp]]`
- `[[include/coroute/net/io_context.hpp]]`
- `[[doc/v2/en/chapters/04_architecture.tex]]`

## `IoContext` as the abstraction boundary

`IoContext` is the interface that isolates the rest of Coroute from the OS-specific event loop implementation.

Core responsibilities exposed by the interface:

- run and stop the event loop
- post callbacks back into the loop
- schedule delayed work
- optionally enable multi-accept mode

Related files:

- `[[include/coroute/net/io_context.hpp]]`
- `[[src/net/event_loop.cpp]]`
- `[[src/net/socket.cpp]]`

## Platform-specific backends

The concrete backend is selected in `[[CMakeLists.txt]]`.

### Linux

- backend: `io_uring`
- implementation: `[[src/net/io_uring/uring_context.cpp]]`
- selected when `COROUTE_IO_BACKEND` is `io_uring`

### macOS

- backend: `kqueue`
- implementation: `[[src/net/kqueue/kqueue_context.cpp]]`
- selected when `COROUTE_IO_BACKEND` is `kqueue`

### Windows

- backend: `IOCP`
- implementation: `[[src/net/iocp/iocp_context.cpp]]`
- selected when `COROUTE_IO_BACKEND` is `iocp`

> [!note]
> The OS abstraction is a structural design choice, not a portability afterthought. Higher layers depend on `IoContext`, `Listener`, and `Connection` rather than on per-platform APIs.

## Accept loop and connection ownership

In server mode, `App::run()` creates the listener and then starts an accept loop.

Important runtime characteristics:

- new connections are moved into detached coroutines
- detached coroutines own their connection object
- graceful shutdown is tracked through `active_connections_` and cancellation tokens
- some paths attempt multi-accept for scalability before falling back to a single-listener loop

Relevant files:

- `[[src/core/app.cpp]]`
- `[[include/coroute/coro/task.hpp]]`
- `[[include/coroute/coro/cancellation.hpp]]`

## Middleware and routing in the runtime path

The runtime does not directly invoke handlers after parsing a request. It goes through:

- router match
- middleware chain execution
- handler invocation or not-found fallback

That means authentication, compression, logging, and other cross-cutting behaviors remain outside the transport layer.

Relevant files:

- `[[include/coroute/core/app.hpp]]`
- `[[include/coroute/core/router.hpp]]`
- `[[src/core/router.cpp]]`

## Protocol branching inside the runtime

Once a connection exists, the runtime may branch into specialized protocol handling:

- TLS handshake and ALPN negotiation
- HTTP/2 connection handling
- WebSocket upgrade and WebSocket session handling
- plain HTTP/1.1 request/response flow

Relevant files:

- `[[src/core/app.cpp]]`
- `[[include/coroute/net/tls.hpp]]`
- `[[include/coroute/net/websocket.hpp]]`
- `[[include/coroute/http2/connection.hpp]]`

## Graceful shutdown

`App::shutdown(...)` is designed to:

- stop accepting new connections
- wait for in-flight work to drain
- force cancellation after a timeout when configured
- stop the event loop when draining completes or times out

Relevant files:

- `[[include/coroute/core/app.hpp]]`
- `[[src/core/app.cpp]]`
- `[[doc/v2/en/chapters/04_architecture.tex]]`

## Example and usage anchors

- `[[examples/Samples/hello_world/main.cpp]]`
- `[[examples/Samples/https_server/main.cpp]]`
- `[[examples/Samples/http2_server/main.cpp]]`
- `[[examples/Samples/websocket_server/main.cpp]]`

## Status

### Current status

- **Implemented**: platform-selected event-loop backends, listener/connection abstractions, runtime accept loop, and graceful shutdown support.
- **Implemented**: protocol branching for TLS, HTTP/2, and WebSocket is visible from the `App` runtime path.
- **Observed build selection**: backend choice is encoded in `[[CMakeLists.txt]]`.
- **Testing note**: protocol-specific tests exist for HTTP/2 and compression; I did not find backend-specific test files named for `io_uring`, `kqueue`, or `IOCP` in `[[tests]]` during this pass.

## Related notes

- [[Architecture/Project Atlas]]
- [[Architecture/View and API Abstractions]]
- [[Architecture/Framework Integration Architecture]]
- [[Protocols/Protocols Index]]
- [[Protocols/HTTP-1.1]]
- [[Protocols/HTTP-2]]
- [[Protocols/WebSocket]]
- [[Protocols/TLS]]
