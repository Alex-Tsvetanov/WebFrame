---
title: TLS
tags:
  - coroute
  - tls
  - https
  - protocol
aliases:
  - TLS Support
  - HTTPS Support
---

# TLS

> [!abstract]
> Coroute wraps TLS behind `TlsContext`, `TlsConnection`, and `TlsListener`, integrating HTTPS and ALPN-based protocol negotiation into the main runtime. The current implementation is OpenSSL-backed and is enabled in the build when TLS support is available.

## Main abstractions

Relevant files:

- `[[include/coroute/net/tls.hpp]]`
- `[[src/net/tls/tls_context.cpp]]`
- `[[include/coroute/core/app.hpp]]`
- `[[src/core/app.cpp]]`

### `TlsConfig`

The current TLS configuration surface includes:

- certificate and key paths
- optional CA and chain files
- minimum TLS version
- client certificate verification flag
- cipher configuration
- ALPN protocol list
- session ticket and cache settings

### `TlsContext`

`TlsContext` owns the underlying SSL context and related policy such as SNI callback support and ALPN result lookup.

### `TlsConnection`

`TlsConnection` wraps a generic `Connection` and exposes:

- handshake
- async read/write operations
- peer certificate inspection
- negotiated protocol lookup
- TLS version lookup

### `TlsListener`

`TlsListener` wraps an existing listener and accepts TLS connections, optionally performing the handshake during accept.

## Runtime integration

The application-facing entry point is:

- `App::enable_tls(const AppTlsConfig&)`

At runtime, when TLS is enabled:

- `App::run()` creates a TLS listener path
- accepted connections are wrapped as TLS connections
- ALPN negotiation may select HTTP/2 when available and enabled
- otherwise the connection continues on the HTTP/1.1 path

Relevant files:

- `[[include/coroute/core/app.hpp]]`
- `[[src/core/app.cpp]]`

## Relation to HTTP/2

TLS is the current gateway to HTTP/2 selection in the main runtime path.

Observed behavior:

- if HTTP/2 is enabled, ALPN defaults can advertise both `h2` and `http/1.1`
- the negotiated protocol is checked on the TLS connection
- `h2` leads into the HTTP/2 connection path

See also:

- [[Protocols/HTTP-2]]

## Build-time enablement

Observed build behavior in `[[CMakeLists.txt]]`:

- TLS support is controlled by `COROUTE_ENABLE_TLS`
- OpenSSL is linked into the framework
- `COROUTE_HAS_TLS` is defined when TLS support is enabled
- `[[src/net/tls/tls_context.cpp]]` is added to the build

## Example usage

- `[[examples/Samples/https_server/main.cpp]]`
- `[[examples/Samples/http2_server/main.cpp]]`
- `[[README.md]]`

## Testing and implementation evidence

Implementation evidence is clear from:

- `[[include/coroute/net/tls.hpp]]`
- `[[src/net/tls/tls_context.cpp]]`
- `[[src/core/app.cpp]]`

During this pass, I did **not** find a dedicated TLS-focused test file in `[[tests]]`.

## Status

### Current status

- **Implemented**: OpenSSL-backed TLS context, connection, and listener abstractions are present.
- **Implemented**: `App` integrates TLS into the runtime and uses it for HTTPS and ALPN.
- **Integrated**: HTTP/2 selection hooks depend on the TLS path when available.
- **Test note**: I did not find a dedicated TLS test file during this pass.

## Related notes

- [[Protocols/Protocols Index]]
- [[Architecture/Server Runtime and OS Backends]]
- [[Protocols/HTTP-1.1]]
- [[Protocols/HTTP-2]]
