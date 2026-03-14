---
trigger: glob
globs: **/*.cpp,**/*.hpp,**/*.dart,**/CMakeLists.txt
description: Cross-cutting security posture, ABI safety, input validation, and TLS enforcement
---

# General Security Rules

WebFrame is designed for internet-facing, high-load async deployments. These rules apply across the entire codebase — C++ backend, Dart/Flutter frontend, and build configuration.

## 1. Input Validation & Parsing
- **Treat all external input as hostile.** Sanitize inputs on both the Flutter client and the C++ backend before processing.
- **Sanitize JSON / Form Payloads:** Do not blindly unpack `req.json()` or `req.form()` into database structures or pass form values unescaped into HTML templates (Inja) or backend logic without explicit validation.
- **Route Parameter Validation:** The framework extracts `{id}` as an `int` safely, but backend logic must verify logical bounds and existence (e.g., reject negative IDs, guard against overflow).
- **Never trust size hints from untrusted sources** when pre-allocating buffers — always clamp to a defined maximum.

## 2. Authentication and Middlewares
- **Middleware Ordering:** Register security middlewares (trailing slash normalization, CORS, authentication extraction, rate limiting) **before** domain logic endpoints or database routing handlers.
- **`AuthState` Integrity:** When using `App::fetch(method, path)` for internal sub-requests, correctly propagate the `AuthState` of the original initiator to avoid silently bypassing security boundaries.
- **No default-open endpoints:** Every route must have an explicit authorization decision — either a guard middleware or an explicit `public` annotation. There is no implicit public access.

## 3. TLS / HTTPS Enforcement
- **Always enforce TLS 1.2+ in production.** Never downgrade the context to accept obsolete ciphers or hash functions.
- **Modern Cipher Suites:** Use the most modern configuration offered by the internal TLS bindings (`COROUTE_ENABLE_TLS=ON`).
- **Production Security Headers:** Every TLS response must include:
    - `Strict-Transport-Security` (HSTS)
    - `Content-Security-Policy`
    - `X-Frame-Options`
- Implement these via a dedicated middleware, not per-route.

## 4. ABI & FFI Safety (C++/Dart Boundary)
- **`extern "C"` boundaries:** Never throw C++ exceptions across an `extern "C"` function. Dart cannot catch them — doing so hard-crashes the application.
- **Struct alignment:** Anticipate ABI mismatches across platforms (Windows, macOS, Linux, Android, iOS). Verify struct alignment and packing at the FFI boundary.
- **No complex C++ types across the boundary:** Only primitive C types (`int`, `double`, `bool`, `char*`), opaque pointers (`void*`), or trivially copyable C-structs may cross the FFI boundary.

## 5. Connection Draining
- **Graceful Shutdowns:** Ensure `ShutdownOptions` manages in-flight `io_uring` / `IOCP` / `kqueue` tasks cleanly with a short drain period. Force-killing the thread pool abruptly can corrupt half-written HTTP/WebSocket frames or tear TLS sessions.
