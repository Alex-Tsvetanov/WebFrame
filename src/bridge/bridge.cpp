#include "coroute/core/app.hpp"
#include "coroute/core/callback_fetch_transport.hpp"
#include <cstring>
#include <iostream>
#include <thread>
#include <nlohmann/json.hpp>

// Define the main function that will be renamed by preprocessor
// The build system must define -Dmain=app_main when compiling the user's main.cpp
extern int app_main();

// ---------------------------------------------------------------------------
// Global state (file-scope, not inside extern "C" so lambdas can capture them)
// ---------------------------------------------------------------------------

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
static std::thread app_thread;

// Fetch callback set by Dart via register_fetch_callback
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
static coroute::FetchRequestCallback g_fetch_callback = nullptr;

// Raw pointer to the transport so complete_fetch_request can reach it
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
static coroute::CallbackFetchTransport* g_transport = nullptr;

// Broadcast callback set by Dart via register_broadcast_callback
using BroadcastCallback = void (*)(const char* json);
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
static BroadcastCallback g_broadcast_callback = nullptr;

// View/submit response callback set by Dart via register_view_callback
using ViewResponseCallback = void (*)(int32_t req_id, const char* json);
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
static ViewResponseCallback g_view_callback = nullptr;

// ---------------------------------------------------------------------------
// Platform export macro
// ---------------------------------------------------------------------------

#if defined(_WIN32)
#define CROSS_EXPORT __declspec(dllexport)
#elif defined(__GNUC__)
#define CROSS_EXPORT __attribute__((visibility("default")))
#else
#define CROSS_EXPORT
#endif

// ---------------------------------------------------------------------------
// Async coroutine helpers (file-scope, not exported)
// ---------------------------------------------------------------------------

