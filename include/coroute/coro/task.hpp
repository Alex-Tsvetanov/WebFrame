#pragma once

#include <coroutine>
#include <exception>
#include <utility>
#include <variant>
#include <optional>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <thread>

#include <functional>

#include "coroute/util/expected.hpp"
#include "coroute/core/error.hpp"
#include "coroute/coro/cancellation.hpp"

namespace coroute
{

	// Forward declarations
	template <typename T = void>
	class Task;

	namespace detail
	{

		// ============================================================================
		// Task Promise Base
		// ============================================================================

		struct TaskPromiseBase
		{
			std::coroutine_handle<> continuation_ = std::noop_coroutine();
			std::exception_ptr exception_;
			CancellationToken cancel_token_;
			bool detached_ = false;              // If true, self-destroy on completion
			std::function<void()> on_complete_;  // Optional completion callback

			struct FinalAwaiter
			{
				static bool await_ready() noexcept { return false; }

				template <typename Promise>
				static std::coroutine_handle<> await_suspend(std::coroutine_handle<Promise> h) noexcept
				{
					auto& promise = h.promise();

					// Invoke completion callback if set
					if (promise.on_complete_)
					{
						promise.on_complete_();
					}

					// If detached, destroy ourselves
					if (promise.detached_)
					{
						h.destroy();
						return std::noop_coroutine();
					}

					if (promise.continuation_)
					{
						return promise.continuation_;
					}
					return std::noop_coroutine();
				}

				static void await_resume() noexcept { }
			};

			static std::suspend_always initial_suspend() noexcept { return {}; }
			static FinalAwaiter final_suspend() noexcept { return {}; }

			void detach() noexcept { detached_ = true; }

			void unhandled_exception() noexcept { exception_ = std::current_exception(); }

			void set_cancellation_token(CancellationToken token) { cancel_token_ = std::move(token); }

			bool is_cancelled() const noexcept { return cancel_token_.is_cancelled(); }
		};

		// ============================================================================
		// Task Promise for T
		// ============================================================================

		template <typename T>
		struct TaskPromise : TaskPromiseBase
		{
			std::optional<T> value_;

			Task<T> get_return_object() noexcept;

			template <typename U>
				requires std::convertible_to<U, T>
			void return_value(U&& value) noexcept(std::is_nothrow_constructible_v<T, U>)
			{
				value_.emplace(std::forward<U>(value));
			}

			T& result() &
			{
				if (exception_) std::rethrow_exception(exception_);
				return *value_;
			}

			T&& result() &&
			{
				if (exception_) std::rethrow_exception(exception_);
				return std::move(*value_);
			}
		};

		// ============================================================================
		// Task Promise for void
		// ============================================================================

		template <>
		struct TaskPromise<void> : TaskPromiseBase
		{
			Task<void> get_return_object() noexcept;

			static void return_void() noexcept { }

			void result()
			{
				if (exception_) std::rethrow_exception(exception_);
			}
		};

	}  // namespace detail

	// ============================================================================
	// Task<T> - Main coroutine type
	// ============================================================================

	template <typename T>
	class [[nodiscard]] Task
	{
	public:
		using promise_type = detail::TaskPromise<T>;
		using handle_type = std::coroutine_handle<promise_type>;

	private:
		handle_type handle_;

	public:
		Task() noexcept : handle_(nullptr) { }

		explicit Task(handle_type h) noexcept : handle_(h) { }

		Task(Task&& other) noexcept : handle_(other.handle_) { other.handle_ = nullptr; }

		Task& operator=(Task&& other) noexcept
		{
			if (this != &other)
			{
				if (handle_) handle_.destroy();
				handle_ = other.handle_;
				other.handle_ = nullptr;
			}
			return *this;
		}

		Task(const Task&) = delete;
		Task& operator=(const Task&) = delete;

		~Task()
		{
			if (handle_) handle_.destroy();
		}

		// Check if task is valid
		bool valid() const noexcept { return handle_ != nullptr; }
		explicit operator bool() const noexcept { return valid(); }

		// Check if task is done
		bool done() const noexcept { return handle_ && handle_.done(); }

		// Set cancellation token for this task
		void set_cancellation_token(CancellationToken token)
		{
			if (handle_)
			{
				handle_.promise().set_cancellation_token(std::move(token));
			}
		}

		// Awaiter for co_await
		struct Awaiter
		{
			handle_type handle_;

