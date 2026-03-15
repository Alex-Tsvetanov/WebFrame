---
title: Compression and Content Encoding
tags:
  - coroute
  - compression
  - content-encoding
  - middleware
aliases:
  - Compression
  - Content Encoding
---

# Compression and Content Encoding

> [!abstract]
> Coroute includes response compression support as middleware rather than as a separate transport stack. The current implementation supports gzip and deflate through zlib, and optional Brotli support when enabled at build time.

## Main implementation files

Relevant files:

- `[[include/coroute/core/compression.hpp]]`
- `[[src/core/compression.cpp]]`
- `[[tests/test_compression.cpp]]`
- `[[CMakeLists.txt]]`

## Supported algorithms

Observed algorithm surface:

- `gzip`
- `deflate`
- `br` / Brotli
- `identity`

Brotli is conditional on build support.

## Parsing and negotiation

The compression layer includes logic for:

- parsing `Accept-Encoding`
- honoring quality values
- ignoring disabled/zero-quality encodings
- preferring stronger available encodings according to Coroute's internal ordering

Relevant files:

- `[[src/core/compression.cpp]]`
- `[[tests/test_compression.cpp]]`

## Middleware behavior

Compression is expressed as middleware so it participates in the normal HTTP response pipeline.

Observed middleware responsibilities:

- wait for the downstream handler response
- skip already encoded responses when configured to do so
- skip small bodies below the configured threshold
- skip non-compressible content types
- parse the request's `Accept-Encoding`
- compress only when it helps
- set `Content-Encoding`, `Content-Length`, and optionally `Vary`

Relevant files:

- `[[src/core/compression.cpp]]`
- `[[README.md]]`

## Build-time behavior

Observed build behavior in `[[CMakeLists.txt]]`:

- zlib is required for the base gzip/deflate implementation
- Brotli is optional and controlled by `COROUTE_ENABLE_BROTLI`
- `COROUTE_HAS_BROTLI` is defined when Brotli support is actually available

## Example usage

- `[[examples/Samples/hello_world/main.cpp]]`
- `[[examples/Project/src/app/server.cpp]]`
- `[[examples/FlutterProject/src/app/server.cpp]]`
- `[[README.md]]`

## Testing and evidence

The compression note has explicit supporting tests.

Observed test areas in `[[tests/test_compression.cpp]]`:

- algorithm naming
- `Accept-Encoding` parsing and quality handling
- gzip round-trip behavior
- deflate round-trip behavior
- Brotli availability-dependent behavior
- compression options defaults
- content-type matching
- generic compression helper behavior

## Status

### Current status

- **Implemented**: gzip and deflate support are present and test-backed.
- **Implemented with conditional build support**: Brotli support exists but depends on build-time availability.
- **Implemented**: response compression is available as normal middleware.
- **Test-backed**: `[[tests/test_compression.cpp]]` provides direct coverage for the core algorithm and negotiation logic.

## Related notes

- [[Protocols/Protocols Index]]
- [[Protocols/HTTP-1.1]]
- [[Architecture/Server Runtime and OS Backends]]