static coroute::Task<void> perform_async_view_request(coroute::App* app, std::string route,
                                                      std::string headers_json, int32_t req_id)
{
	std::string result_json;
	// NOLINTBEGIN(fuchsia-default-arguments-calls)
	try
	{
		std::cout << "[Bridge] Requesting view: " << route << "\n";
		std::cout << "[Bridge] Headers JSON: " << headers_json << "\n";
		std::unordered_map<std::string, std::string> headers;
		if (!headers_json.empty())
		{
			try
			{
				auto j = nlohmann::json::parse(headers_json);
				for (auto& el : j.items())
				{
					if (el.value().is_string())
					{
						std::cout << "[Bridge] Adding header: " << el.key() << " = "
								  << el.value().get<std::string>() << "\n";
						headers[el.key()] = el.value().get<std::string>();
					}
				}
			}
			catch (const std::exception& e)
			{
				std::cerr << "[Bridge] JSON Parse Error: " << e.what() << "\n";
			}
		}
		auto resp = co_await app->fetch(coroute::HttpMethod::GET, route, "", headers);

		nlohmann::json ffi_resp;
		ffi_resp["_coroute_ffi_response"] = true;
		ffi_resp["status"] = resp.status();
		ffi_resp["headers"] = nlohmann::json::object();
		for (auto& h : resp.headers())
		{
			std::string name = std::string(h.first);
			if (!ffi_resp["headers"].contains(name))
			{
				ffi_resp["headers"][name] = nlohmann::json::array();
			}
			ffi_resp["headers"][name].push_back(std::string(h.second));
		}
		ffi_resp["body"] = std::string(resp.body());
		result_json = ffi_resp.dump();
	}
	catch (const std::exception& e)
	{
		nlohmann::json err;
		err["_coroute_ffi_response"] = true;
		err["status"] = 500;
		err["body"] = std::string(R"({"error": "Exception: )") + e.what() + R"("})";
		result_json = err.dump();
	}
	catch (...)
	{
		nlohmann::json err;
		err["_coroute_ffi_response"] = true;
		err["status"] = 500;
		err["body"] = R"({"error": "Unknown exception"})";
		result_json = err.dump();
	}

	if (g_view_callback)
	{
		char* json_cstr = strdup(result_json.c_str());
		g_view_callback(req_id, json_cstr);
	}
}

static coroute::Task<void> perform_async_submit(coroute::App* app, std::string route,
                                                std::string json_data, std::string headers_json,
                                                int32_t req_id)
{
	std::string result_json;
	try
	{
		std::unordered_map<std::string, std::string> headers;
		if (!headers_json.empty())
		{
			try
			{
				auto j = nlohmann::json::parse(headers_json);
				for (auto& el : j.items())
				{
					if (el.value().is_string()) headers[el.key()] = el.value().get<std::string>();
				}
			}
			catch (const std::exception& e)
			{
				std::cerr << "[Bridge] Header parse error: " << e.what() << "\n";
			}
		}
		auto resp = co_await app->fetch(coroute::HttpMethod::POST, route, std::move(json_data), headers);

		nlohmann::json ffi_resp;
		ffi_resp["_coroute_ffi_response"] = true;
		ffi_resp["status"] = resp.status();
		ffi_resp["headers"] = nlohmann::json::object();
		for (auto& h : resp.headers())
		{
			std::string name = std::string(h.first);
			if (!ffi_resp["headers"].contains(name))
			{
				ffi_resp["headers"][name] = nlohmann::json::array();
			}
			ffi_resp["headers"][name].push_back(std::string(h.second));
		}
		ffi_resp["body"] = std::string(resp.body());
		result_json = ffi_resp.dump();
	}
	catch (const std::exception& e)
	{
		nlohmann::json err;
		err["_coroute_ffi_response"] = true;
		err["status"] = 500;
		err["body"] = std::string(R"({"error": "Exception: )") + e.what() + R"("})";
		result_json = err.dump();
	}
	catch (...)
	{
		nlohmann::json err;
		err["_coroute_ffi_response"] = true;
		err["status"] = 500;
		err["body"] = R"({"error": "Unknown exception"})";
		result_json = err.dump();
	}

	if (g_view_callback)
	{
		char* json_cstr = strdup(result_json.c_str());
		g_view_callback(req_id, json_cstr);
	}
}

// ---------------------------------------------------------------------------
// Exported C API
// ---------------------------------------------------------------------------

extern "C"
{
	// Register the fetch callback (called by Dart on startup)
	CROSS_EXPORT void register_fetch_callback(coroute::FetchRequestCallback callback) noexcept
	{
		std::cout << "[Bridge] Registering fetch callback\n";
		g_fetch_callback = callback;
	}

	// Complete a pending fetch request (called by Dart when HTTP request finishes)
	CROSS_EXPORT void complete_fetch_request(int32_t req_id, int status, const char* body) noexcept
	{
		if (g_transport)
		{
			g_transport->complete_request(req_id, status, body);
		}
	}

	// Register the broadcast callback (called by Dart on startup).
	// Forwarded to App::set_broadcast_callback() so project code calling
	// app.fire_broadcast() reaches Flutter without any project-specific FFI exports.
	CROSS_EXPORT void register_broadcast_callback(BroadcastCallback callback) noexcept
	{
		std::cout << "[Bridge] Registering broadcast callback\n";
		g_broadcast_callback = callback;
		// If App is already alive (late registration), wire it immediately.
		auto* app = coroute::App::instance();
		if (app && callback)
		{
			auto* cb = callback;
			app->set_broadcast_callback(
			    [cb](std::string_view msg)
			    {
				    char* cstr = strdup(std::string(msg).c_str());
				    cb(cstr);
			    });
		}
	}

	// Register the view response callback (called by Dart on startup)
	CROSS_EXPORT void register_view_callback(ViewResponseCallback callback) noexcept
	{
		std::cout << "[Bridge] Registering view callback\n";
		g_view_callback = callback;
	}

	// Initialize the application (starts app_main in a background thread)
	CROSS_EXPORT void init_app() noexcept
	{
		std::cout << "[Bridge] init_app called\n";

		app_thread = std::thread(
			[]()
			{
				std::cout << "[Bridge] Calling app_main()...\n";

				// Poll for the App instance then inject transport + broadcast callback.
				// app.run() blocks so we use a detached injector thread.
				std::thread injector(
					[]()
					{
						int retries = 0;
						while (retries < 20)
						{
							auto* app = coroute::App::instance();
							if (app)
							{
								if (g_fetch_callback)
								{
									std::cout << "[Bridge] Injecting Fetch Transport...\n";
									auto transport = std::make_unique<coroute::CallbackFetchTransport>(
									    g_fetch_callback);
									g_transport = transport.get();
									app->set_fetch_transport(std::move(transport));
								}
								if (g_broadcast_callback)
								{
									std::cout << "[Bridge] Injecting Broadcast Callback...\n";
									auto* cb = g_broadcast_callback;
									app->set_broadcast_callback(
									    [cb](std::string_view msg)
									    {
										    char* cstr = strdup(std::string(msg).c_str());
										    cb(cstr);
									    });
								}
								break;
							}
							std::this_thread::sleep_for(std::chrono::milliseconds(100));
							retries++;
						}
					});
				injector.detach();

				app_main();
				std::cout << "[Bridge] app_main() returned.\n";
			});

		app_thread.detach();
	}

	// Return the configured API base URL (e.g. "http://localhost:8080").
	// Dart reads this once at startup via get_api_base_url() and uses it in
	// _doFetch() instead of the hardcoded 127.0.0.1:8080 fallback.
	// Returns a strdup'd C string; bridge.dart must free it with malloc.free().
	CROSS_EXPORT const char* get_api_base_url() noexcept
	{
		auto* app = coroute::App::instance();
		if (!app)
		{
			return strdup("http://localhost:8080");
		}
		return strdup(app->fetch_base_url().c_str());
	}

	// Request a view asynchronously
	CROSS_EXPORT void request_view_async(int32_t req_id, const char* route, const char* headers_json) noexcept
	{
		auto* app = coroute::App::instance();
		if (!app || !app->io_context())
		{
			if (g_view_callback)
			{
				g_view_callback(req_id, "{\"error\": \"App not initialized\"}");
			}
			return;
		}

		std::cout << "[Bridge] Requesting view async: " << route << " (ID: " << req_id << ")\n";
		std::string route_str = route;
		std::string hdr_str = headers_json ? headers_json : "";

		app->io_context()->post([app, route_str, hdr_str, req_id]()
		                        { perform_async_view_request(app, route_str, hdr_str, req_id).start_detached(); });
	}

	CROSS_EXPORT void submit_action_async(int32_t req_id, const char* route, const char* json_data,
	                                      const char* headers_json) noexcept
	{
		auto* app = coroute::App::instance();
		if (!app || !app->io_context())
		{
			if (g_view_callback)
			{
				g_view_callback(req_id, "{\"error\": \"App not initialized\"}");
			}
			return;
		}

		std::cout << "[Bridge] Submit action async: " << route << " (ID: " << req_id << ")\n";
		std::string route_str = route;
		std::string json_str = json_data ? json_data : "";
		std::string hdr_str = headers_json ? headers_json : "";

		app->io_context()->post([app, route_str, json_str, hdr_str, req_id]()
		                        { perform_async_submit(app, route_str, json_str, hdr_str, req_id).start_detached(); });
	}

}  // extern "C"
