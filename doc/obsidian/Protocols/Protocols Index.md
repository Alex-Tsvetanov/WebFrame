---
title: Protocols Index
tags:
  - coroute
  - protocols
  - status
aliases:
  - Protocol Support Index
  - Transport Feature Index
---

# Protocols Index

> [!abstract]
> This note is the entry point for protocol-level documentation in the vault. It groups the major wire-level and content-encoding capabilities implemented in Coroute and summarizes their observed implementation status based on the current repository state.

## Protocol notes

- [[Protocols/HTTP-1.1]]
- [[Protocols/HTTP-2]]
- [[Protocols/WebSocket]]
- [[Protocols/TLS]]
- [[Protocols/Compression and Content Encoding]]

## Status summary

| Area | Status | Main evidence |
| --- | --- | --- |
| HTTP/1.1 | Implemented | `[[src/core/app.cpp]]`, `[[include/coroute/core/app.hpp]]` |
| HTTP/2 | Implemented with dedicated module and tests | `[[src/http2]]`, `[[tests/http2_tests.cpp]]`, `[[tests/test_http2_advanced.cpp]]` |
| WebSocket | Implemented, examples present, no dedicated test file found in this pass | `[[src/net/websocket.cpp]]`, `[[include/coroute/net/websocket.hpp]]`, `[[examples/Samples/websocket_server/main.cpp]]` |
| TLS | Implemented through OpenSSL-backed abstraction, no dedicated test file found in this pass | `[[src/net/tls/tls_context.cpp]]`, `[[include/coroute/net/tls.hpp]]` |
| Compression | Implemented with tests, Brotli optional at build time | `[[src/core/compression.cpp]]`, `[[tests/test_compression.cpp]]`, `[[CMakeLists.txt]]` |

## How protocols fit the runtime

Protocols are not independent subsystems floating outside the framework. They are selected and orchestrated from the runtime path rooted in `App`.

Relevant files:

- `[[include/coroute/core/app.hpp]]`
- `[[src/core/app.cpp]]`
- `[[include/coroute/net/io_context.hpp]]`
- `[[Architecture/Server Runtime and OS Backends]]`

## Notes on status language

This protocol index uses intentionally conservative labels.

- **Implemented** means the repository contains the code path and it is clearly wired into the runtime or build.
- **Test-backed** means I found relevant tests in `[[tests]]` during this pass.
- **No dedicated test file found in this pass** means only that I did not locate an obvious protocol-specific test file by name or by a quick targeted scan.

## Related notes

- [[Architecture/Project Atlas]]
- [[Architecture/Server Runtime and OS Backends]]
- [[Renderers/Renderers and Template Engines]]
