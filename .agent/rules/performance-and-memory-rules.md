# Performance and Memory Rules

WebFrame/Coroute's performance is strictly bound by allocation frequency and memory copies on the hot path (the per-request event loop). Adhere to these strategies for zero-copy operation and maximum throughput.

## 1. Zero-Copy String Operations
- **`std::string_view` over `std::string`:** For read-only string inspection (e.g., HTTP headers, URI paths, regex matched parameters), always use `std::string_view`. Avoid allocating intermediate `std::string` objects.
- **`std::span` over `std::vector`:** When reading chunks from a socket or processing byte buffers, pass and slice memory using `std::span<uint8_t>`.
- **String Concat:** Use `std::format` or pre-allocate memory utilizing `.reserve()` before appending strings to prevent multiple heap re-allocations.

## 2. SIMD JSON Practices
- **In-Place Parsing:** When accessing `req.json()`, use the integrated `simdjson` module correctly. Parse the JSON text in-place without copying the raw string.
- **Streamlined Access:** Prevent serializing the JSON into expensive DOM objects (`nlohmann::json`) if you only need a single value. Traverse `simdjson::ondemand` directly.

## 3. Middleware & Routing Hot Path
- **Pre-Compilation:** Middleware chains must be strictly initialized at application setup (`CompiledMiddlewareChain`). Do not construct or allocate new chain components inside the `Router` match sequence!
- **DFA Router Optimization:** The `RegexMatcher` DFA handles incoming requests with zero dynamic allocation. Handler callbacks must capture dependencies by reference to an application state object instead of copying huge state copies into each routing thunk.

## 4. Memory Moves
- **Enforce `std::move`:** When transferring heavy objects (e.g., `std::vector` buffers, `Response` structures, or payload body strings), strictly use `std::move()` to transfer ownership without copying memory.
- **Return Value Optimization (RVO):** Build the response directly in the return statement when feasible so the compiler elides to copies.
