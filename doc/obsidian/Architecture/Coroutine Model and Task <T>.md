---
title: Coroutine Model and Task<T>
tags:
  - coroute
  - architecture
  - coroutines
  - task
aliases:
  - Task<T> Implementation
  - Coroutine Lifecycle
---

# Coroutine Model and `Task<T>`

> [!abstract]
> The Heart of Coroute is its C++20 coroutine implementation. This document details the `Task<T>` type, its internal promise machinery, and how the library ensures memory safety in an asynchronous, multi-threaded environment.

## The `Task<T>` Type

`Task<T>` is a "lazy" coroutine type. Unlike some asynchronous primitives (like `std::future`), the coroutine body does not begin execution until it is explicitly `co_await`-ed or started via `start_detached()`.

Relevant files:
- `[[include/coroute/coro/task.hpp]]`
- `[[include/coroute/coro/awaiter.hpp]]`

### Modular Design

The implementation is split into several key components:
1. **`Task<T>`**: The handle-like object returned by any `async` function.
2. **`TaskPromise<T>`**: The internal state that manages the result and continuation of the coroutine.
3. **`FinalAwaiter`**: Controls what happens when the coroutine finishes (e.g., resuming the caller or self-destruction).
4. **`Awaiter`**: Implements the `co_await` operator.

---

## Internal Machinery

### 1. Promise Type (`TaskPromiseBase`)

The promise type is the controller of the coroutine. It handles the initial suspension, exceptions, and the result storage.

```cpp
struct TaskPromiseBase {
    std::coroutine_handle<> continuation_ = std::noop_coroutine();
    std::exception_ptr exception_;
    CancellationToken cancel_token_;
    bool detached_ = false;

    // Lazy start: suspend initially
    std::suspend_always initial_suspend() noexcept { return {}; }
    
    // Custom final suspension logic
    FinalAwaiter final_suspend() noexcept { return {}; }
    
    void unhandled_exception() noexcept {
        exception_ = std::current_exception();
    }
};
```

### 2. FinalAwaiter

When a coroutine reaches its end, `final_suspend` returns a `FinalAwaiter`. This is critical for **detached tasks** (fire-and-forget).

```cpp
struct FinalAwaiter {
    template<typename Promise>
    std::coroutine_handle<> await_suspend(std::coroutine_handle<Promise> h) noexcept {
        auto& promise = h.promise();
        
        // If detached, destroy the coroutine state immediately
        if (promise.detached_) {
            h.destroy();
            return std::noop_coroutine();
        }
        
        // Resume the parent coroutine that was awaiting us
        if (promise.continuation_) {
            return promise.continuation_;
        }
        return std::noop_coroutine();
    }
};
```

### 3. Awaiter

The `Awaiter` is used when you `co_await task;`. It transfers control to the new task and saves the current coroutine as the continuation.

```cpp
struct Awaiter {
    handle_type handle_;

    bool await_ready() const noexcept {
        return !handle_ || handle_.done();
    }

    std::coroutine_handle<> await_suspend(std::coroutine_handle<> continuation) noexcept {
        handle_.promise().continuation_ = continuation;
        return handle_; // Symmetric transfer: direct switch to the new task
    }

    T await_resume() {
        return std::move(handle_.promise()).result();
    }
};
```

---

## Memory Safety and Detached Tasks

Coroute makes heavy use of `start_detached()` to handle new connections. This is powerful but dangerous because the coroutine might outlive the scope where it was created.

### The Problem: Use-After-Free

If a coroutine captures a local variable by reference and then suspends, that variable might be gone when the coroutine wakes up.

```cpp
// DANGEROUS CODE
void app_logic() {
    std::string data = "internal state";
    [&data]() -> Task<void> {
        co_await async_sleep(100ms);
        std::cout << data; // CRASH! 'data' is destroyed.
    }().start_detached();
}
```

### The Solution: Ownership and RAII

Coroute enforces several rules to ensure safety:

1. **Move Semantics**: Everything needed by a detached task must be **moved** into its capture block or arguments.
   ```cpp
   handle_connection(std::move(connection)).start_detached();
   ```
2. **`std::shared_ptr`**: For truly shared state (like application configuration), use shared pointers captured by value.
3. **RAII Guards**: Connection objects are owned by the coroutine and automatically closed/destroyed when the coroutine state is destroyed in `FinalAwaiter`.
4. **Cancellation Tokens**: Detached tasks monitor a `CancellationToken`. During server shutdown, these tokens are tripped, allowing tasks to finish their loop and clean up gracefully.

---

## Performance: Symmetric Transfer

Coroute uses **Symmetric Transfer** (returning a handle from `await_suspend`). This optimization prevents stack overflow when deeply nesting coroutines and allows the compiler to perform tail-call optimization on the state switch. This is significantly faster and more memory-efficient than returning `void` (which involves the scheduler).

---

## Related Notes
- [[Architecture/Server Runtime and OS Backends|Server Runtime and OS Backends]]
- [[Architecture/OS Backends/io_uring|io_uring]]
- [[Architecture/DFA Routing and Middleware|DFA Routing and Middleware]]
