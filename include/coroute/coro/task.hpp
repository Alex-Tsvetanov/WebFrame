#pragma once

#include <coroutine>
#include <exception>
#include <utility>
#include <variant>
#include <optional>
#include <atomic>

#include "coroute/util/expected.hpp"
#include "coroute/core/error.hpp"
#include "coroute/coro/cancellation.hpp"

namespace coroute {

// Forward declarations
template<typename T = void>
class Task;

namespace detail {

// ============================================================================
// Task Promise Base
// ============================================================================

struct TaskPromiseBase {
    std::coroutine_handle<> continuation_ = std::noop_coroutine();
    std::exception_ptr exception_;
    CancellationToken cancel_token_;
    bool detached_ = false;  // If true, self-destroy on completion

    struct FinalAwaiter {
        bool await_ready() const noexcept { return false; }
        
        template<typename Promise>
        std::coroutine_handle<> await_suspend(std::coroutine_handle<Promise> h) noexcept {
            auto& promise = h.promise();
            
            // If detached, destroy ourselves
            if (promise.detached_) {
                h.destroy();
                return std::noop_coroutine();
            }
            
            if (promise.continuation_) {
                return promise.continuation_;
            }
            return std::noop_coroutine();
        }
        
        void await_resume() noexcept {}
    };

    std::suspend_always initial_suspend() noexcept { return {}; }
    FinalAwaiter final_suspend() noexcept { return {}; }
    
    void detach() noexcept { detached_ = true; }

    void unhandled_exception() noexcept {
        exception_ = std::current_exception();
    }

    void set_cancellation_token(CancellationToken token) {
        cancel_token_ = std::move(token);
    }

    bool is_cancelled() const noexcept {
        return cancel_token_.is_cancelled();
    }
};

// ============================================================================
// Task Promise for T
// ============================================================================

template<typename T>
struct TaskPromise : TaskPromiseBase {
    std::optional<T> value_;

    Task<T> get_return_object() noexcept;

    template<typename U>
        requires std::convertible_to<U, T>
    void return_value(U&& value) noexcept(std::is_nothrow_constructible_v<T, U>) {
        value_.emplace(std::forward<U>(value));
    }

    T& result() & {
        if (exception_) std::rethrow_exception(exception_);
        return *value_;
    }

    T&& result() && {
        if (exception_) std::rethrow_exception(exception_);
        return std::move(*value_);
    }
};

// ============================================================================
// Task Promise for void
// ============================================================================

template<>
struct TaskPromise<void> : TaskPromiseBase {
    Task<void> get_return_object() noexcept;

    void return_void() noexcept {}

    void result() {
        if (exception_) std::rethrow_exception(exception_);
    }
};

} // namespace detail

// ============================================================================
// Task<T> - Main coroutine type
// ============================================================================

template<typename T>
class [[nodiscard]] Task {
public:
    using promise_type = detail::TaskPromise<T>;
    using handle_type = std::coroutine_handle<promise_type>;

private:
    handle_type handle_;

public:
    Task() noexcept : handle_(nullptr) {}
    
    explicit Task(handle_type h) noexcept : handle_(h) {}
    
    Task(Task&& other) noexcept : handle_(other.handle_) {
        other.handle_ = nullptr;
    }
    
    Task& operator=(Task&& other) noexcept {
        if (this != &other) {
            if (handle_) handle_.destroy();
            handle_ = other.handle_;
            other.handle_ = nullptr;
        }
        return *this;
    }
    
    Task(const Task&) = delete;
    Task& operator=(const Task&) = delete;
    
    ~Task() {
        if (handle_) handle_.destroy();
    }

    // Check if task is valid
    bool valid() const noexcept { return handle_ != nullptr; }
    explicit operator bool() const noexcept { return valid(); }

    // Check if task is done
    bool done() const noexcept { return handle_ && handle_.done(); }

    // Set cancellation token for this task
    void set_cancellation_token(CancellationToken token) {
        if (handle_) {
            handle_.promise().set_cancellation_token(std::move(token));
        }
    }

    // Awaiter for co_await
    struct Awaiter {
        handle_type handle_;

        bool await_ready() const noexcept {
            return !handle_ || handle_.done();
        }

        std::coroutine_handle<> await_suspend(std::coroutine_handle<> continuation) noexcept {
            handle_.promise().continuation_ = continuation;
            return handle_;
        }

        T await_resume() {
            if constexpr (std::is_void_v<T>) {
                handle_.promise().result();
            } else {
                return std::move(handle_.promise()).result();
            }
        }
    };

    Awaiter operator co_await() && noexcept {
        return Awaiter{handle_};
    }

    // Start the task without waiting (detached execution)
    void start() {
        if (handle_ && !handle_.done()) {
            handle_.resume();
        }
    }

    // Get result (blocks if not done - use carefully!)
    T sync_wait() {
        start();
        while (!done()) {
            // Spin - in real code, integrate with event loop
        }
        if constexpr (std::is_void_v<T>) {
            handle_.promise().result();
        } else {
            return std::move(handle_.promise()).result();
        }
    }

    // Release ownership of the handle
    handle_type release() noexcept {
        auto h = handle_;
        handle_ = nullptr;
        return h;
    }
    
    // Detach the task - it will self-destroy when complete
    // Use this for fire-and-forget tasks
    void detach() {
        if (handle_) {
            handle_.promise().detach();
            handle_ = nullptr;  // We no longer own it
        }
    }
    
    // Start and detach in one call
    void start_detached() {
        if (handle_ && !handle_.done()) {
            handle_.promise().detach();
            handle_.resume();
            handle_ = nullptr;  // We no longer own it
        }
    }
};

namespace detail {

template<typename T>
Task<T> TaskPromise<T>::get_return_object() noexcept {
    return Task<T>{std::coroutine_handle<TaskPromise<T>>::from_promise(*this)};
}

inline Task<void> TaskPromise<void>::get_return_object() noexcept {
    return Task<void>{std::coroutine_handle<TaskPromise<void>>::from_promise(*this)};
}

} // namespace detail

// ============================================================================
// Yield - Cooperative scheduling point
// ============================================================================

struct YieldAwaiter {
    bool await_ready() const noexcept { return false; }
    
    void await_suspend(std::coroutine_handle<> h) const noexcept {
        // In a real implementation, this would:
        // 1. Check cancellation
        // 2. Check time slice
        // 3. Possibly reschedule
        // For now, just resume immediately
        h.resume();
    }
    
    void await_resume() const noexcept {}
};

inline YieldAwaiter yield() noexcept {
    return {};
}

// ============================================================================
// CheckCancellation - Awaiter that throws if cancelled
// ============================================================================

template<typename Promise>
struct CheckCancellationAwaiter {
    bool await_ready() const noexcept { return false; }
    
    bool await_suspend(std::coroutine_handle<Promise> h) const noexcept {
        // Check cancellation, don't suspend if not cancelled
        return false;
    }
    
    void await_resume() const {
        // This would check the promise's cancellation token
        // and throw if cancelled
    }
};

// ============================================================================
// Task with expected result (for error handling without exceptions)
// ============================================================================

template<typename T>
using TaskResult = Task<expected<T, Error>>;

template<typename T>
using Result = expected<T, Error>;

} // namespace coroute
