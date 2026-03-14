---
trigger: glob
globs: **/*.dart,**/windows/**,**/macos/**,**/linux/**
description: Desktop-specific security rules for Windows, macOS, and Linux targets
---

# Desktop Security Rules (Windows / macOS / Linux)

## 1. Secure Local Storage
- **Never store sensitive data in plaintext** on the filesystem (credentials, tokens, session cookies).
- **macOS:** Use `flutter_secure_storage` backed by the macOS Keychain Services API.
- **Windows:** Use `flutter_secure_storage` backed by the Windows Credential Manager (DPAPI).
- **Linux:** Use `flutter_secure_storage` backed by the Secret Service API (libsecret / GNOME Keyring / KWallet).
- **Never store sensitive values in plain `.json`, `.ini`, `.env`, or registry string values** that are readable without OS-level access control.

## 2. TLS Certificate Validation
- **Never disable TLS certificate verification** in desktop builds. The `HttpClient`'s `badCertificateCallback` must not return `true` unconditionally in production code.
- **Use the OS trust store** for root CA validation on each platform rather than bundling a static CA list, so system-managed certificate updates apply automatically.
- **Log and surface TLS errors** rather than silently swallowing them — a failed handshake must never fall back to an unencrypted connection.

## 3. Process & File System Isolation
- **Least privilege:** The application must not request administrator/root permissions unless strictly required. Any operation that requires elevation must be scoped and time-limited.
- **Sensitive files** (cached tokens, config files containing credentials) must be created with restrictive permissions (`0600` on POSIX, ACL-restricted on Windows). Never create them world-readable.
- **Temporary files** containing sensitive data must be deleted securely (overwrite before unlink where the OS does not guarantee zeroing).

## 4. Auto-Update & Distribution
- **Verify update signatures** before applying any auto-update package. Never apply an unsigned or unverified binary update.
- **Distribution artifacts** must be code-signed for macOS (notarized) and Windows (Authenticode). Unsigned desktop apps trigger OS security warnings and may be blocked.
