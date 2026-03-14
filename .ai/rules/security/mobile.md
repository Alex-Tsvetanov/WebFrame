---
trigger: glob
globs: **/*.dart,**/android/**,**/ios/**
description: Mobile-specific security rules for Android and iOS targets
---

# Mobile Security Rules (Android & iOS)

## 1. Secure Local Storage
- **Never store sensitive data in plaintext** (credentials, tokens, session cookies, device identifiers) in shared preferences, local files, or SQLite without encryption.
- **iOS:** Use `flutter_secure_storage` backed by the iOS Keychain for all sensitive values.
- **Android:** Use `flutter_secure_storage` backed by `EncryptedSharedPreferences` (Jetpack Security) or the Android Keystore system.
- **Never use `SharedPreferences` / `NSUserDefaults` for sensitive data** — these are plaintext, unencrypted, and accessible to backup tools.

## 2. Certificate Validation & Network Security
- **Do not disable SSL certificate validation** (`badCertificateCallback: (_,__,___) => true` is strictly forbidden in any non-test code path).
- **Certificate Pinning:** For production builds targeting known server endpoints, implement certificate or public-key pinning to prevent MITM attacks on mobile networks.
- **Enforce HTTPS only:** The `flutter_secure_storage` and network layer must never fall back to HTTP in production. Use `android:usesCleartextTraffic="false"` in the Android manifest and App Transport Security on iOS.

## 3. Biometric & Authentication
- **Use platform biometrics** (`local_auth` or native equivalents) for re-authentication on sensitive operations rather than re-prompting for a password in-app.
- **Do not cache raw passwords or tokens in memory longer than necessary.** Clear sensitive `String`/`Uint8List` values from memory as soon as they are no longer needed.

## 4. Build & Distribution
- **Release builds must be obfuscated:** Enable Dart obfuscation (`--obfuscate --split-debug-info`) for all production release builds.
- **Strip debug symbols** from release APKs/IPAs. Debug symbols must never appear in a production artifact.
- **ProGuard/R8 (Android):** Ensure rules are not inadvertently disabling obfuscation of security-sensitive classes.
