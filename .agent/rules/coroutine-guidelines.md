# C++20 Coroutine Guidelines for Coroute v2

As this framework heavily utilizes C++20 coroutines (`Task<T>`) for asynchronous I/O and routing, adherence to the following rules is critical to prevent memory corruption, dangling references, and thread pool starvation.

## 1. Lifetime Management & Dangling References
- **Pass by Value for Coroutines:** Always capture parameters and local variables by value in coroutines, especially in lambda coroutines (`[=]() -> Task<void>`). A coroutine outlives its caller, and capturing by reference (`[&]`) will result in use-after-free errors as the caller's stack frame is destroyed when the coroutine suspends (`co_await`).
- **Careful with `this`:** When a class member function is a coroutine, ensure the object (`this`) outlives the coroutine execution. Use `std::shared_ptr` or `weak_ptr` when binding coroutines to long-lived networking operations.

## 2. Non-blocking Enforcement
- **Never Block the Event Loop:** Coroute uses high-performance I/O abstraction (`net::IoContext`). Never use blocking operations inside a `Task<T>`. 
    - ❌ `std::this_thread::sleep_for(...)`
    - ❌ `std::mutex::lock()`
    - ❌ Synchronous file/socket I/O (`std::ifstream::read()`, `recv()`)
- **Use Awaitables:** Always use the framework-provided `co_await` equivalents (e.g., `co_await coro::sleep(ms)`, async file streams, or `net::Socket::async_read`).

## 3. Exception Safety
- **Yielding Exceptions:** When an error occurs deeply within a coroutine, prefer returning `std::expected` or using specific framework error types over throwing exceptions if performance is a concern on the hot path.
- **Catch Contexts:** If throwing is unavoidable, ensure `try/catch` surrounds the `co_await` call. An unhandled exception inside a detached coroutine will terminate the process.

## 4. `co_return` & `co_await` Semantics
- **Void Coroutines:** For Fire-and-Forget tasks, explicitly return `Task<void>` and finish with `co_return;`.
- **Generator Exhaustion:** If using `coro::Generator`, ensure the caller properly handles early termination to avoid resource leaks in the generator frame.
