# Flutter FFI Boundary Rules (C++ to Dart Interop)

Coroute automatically compiles web logic into headless engines for Flutter via `bridge/` and `packages/coroute_framework/`. Strict adherence to the C API boundary ensures stability and cross-platform reliability.

## 1. strict `extern "C"` ABI
- **No C++ Overloads:** All exported functions must be enclosed in `extern "C"`. Overloaded functions will be mangled and uncallable from Dart.
- **Primitive Data Types Only:** Only pass primitive C-types (`int`, `double`, `bool`, `char*`), opaque pointers (`void*`), or trivially copyable C-structs across the boundary. Never pass `std::string`, `std::vector`, or complex C++ classes.

## 2. Exception Safety Across ABI
- **Never Throw Across the Boundary:** Dart cannot catch C++ exceptions. Throwing a C++ exception across an `extern "C"` function will hard-crash the application locally.
- **Fail-Safe Catching:** Wrap every exported FFI function in a top-level `try/catch(...)`:
```cpp
extern "C" int bridge_call() {
    try {
        // ... logic
        return 0; // Success
    } catch (...) {
        return -1; // Error code to Dart
    }
}
```

## 3. Memory Ownership
- **Explicit Ownership:** Always document whether C++ or Dart owns the returned pointer.
- **Providing Release Endpoints:** If C++ allocates memory for Dart using `new` or `malloc` (e.g., passing a `char*` string to Dart), C++ MUST provide a paired `extern "C" void bridge_free(void* ptr)` function that Dart can call via `Finalizer` to free the resource. Dart's garbage collector will not free C++ allocations.

## 4. Thread Safety & Asynchrony
- **Dart is Single Threaded:** Dart executes isolated event loops. If C++ invokes a Dart callback (e.g., Dart port post message) from a background worker thread (`net::IoContext`), ensure the message passing primitives are thread-safe.
- **Native Ports:** Use `Dart_PostCObject` for safe, asynchronous cross-isolate communication from C++ threads back to the Dart UI thread.
