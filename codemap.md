# WebFrame / Coroute v2 - Architecture Codemap

## Overview
Coroute v2 is a high-performance C++20 web framework leveraging coroutines, io_uring/kqueue/IOCP for platform-optimized async I/O, TLS/SSL, HTTP/2, WebSocket, and an internal routing/middleware system. It also features seamless Flutter integration, allowing web applications to be compiled directly into native mobile (iOS/Android) and desktop (macOS/Windows/Linux) applications sharing the same C++ core logic via FFI.

## Directory Structure
- `include/coroute/` - Public header files.
  - `core/` - Application, Routing, Middleware, Request/Response, Server mechanics.
  - `coro/` - C++20 coroutine primitives (`Task`, awaitables, generators).
  - `http2/` - HTTP/2 protocol implementation.
  - `net/` - Networking primitives (sockets, TLS).
  - `util/` - Utilities (logging, configuration, parsers).
  - `view/` - Template rendering (Inja wrapper).
- `src/` - Implementation files matching the `include/coroute/` structure.
  - `bridge/` - C/C++ FFI boundary for exposing Coroute internals to Flutter.
- `tests/` - Unit tests (using a standard C++ testing framework).
- `examples/` - Example applications demonstrating routing, websocket, templates, etc.
- `packages/coroute_framework/` - Core Flutter framework for Coroute integration.
- `benchmark/` - Performance benchmarking against other frameworks.
- `cmake/` - CMake configuration modules (including `CorouteApp.cmake` for Flutter integration).
- `external/` or `vendor/` - Third-party dependencies (like OpenSSL, nlohmann_json, simdjson, Inja, etc.).

## Core Architecture & Execution Model
1. **Application Lifecycle (`App`)**: The `App` class is the main entry point. It handles route registration, pre-compiles the middleware chain, and manages the `net::IoContext` and HTTP listeners. Offers a graceful shutdown with connection draining (`ShutdownOptions`).
2. **Threading & I/O**: Platform-native asynchronous I/O is abstracted via `net::IoContext` (io_uring, kqueue, IOCP). Thread pools concurrently process event loops.
3. **C++20 Coroutines (`Task<T>`)**: Avoids callback hell. Almost all request handlers and middlewares return `Task<Response>` or `Task<void>` using `co_await` and `co_return`. Coroutines efficiently share execution threads.

## Routing & Request Handling
1. **DFA Regex Routing**: The `Router` uses a DFA-based `RegexMatcher` allowing extremely fast O(n) exact path matching without backtracking.
2. **Type-Safe Parameter Extraction**: Parameters are automatically extracted positionally. `app.get<int, std::string>("/user/{id}/post/{slug}", [](int id, std::string slug, Request& req))` extracts the parameter directly without manual string parsing.
3. **Internal Sub-requests (`App::fetch`)**: The system supports internal routing fetches. Handlers can call `App::fetch(method, path)` to invoke another endpoint completely in-memory, bypassing the network stack but correctly propagating `AuthState`.

## Middleware Pipeline
1. **Compiled Middleware Chain**: Middleware handlers (`Middleware`) are registered and compiled in `CompiledMiddlewareChain` to avoid vector iteration overhead during per-request execution.
2. **Composition via `Next`**: Middleware functions take a `Request&` and a `Next` callback. This allows code to execute before handing off to the next chain element, and wait for the `Response` to modify headers or log times on the way back up.
3. **Built-in Systems**: The framework provides established middlewares for authentication (`AuthState`), logging, compression, static files serving, and session/cookie serialization.

## Content & Templating
1. **Body Parsing**: Framework utilities seamlessly parse JSON (nlohmann / simdjson via `req.json()`) and form data (`req.form()`).
2. **Template Views**: The `view/` directory encapsulates `ViewTemplates` and `ViewRouteInfo`. Features integration with the Inja template engine. 
3. **Response Builder**: Advanced endpoints utilize `ResponseBuilder` for expressive chaining of headers, streaming chunks, content type, and status codes. 

## Flutter Integration (`CorouteApp.cmake` & `coroute_framework`)
1. **Shared Library FFI**: When targeting Desktop/Mobile, `CorouteApp.cmake` compiles the developer's exact web application C++ logic into a `coroute_app` shared library instead of a standalone server executable.
2. **Automated Sandbox**: CMake automatically scaffolds the `.flutter/` project directory, links the user's viewmodels and `main.dart`, and prepares the podfiles/gradle scripts. Flutter dynamically loads the C++ core via Dart FFI.
3. **Headless Web Engine**: The app runs the exact same framework routing and data layers, passing UI render events between the Dart frontend and the C++ coroutine backend, ensuring total code reuse.

## Development Workflow
- **Build System**: CMake. Use `cmake -B build -DCMAKE_BUILD_TYPE=Release` and `cmake --build build`.
- **Options**:
  - `COROUTE_BUILD_EXAMPLES` (default ON)
  - `COROUTE_BUILD_TESTS` (default ON)
  - `COROUTE_ENABLE_TLS` (default ON)
  - `COROUTE_ENABLE_HTTP2` (default ON)
  - `COROUTE_ENABLE_SIMDJSON` (default ON)

## Instructions for AI Agents
When receiving a request related to WebFrame/Coroute:
1. Identify the domain of the request (e.g., routing, networking, templates, Flutter FFI).
2. Check `include/coroute/<domain>` for interfaces and header definitions.
3. Check `src/<domain>` for implementation details.
4. If integrating with Flutter, refer to `packages/coroute_framework/` and `cmake/CorouteApp.cmake`.
5. Verify your changes by ensuring it builds cleanly and running tests via `ctest --output-on-failure` from the `build` directory.
