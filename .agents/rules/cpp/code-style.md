---
trigger: glob
globs: **/*.cpp,**/*.hpp,**/*.h
description: C++23 code style, coroutine, and performance rules for the Coroute codebase
---

# C++23 Code Style, Coroutine & Performance Rules

These rules are enforced via `.clang-format` and `.clang-tidy`. `WarningsAsErrors: "*"` is active — all linter warnings are build errors. Adhere strictly when writing or reviewing any C++ code.

---

## 1. Formatting (`.clang-format`)

- **Indentation:** 4 spaces. No tabs.
- **Line Length:** 120 characters maximum.
- **Braces:** Wrapped on their own line after classes, functions, control statements, enums, namespaces, and structs (`BreakBeforeBraces: Custom`).
- **Pointers and References:** Left-aligned (e.g., `int* ptr`, `const std::string& str`).
- **Access Modifiers:** Aligned with the class body (`AccessModifierOffset: -4`) — no extra indentation for `public:`, `protected:`, `private:`.
- **Namespaces:** Indent contents inside namespaces.
- **Includes:** Automatically grouped; not sorted alphabetically.

---

## 2. Naming Conventions (`.clang-tidy`)

- **Types (Classes, Structs, Enums):** `CamelCase` (e.g., `MyClass`, `GameState`).
- **Enum Constants:** `CamelCase` (e.g., `ActiveState`, `Pending`).
- **Functions & Methods:** `lower_case` (e.g., `get_user_data()`, `process_input()`).
- **Variables (Local, Global, Parameters):** `lower_case` (e.g., `player_name`, `index`).
- **Namespaces:** `lower_case` (e.g., `network`, `core`).
- **Data Members (Private/Protected):** `lower_case` with trailing underscore (e.g., `buffer_`, `connection_id_`).

---

## 3. Linter Rules & Exceptions

Active check families: `bugprone-*`, `cert-*`, `cppcoreguidelines-*`, `google-*`, `modernize-*`, `performance-*`, `readability-*`.

Notable exceptions:
- **Magic Numbers:** Allowed (`-cppcoreguidelines-avoid-magic-numbers`, `-readability-magic-numbers`).
- **C-style Arrays & Pointer Arithmetic:** Explicitly permitted.
- **`auto` Keyword:** Use for typenames evaluating to 5+ characters (`modernize-use-auto.MinTypeNameLength: 5`).
- **Trailing Return Types:** `auto foo() -> int` is not rigidly enforced.
- **Implicit Boolean Conversions:** Allowed.
- **`WarnOnAllAutoCopies`:** Active for range-based for loops — always use `const auto&` or `auto&` unless explicitly copying.

---

## 4. C++23 Best Practices

### Error Handling & State
- **`std::expected` and `std::optional` over exceptions and out-parameters.** Use monadic operations (`.and_then()`, `.transform()`, `.or_else()`) for clean functional flow.
- **`[[nodiscard]]` is mandatory** on all functions returning `std::expected<T,E>`, `Task<T>`, error codes, or resource handles. A caller that silently discards a failure path is a bug.

### Memory & Views
- **No raw `new`/`delete`** outside of explicit, documented ownership-transfer contexts. All ownership must be expressed through `std::unique_ptr` or `std::shared_ptr`.
- **`std::span` & `std::string_view`:** Prefer over `const std::vector<T>&`, `const std::string&`, or pointer-size pairs for non-owning reads of contiguous memory.
- **Zero raw `new`/`delete` rule:** If an allocation cannot be expressed as a smart pointer, document why with a comment and pair it with an explicit `extern "C"` free endpoint.

### Const Correctness
- **`constexpr` / `consteval` everything** that can be evaluated at compile time.
- **`noexcept` is mandatory** on all FFI-boundary functions and all hot-path functions (per-request event loop). An FFI function that throws will hard-crash the process.

### Public API Strictness
- **No `auto` return type on public API functions** without a trailing return type annotation. `auto foo()` is banned on any function in a public header — callers must be able to see the return type without inference.

### Templates & Generic Programming
- **Use Concepts over SFINAE.** Constrain templates with `requires` clauses and C++20/23 Concepts instead of `std::enable_if`.
- **Deducing `this` (C++23):** Use explicit object parameters (`this auto&& self`) to de-duplicate `const` and non-`const` method overloads.

### Loops & Algorithms
- **`std::ranges` / `std::views` over raw loops.** Use composable views: `std::views::filter`, `std::views::transform`, `std::views::take`, etc.

### Strings & Formatting
- **`std::format` and `std::print`** for string building and output. Replace `<iostream>` and `<cstdio>` wrappers entirely.

---

## 5. Coroutine Rules (`Task<T>`)

### 5.1 Lifetime Management & Dangling References
- **Pass by value inside coroutines.** Capture parameters and locals by value in coroutine lambdas (`[=]() -> Task<void>`). A coroutine outlives its caller — capturing by reference (`[&]`) causes use-after-free when the caller's stack frame is destroyed at the first `co_await`.
- **`this` in member coroutines:** Ensure the object outlives the coroutine. Use `std::shared_ptr` or `std::weak_ptr` when binding coroutines to long-lived networking operations.

### 5.2 Non-Blocking Enforcement
- **Never block the event loop.** `net::IoContext` is a high-performance async I/O abstraction — blocking inside a `Task<T>` starves the thread pool.
    - ❌ `std::this_thread::sleep_for(...)`
    - ❌ `std::mutex::lock()`
    - ❌ Synchronous I/O: `std::ifstream::read()`, `recv()`
    - ✅ `co_await coro::sleep(ms)`
    - ✅ `co_await net::Socket::async_read(...)`

### 5.3 `co_await` on Void Awaitables
- **Do not silently discard `co_await` on void-returning awaitables** without an explicit justification comment. Fire-and-forget coroutines must be typed `Task<void>` and terminated with `co_return;`.

### 5.4 Exception Safety
- **Prefer `std::expected` over exceptions** on the hot path.
- **Wrap `co_await` calls** in `try/catch` where throwing is unavoidable. An unhandled exception inside a detached coroutine terminates the process.

### 5.5 Generator Exhaustion
- Callers of `coro::Generator<T>` must handle early termination correctly to avoid resource leaks in the generator frame.

---

## 6. Performance & Memory (Hot Path)

### 6.1 Zero-Copy String Operations
- **`std::string_view` over `std::string`** for read-only string inspection (HTTP headers, URI paths, regex captures). Never allocate intermediate `std::string` objects on the hot path.
- **`std::span<uint8_t>` over `std::vector`** when reading socket chunks or processing byte buffers.
- **String concatenation:** Use `std::format` or pre-allocate with `.reserve()` before appending to prevent repeated heap reallocations.

### 6.2 SIMD JSON
- **In-place parsing:** Use the integrated `simdjson` module. Parse JSON text in-place without copying the raw string.
- **On-demand traversal:** Do not deserialize to `nlohmann::json` DOM when only a single field is needed — use `simdjson::ondemand` directly.

### 6.3 Middleware & Routing Hot Path
- **Pre-compilation:** Middleware chains must be fully initialized at application startup (`CompiledMiddlewareChain`). Never construct or allocate new chain components inside the `Router` match sequence.
- **DFA Router:** The `RegexMatcher` DFA processes requests with zero dynamic allocation. Handler callbacks must capture dependencies by reference to application-state objects, not by copying large state into each routing thunk.

### 6.4 Move Semantics
- **Enforce `std::move`** when transferring heavy objects (`std::vector` buffers, `Response` structures, payload body strings). Never copy where a move is possible.
- **Return Value Optimization (RVO):** Build response objects directly in the return statement to let the compiler elide copies.
