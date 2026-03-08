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

#include <nlohmann/json.hpp>

namespace coroute
{

	// C-compatible callback types for FFI
	using FetchRequestCallback = void (*)(int32_t req_id, const char* url, const char* method, const char* headers,
	                                      const char* body);

	class CallbackFetchTransport : public FetchTransport
	{
	public:
		explicit CallbackFetchTransport(FetchRequestCallback callback) : callback_(callback) { }

		Task<Response> dispatch(const Request& req) override
		{
			// IMPORTANT: Eagerly copy all data we need from `req` NOW, before the
			// coroutine suspends. `req` is a reference to a coroutine parameter,
			// which becomes a dangling reference after the first suspension point.
			auto req_id = next_req_id_++;
			std::string path = std::string(req.path());
			std::string method = std::string(method_to_string(req.method()));
			std::string body = std::string(req.body());

			// Serialize headers to JSON
			nlohmann::json hj = nlohmann::json::object();
			for (auto& h : req.headers())
			{
				hj[std::string(h.first)] = std::string(h.second);
			}
			std::string headers_json = hj.dump();

			struct Awaiter
			{
				CallbackFetchTransport& transport;
				int32_t req_id;
				// Store values, not references — these outlive the suspension.
				std::string path;
				std::string method;
				std::string headers_json;
				std::string body;
				Response result;

				bool await_ready() { return false; }

				void await_suspend(std::coroutine_handle<> h)
				{
					transport.register_pending(req_id, h, &result);

					// strdup so Dart's async NativeCallable.listener receives
					// heap-stable memory it can safely read after this frame is gone.
					char* path_dup = strdup(path.c_str());
					char* method_dup = strdup(method.c_str());
					char* headers_dup = strdup(headers_json.c_str());
					char* body_dup = strdup(body.c_str());

					transport.callback_(req_id, path_dup, method_dup, headers_dup, body_dup);
					// Dart frees these via malloc.free() after toDartString().
				}

				Response await_resume() { return std::move(result); }
			};

			co_return co_await Awaiter{.transport = *this,
			                           .req_id = req_id,
			                           .path = std::move(path),
			                           .method = std::move(method),
			                           .headers_json = std::move(headers_json),
			                           .body = std::move(body)};
		}

		// Called by FFI when response is ready
		void complete_request(int32_t req_id, int status, const char* body)
		{
			std::lock_guard lock(mutex_);
			auto it = pending_.find(req_id);
			if (it != pending_.end())
			{
				auto text = std::string(body);
				// Construct response
				*it->second.result_ptr = Response(status,
				                                  {
													  {"Content-Type", "application/json"}
                },
				                                  text);

				auto handle = it->second.handle;
				pending_.erase(it);

				// Resume the coroutine
				handle.resume();
			}
		}

	private:
		struct PendingRequest
		{
			std::coroutine_handle<> handle;
			Response* result_ptr;
		};

		void register_pending(int32_t req_id, std::coroutine_handle<> h, Response* r)
		{
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
