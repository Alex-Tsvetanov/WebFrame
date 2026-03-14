---
trigger: glob
globs: **/CMakeLists.txt,**/*.cmake
description: Modern CMake rules for target-centric, cross-platform builds
---

# Modern CMake Build Rules

Coroute utilizes a custom auto-scaffolded CMake system out-of-the-box (e.g., `cmake/CorouteApp.cmake`). Ensuring the structure remains strictly "Modern CMake" guarantees cross-platform reliability when compiling the native FFI boundary on all target platforms.

## 1. Target-Centric Build Scripts
- **No Global Configurations:** Absolutely do not use legacy global CMake commands. All properties MUST be bound to a specific target to prevent namespace and linking collisions.
    - ❌ `include_directories(...)` / `link_libraries(...)` / `add_compile_options(...)`
    - ✅ `target_include_directories(my_target PUBLIC ...)`
    - ✅ `target_link_libraries(my_target PRIVATE ...)`
    - ✅ `target_compile_features(my_target PUBLIC cxx_std_23)`
- **Visibility Keywords:** Always use `PUBLIC`, `PRIVATE`, or `INTERFACE` when specifying dependencies or include directories for a target.

## 2. Options and Preprocessor Variables
- **Handling Built-In Options:** Utilize the predefined `COROUTE_*` flags (e.g., `COROUTE_ENABLE_TLS`, `COROUTE_ENABLE_HTTP2`) rather than hardcoding manual feature flags.
- **Target Compile Definitions:** Inject constants or flags via `target_compile_definitions(my_target PRIVATE ENABLE_MY_FEATURE=1)`. Do not use `add_definitions()`.

## 3. Flutter / Native Scaffolding Integrity
- **Preserve `CorouteApp.cmake`:** Do not modify the core scaffold modules unless strictly necessary. Ensure any added directories are safely globbed or explicitly listed before the `add_library(coroute_app SHARED ...)` compilation stage.
- **Transitive Dependencies:** Rely on CMake's transitive dependency resolution. If `coroute_framework` depends on OpenSSL, simply linking `coroute_framework` into your project target will automatically pull in the required include flags and link flags.

## 4. Build Hooks Over Manual Steps
- **Automate via hooks:** Any step that must produce a file or set a build property must be expressed as a CMake custom command or target, not as a manual post-build instruction to the developer.
- **No hardcoded paths:** Use `CMAKE_CURRENT_SOURCE_DIR`, `CMAKE_CURRENT_BINARY_DIR`, generator expressions, and `find_package` rather than absolute or machine-specific paths.
