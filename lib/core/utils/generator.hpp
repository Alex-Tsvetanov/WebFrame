#pragma once

#if (!defined(__cpp_lib_coroutine) && !defined(__cpp_impl_coroutine))
#include <experimental/coroutine>
namespace coro_std = std::experimental;
#else
#include <coroutine>
namespace coro_std = std;
#endif

#include <optional>

namespace webframe::utils {

	template <typename T>
	struct generator {
		struct promise_type;
		using handle_type = coro_std::coroutine_handle<promise_type>;

		generator(handle_type h) : coro(h) {}

		handle_type coro;

		~generator() {
			if (coro) coro.destroy();
		}
		generator(const generator&) = delete;
		generator& operator=(const generator&) = delete;
		generator(generator&& oth) : coro(oth.coro) { oth.coro = nullptr; }
		generator& operator=(generator&& oth) {
			coro = oth.coro;
			oth.coro = nullptr;
			return *this;
		}
		std::optional<T> getNextValue() {
			if (!coro || coro.done()) return std::nullopt;
			coro.resume();
			if (!coro.promise().value_ready) return std::nullopt;
			coro.promise().value_ready = false;
			return coro.promise().current_value;
		}

		struct promise_type {
			promise_type() {}

			~promise_type() {}

			coro_std::suspend_always initial_suspend() { return {}; }
			coro_std::suspend_always final_suspend() noexcept { return {}; }
			auto get_return_object() { return generator{handle_type::from_promise(*this)}; }

			coro_std::suspend_always yield_value(const T value) {
				current_value = value;
				value_ready = true;
				return {};
			}

			void return_void() {}
			void unhandled_exception() { std::exit(1); }

			T current_value;
			bool value_ready = false;
		};
	};
}  // namespace webframe::utils