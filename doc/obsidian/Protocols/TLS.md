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

## Implementation Details

### 1. OpenSSL Integration
Coroute uses **OpenSSL** (or BoringSSL/LibreSSL) for its cryptographic operations. 
- **`TlsContext`**: A thin RAII wrapper around `SSL_CTX`. It handles certificate loading, private key verification, and global protocol policies.
- **`TlsConnection`**: Wraps the raw socket `Connection`. It uses an internal `SSL*` object to perform the handshake and encrypt/decrypt data.

### 2. Configuration (`TlsConfig`)
Setting up TLS requires a `TlsConfig` struct:
```cpp
struct TlsConfig {
    std::string cert_file;      // Path to server certificate
    std::string key_file;       // Path to private key
    std::string ca_file;        // (Optional) CA certificate
    bool verify_client = false; // Client certificate verification
    std::vector<std::string> alpn_protocols; // e.g., {"h2", "http/1.1"}
};
```

### 3. Version Support
Coroute prioritizes modern encryption:
- **TLS 1.3**: Supported and preferred. It reduces handshake latency (1-RTT) and removes insecure cipher suites.
- **TLS 1.2**: Supported for compatibility but can be disabled via configuration.
- **Older Versions**: SSL 2.0/3.0 and TLS 1.0/1.1 are disabled by default for security.

### 4. ALPN Negotiation
Application-Layer Protocol Negotiation is critical for HTTP/2.
- **Process**: During the TLS Client Hello / Server Hello exchange, the client and server negotiate the upper-layer protocol.
- **Result**: The `TlsConnection` stores the negotiated string (e.g., `h2`). The `App` uses this to decide whether to start an `Http2Connection` or an `Http1Connection`.

### 5. Asynchronous Handshake
Unlike traditional synchronous servers where `SSL_accept` blocks the thread, Coroute handles the handshake as a C++20 coroutine state. If OpenSSL needs more data (`SSL_ERROR_WANT_READ`), the coroutine suspends and the `IoContext` takes over until the socket is ready again.

Relevant files:
- `[[include/coroute/net/tls.hpp]]`
- `[[src/net/tls/tls_context.cpp]]`
- `[[src/core/app.cpp]]`

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
