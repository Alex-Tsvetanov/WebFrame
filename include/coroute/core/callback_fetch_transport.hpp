#ifndef COROUTE_CORE_CALLBACK_FETCH_TRANSPORT_HPP
#define COROUTE_CORE_CALLBACK_FETCH_TRANSPORT_HPP

#include "coroute/core/fetch_transport.hpp"
#include "coroute/core/response.hpp"
#include "coroute/coro/task.hpp"
#include <functional>
#include <map>
#include <mutex>
#include <atomic>
#include <coroutine>
#include <string>

namespace coroute {

	// C-compatible callback types for FFI
	using FetchRequestCallback = void (*)(int32_t req_id, const char* url, const char* method, const char* headers,
	                                      const char* body);

	class CallbackFetchTransport : public FetchTransport {
	public:
		CallbackFetchTransport(FetchRequestCallback callback) : callback_(callback) {}

		Task<Response> dispatch(const Request& req) override {
			// Create a promise for the result
			auto req_id = next_req_id_++;

			struct Awaiter {
				CallbackFetchTransport& transport;
				int32_t req_id;
				const Request& req;  // Store reference to request
				Response result;

				// Default constructor required? No, aggregate init.

				bool await_ready() { return false; }

				void await_suspend(std::coroutine_handle<> h) {
					transport.register_pending(req_id, h, &result);
					// Trigger the callback

					// Copy to std::string to ensure null-termination and memory safety for C-API
					// Use direct initialization because string_view constructor is explicit
					std::string path_str(req.path());
					std::string method_str(method_to_string(req.method()));
					std::string body_str(req.body());

					transport.callback_(req_id, path_str.c_str(), method_str.c_str(), "", body_str.c_str());
				}

				Response await_resume() { return std::move(result); }
			};

			// Initialize Awaiter with explicit members (aggregate initialization)
			// Note: Response result is default constructed.
			co_return co_await Awaiter{.transport = *this, .req_id = req_id, .req = req};
		}

		// Called by FFI when response is ready
		void complete_request(int32_t req_id, int status, const char* body) {
			std::lock_guard lock(mutex_);
			auto it = pending_.find(req_id);
			if (it != pending_.end()) {
				auto text = std::string(body);
				// Construct response
				*it->second.result_ptr = Response(status, {{"Content-Type", "application/json"}}, text);

				auto handle = it->second.handle;
				pending_.erase(it);

				// Resume the coroutine
				handle.resume();
			}
		}

	private:
		struct PendingRequest {
			std::coroutine_handle<> handle;
			Response* result_ptr;
		};

		void register_pending(int32_t req_id, std::coroutine_handle<> h, Response* r) {
			std::lock_guard lock(mutex_);
			pending_[req_id] = {h, r};
		}

		FetchRequestCallback callback_;
		std::atomic<int32_t> next_req_id_{1};
		std::mutex mutex_;
		std::map<int32_t, PendingRequest> pending_;
	};

}  // namespace coroute

#endif  // COROUTE_CORE_CALLBACK_FETCH_TRANSPORT_HPP
