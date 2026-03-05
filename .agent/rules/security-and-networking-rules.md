# Security and Networking Guidelines

WebFrame is designed for internet-facing, high-load async deployments. Adhere to these strict policies for network integrity and input validation.

## 1. Request Validation & Parsing
- **Sanitize JSON / Form Payloads:** Treat incoming `req.json()` or `req.form()` inputs as fundamentally hostile. Do not blindly unpack JSON into database structures or pass form values unescaped into HTML templates (Inja) or backend logic without explicit checks.
- **Route Matching Parameters:** Validate parameter captures safely. The framework extracts `{id}` as an `int` safely, but backend logic must verify logical bounds and existence (e.g., checking for negative IDs or malicious overflow).

## 2. Authentication and Middlewares
- **Middleware Ordering:** Always register security middlewares (e.g., trailing slash normalization, CORS handling, Authentication extraction, Rate Limiting) BEFORE domain logic endpoints or database routing handlers. 
- **`AuthState` Integrity:** When using `App::fetch(method, path)` to perform purely internal, in-memory sub-requests, assure that the internal route correctly checks or propagates the `AuthState` of the original initiator to authorize the local sub-fetch correctly. Do not unknowingly bypass security boundaries.

## 3. TLS / HTTPS Enforcement
- **Modern Cipher Suites:** WebFrame includes internal TLS bindings (`COROUTE_ENABLE_TLS=ON`). Use the most modern configuration offered (TLS 1.2 / TLS 1.3). Never downgrade the context to accept obsolete, vulnerable ciphers or hashes.
- **Production Headers:** Implement middleware that enforces strictly secure response headers (HSTS `Strict-Transport-Security`, `Content-Security-Policy`, `X-Frame-Options`) on every TLS response natively.

## 4. Connection Draining
- **Graceful Shutdowns:** Ensure `ShutdownOptions` manages in-flight `io_uring` or `IOCP` tasks cleanly by allowing a short drain period. Force-killing the execution thread pool abruptly can tear TLS connections or corrupt half-written HTTP/WebSocket frames.
