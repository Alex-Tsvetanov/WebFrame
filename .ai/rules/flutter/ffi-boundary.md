---
trigger: glob
globs: include/**/*.hpp,src/bridge/**/*.cpp,packages/**/*.dart,packages/**/CMakeLists.txt
description: Strict C++ to Dart FFI boundary rules for ABI safety, memory ownership, and thread safety
---

# Flutter FFI Boundary Rules (C++ to Dart Interop)

Coroute compiles web logic into headless engines for Flutter via `src/bridge/` and `packages/coroute_framework/`. Strict adherence to the C API boundary ensures stability and cross-platform reliability.

## 1. Strict `extern "C"` ABI
- **No C++ Overloads:** All exported functions must be enclosed in `extern "C"`. Overloaded functions are name-mangled and uncallable from Dart.
- **Primitive Data Types Only:** Only pass primitive C types (`int`, `double`, `bool`, `char*`), opaque pointers (`void*`), or trivially copyable C-structs across the boundary. Never pass `std::string`, `std::vector`, or any complex C++ class.
- **No exceptions across the boundary:** Every exported `extern "C"` function must wrap its body in `try/catch(...)`. A C++ exception crossing this boundary hard-crashes the process — Dart cannot catch it.

```cpp
extern "C" int bridge_call() {
    try {
        // ... logic
        return 0;
    } catch (...) {
        return -1;
    }
}
```

## 2. Dart FFI Bindings: `@Native` Over `lookupFunction`
- **Always use `@Native()` external function declarations** for all Dart FFI bindings. Never use `DynamicLibrary.open()` + `lookupFunction<C, Dart>(symbol)`.
- `@Native` is resolved at compile time against the declared `CodeAsset`, eliminates runtime `DynamicLibrary` management, and is the idiomatic modern Dart FFI pattern.
- When refactoring existing FFI code, always migrate `lookupFunction` to `@Native` — do not leave mixed patterns in the same file.

## 3. Memory Ownership
- **Always document ownership explicitly** on every function that returns a pointer: which side (C++ or Dart) owns the allocation and is responsible for freeing it.
- **C++ → Dart allocations:** If C++ allocates memory returned to Dart (e.g., a `char*` string), C++ **must** provide a paired `extern "C" void bridge_free(void* ptr)` function that Dart calls via `Finalizer`. Dart's garbage collector will never free C++ allocations.
- **Dart → C++ allocations:** Document the lifetime contract. Ensure C++ does not retain a pointer to Dart-managed memory beyond the call.

## 4. `noexcept` on All Bridge Functions
- All `extern "C"` bridge functions must be marked `noexcept`. This is a compile-time enforcement of the no-throw contract at the boundary.

```cpp
extern "C" int bridge_call() noexcept {
    try { /* ... */ return 0; }
    catch (...) { return -1; }
}
```

## 5. Thread Safety & Asynchrony
- **Dart is single-threaded per isolate.** If C++ invokes a Dart callback from a background worker thread (`net::IoContext`), the message-passing primitive must be thread-safe.
- **Use `Dart_PostCObject`** for safe, asynchronous cross-isolate communication from C++ threads back to the Dart UI thread. Never call Dart callbacks directly from C++ background threads.