			bool await_ready() const noexcept { return !handle_ || handle_.done(); }

			std::coroutine_handle<> await_suspend(std::coroutine_handle<> continuation) noexcept
			{
				handle_.promise().continuation_ = continuation;
				return handle_;
			}

			T await_resume()
			{
				if constexpr (std::is_void_v<T>)
				{
					handle_.promise().result();
				}
				else
				{
					return std::move(handle_.promise()).result();
				}
			}
		};

		Awaiter operator co_await() && noexcept { return Awaiter{handle_}; }

		// Resume the coroutine (caller retains ownership)
		void start()
		{
			if (handle_ && !handle_.done())
			{
				handle_.resume();
			}
		}

		// Synchronously wait for the task to complete.
		// Blocks the calling thread using a condition variable (no busy-wait).
		T sync_wait()
		{
			if (done())
			{
				if constexpr (std::is_void_v<T>)
				{
					handle_.promise().result();
					return;
				}
				else
				{
					return std::move(handle_.promise()).result();
				}
			}

			std::mutex mtx;
			std::condition_variable cv;
			bool finished = false;

			// Install a completion callback that signals the CV
			handle_.promise().on_complete_ = [&]
			{
				{
					std::lock_guard<std::mutex> lock(mtx);
					finished = true;
				}
				cv.notify_one();
			};

			// Kick off the task
			handle_.resume();

			// Block until FinalAwaiter fires the callback
			{
				std::unique_lock<std::mutex> lock(mtx);
				cv.wait(lock, [&] { return finished; });
			}

			// Clear the callback to avoid dangling references
			handle_.promise().on_complete_ = nullptr;

			if constexpr (std::is_void_v<T>)
			{
				handle_.promise().result();
			}
			else
			{
				return std::move(handle_.promise()).result();
			}
		}

		// Release ownership of the handle
		handle_type release() noexcept
		{
			auto h = handle_;
			handle_ = nullptr;
			return h;
		}

		// Detach the task - it will self-destroy when complete
		// Use this for fire-and-forget tasks
		void detach()
		{
			if (handle_)
			{
				handle_.promise().detach();
				handle_ = nullptr;  // We no longer own it
			}
		}

		// Start and detach in one call
		void start_detached()
		{
			if (handle_ && !handle_.done())
			{
				handle_.promise().detach();
				handle_.resume();
				handle_ = nullptr;  // We no longer own it
			}
		}
	};

	namespace detail
	{

		template <typename T>
		Task<T> TaskPromise<T>::get_return_object() noexcept
		{
			return Task<T>{std::coroutine_handle<TaskPromise<T>>::from_promise(*this)};
		}

		inline Task<void> TaskPromise<void>::get_return_object() noexcept
		{
			return Task<void>{std::coroutine_handle<TaskPromise<void>>::from_promise(*this)};
		}

	}  // namespace detail

	// ============================================================================
	// Yield - Cooperative scheduling point
	// ============================================================================

	struct YieldAwaiter
	{
		static bool await_ready() noexcept { return false; }

		static std::coroutine_handle<> await_suspend(std::coroutine_handle<> h) noexcept
		{
			// Yield the OS time slice so other threads can make progress,
			// then resume ourselves.  This is a lightweight cooperative
			// scheduling point that does not require an IoContext.
			std::this_thread::yield();
			return h;  // symmetric transfer: resume immediately after yielding
		}

		static void await_resume() noexcept { }
	};

	inline YieldAwaiter yield() noexcept { return {}; }

	// ============================================================================
	// CheckCancellation - Awaiter that throws if cancelled
	// ============================================================================

	struct CheckCancellationAwaiter
	{
		bool cancelled_ = false;

		static bool await_ready() noexcept { return false; }

		template <typename Promise>
		bool await_suspend(std::coroutine_handle<Promise> h) noexcept
		{
			// Capture cancellation state, then resume immediately (never actually suspend)
			cancelled_ = h.promise().is_cancelled();
			return false;
		}

		void await_resume() const
		{
			if (cancelled_)
			{
				throw Error::cancelled();
			}
		}
	};

	inline CheckCancellationAwaiter check_cancellation() noexcept { return {}; }

	// ============================================================================
	// Task with expected result (for error handling without exceptions)
	// ============================================================================

	template <typename T>
	using TaskResult = Task<expected<T, Error>>;

	template <typename T>
	using Result = expected<T, Error>;

}  // namespace coroute
