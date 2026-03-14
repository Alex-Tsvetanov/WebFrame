---
description: Comprehensive Codebase Security and Revision Checks
---

# Codebase Security and Revision Checks

Run this workflow periodically to revise the codebase for vulnerabilities, leaks, and compatibility issues. To trigger, the user can prompt: "Run the codebase revision workflow."

1. **Static Analysis & Memory Checking**
   - Run `clang-tidy` on all C++ source files to catch memory leaks, dangling pointers, and modern C++ violations.
   - Run Dart analyzer (`flutter analyze`) to catch Dart-side typing and logic issues.
// turbo

2. **Review FFI Boundaries**
   - Inspect all `extern "C"` functions for exception-safety blocks (`try/catch(...)`).
   - Audit pointer allocations passed to Dart to verify they have a corresponding `Finalizer` or free call.
// turbo

3. **Audit Data Storage Mechanisms**
   - Search for hardcoded API keys or plaintext credential storage in Dart (`grep_search` for `SharedPreferences` storing passwords).
   - Verify usage of secure storage plugins (`flutter_secure_storage`).
// turbo

4. **Database Injection & Query Audit**
   - Locate database interaction points in C++ and ensure parameterized statements are used exclusively.
// turbo

5. **UI & Cross-Platform Responsiveness Check**
   - Test UI layouts for overflow on various screen resolutions (Desktop vs Mobile dimensions).
   - Ensure native integration (ArkUI-X or standard iOS/Android) does not break accessibility or input scaling.
// turbo

6. **Network & Protocol Security**
   - Verify HTTPS/WSS enforcement in networking modules.
   - Check TLS configuration in WebFrame for outdated cipher suites.
