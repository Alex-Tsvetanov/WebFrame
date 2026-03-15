---
title: Project Atlas
tags:
  - coroute
  - architecture
  - obsidian
  - atlas
aliases:
  - Coroute Atlas
  - WebFrame Atlas
---

# Project Atlas

> [!abstract]
> This note is the entry point for the Obsidian architecture vault. It maps the repository from the shared C++ engine, through the operating-system backends and protocol implementations, to the host-framework adapters that let the same application logic run as a web server or as an embedded engine for Flutter.

## How to read this vault

Start here for the big picture, then branch into:

- [[Architecture/View and API Abstractions]]
- [[Architecture/Server Runtime and OS Backends]]
- [[Architecture/Framework Integration Architecture]]
- [[Architecture/Flutter Integration]]
- [[Renderers/Renderers and Template Engines]]
- [[Protocols/Protocols Index]]

## Repository map

### Engine and public API

- `include/` exposes the framework surface used by applications.
- `src/` contains the implementation of routing, runtime, protocols, and integration layers.
- `[[include/coroute/coroute.hpp]]` is the umbrella include for consumers.

### Formal project documentation

- `[[doc/v2/en/chapters/04_architecture.tex]]` is the main formal architecture chapter.
- `[[doc/v2/bg/chapters/04_architecture.tex]]` is the Bulgarian counterpart.
- `[[README.md]]` is the practical framework overview.

### Integration packages

- `[[packages/Flutter/TECHNICAL_SPEC.md]]` describes the Flutter package contract.
- `[[packages/Flutter/coroute_framework/pubspec.yaml]]` anchors the reusable Flutter adapter package.
- `packages/KotlinMultiPlatform/` exists as a future integration direction rather than a finished adapter.

### Example applications

- `[[examples/Project/src/main.cpp]]` shows the web-server oriented project structure.
- `[[examples/FlutterProject/main.dart]]` shows the cross-platform app path.
- `[[examples/view_example/main.cpp]]` is the most compact illustration of the view contract.
- `[[examples/Samples/http2_server/main.cpp]]`, `[[examples/Samples/https_server/main.cpp]]`, and `[[examples/Samples/websocket_server/main.cpp]]` are the main protocol-focused samples.

## Architectural thesis

The central idea of Coroute v2 is that the same C++ application logic can be deployed in more than one presentation mode.

- In **web-server mode**, Coroute owns the socket listener, parses HTTP, routes requests, runs middleware, and renders HTML or returns JSON.
- In **embedded app mode**, Coroute is compiled as a shared library and hosted by an external UI runtime, currently Flutter.
- The shared part between those modes is the application core: `App`, `Router`, `Task<T>`, `IoContext`, and the view-model contract represented by `ViewTemplates` and `ViewResult`.

## Main architectural layers

### 1. Request and application orchestration

The `App` class is the orchestration root.

Relevant files:

- `[[include/coroute/core/app.hpp]]`
- `[[src/core/app.cpp]]`
- `[[include/coroute/core/router.hpp]]`
- `[[src/core/router.cpp]]`

### 2. Coroutine runtime and OS-independent async I/O

Coroute isolates its event-loop and socket behavior behind `IoContext`, then selects the concrete backend per platform.

Relevant files:

- `[[include/coroute/net/io_context.hpp]]`
- `[[src/net/io_uring/uring_context.cpp]]`
- `[[src/net/kqueue/kqueue_context.cpp]]`
- `[[src/net/iocp/iocp_context.cpp]]`
- `[[CMakeLists.txt]]`

### 3. View and rendering contract

The view system allows one handler to return platform-specific template identifiers plus a shared serialized model.

Relevant files:

- `[[include/coroute/view/view_types.hpp]]`
- `[[include/coroute/view/view_renderer.hpp]]`
- `[[include/coroute/view/web_renderer.hpp]]`
- `[[src/view/web_renderer.cpp]]`

### 4. Host-framework integration

The `bridge/` layer and the package adapters let Coroute run inside another runtime without moving business logic into that runtime.

Relevant files:

- `[[src/bridge/bridge.cpp]]`
- `[[cmake/CorouteApp.cmake]]`
- `[[packages/Flutter/coroute_framework/lib/bridge.dart]]`
- `[[packages/Flutter/coroute_framework/hook/build.dart]]`

### 5. Protocol implementations

Coroute contains concrete implementations for HTTP/2, WebSocket, TLS, and compression alongside the default HTTP/1.1 path.

Relevant notes:

- [[Protocols/HTTP-1.1]]
- [[Protocols/HTTP-2]]
- [[Protocols/WebSocket]]
- [[Protocols/TLS]]
- [[Protocols/Compression and Content Encoding]]

## Design boundaries

> [!note]
> The clean separation in this repository is **engine vs adapter**.
>
> - The engine owns routing, middleware, protocol behavior, view-model creation, and transport abstractions.
> - The adapter owns packaging, host-runtime bindings, and target-native presentation.

### Engine-owned concerns

- request parsing and response construction
- middleware execution
- route matching
- view selection and model serialization
- protocol implementations
- FFI-safe exported boundary

### Adapter-owned concerns

- native asset bundling
- callback registration in the host runtime
- mapping backend-selected template names to native UI classes
- target-specific packaging fixes

## Status of the vault

This vault is meant to be descriptive, not speculative.

- If a feature is implemented and visible in code, the relevant note links to its implementation files.
- If a feature appears tested, the relevant note links to the test file.
- If a feature is planned but not implemented, the note calls that out explicitly.

## Next notes

- Read [[Architecture/View and API Abstractions]] for the shared view/API contract.
- Read [[Architecture/Server Runtime and OS Backends]] for the runtime and event-loop story.
- Read [[Architecture/Framework Integration Architecture]] for the host-framework pattern.
- Read [[Architecture/Flutter Integration]] for the concrete Flutter path.
