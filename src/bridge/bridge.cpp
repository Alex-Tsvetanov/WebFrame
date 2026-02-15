#include "coroute/core/app.hpp"
#include <cstring>
#include <iostream>
#include <future>
#include <thread>
#include <vector>

// Define the main function that will be renamed by preprocessor
// The build system must define -Dmain=app_main when compiling the user's main.cpp
extern int app_main();

// Global thread to run the app
std::thread app_thread;

#include "coroute/core/callback_fetch_transport.hpp"

// Global callback (set by Dart)
static coroute::FetchRequestCallback g_fetch_callback = nullptr;

// Global transport instance (managed by App, but we need a reference for completion)
static coroute::CallbackFetchTransport* g_transport = nullptr;

extern "C" {

// Register the fetch callback (called by Dart on startup)
void register_fetch_callback(coroute::FetchRequestCallback callback) {
	std::cout << "[Bridge] Registering fetch callback" << std::endl;
	g_fetch_callback = callback;
}

// Complete a pending fetch request (called by Dart when HTTP request finishes)
void complete_fetch_request(int32_t req_id, int status, const char* body) {
	if (g_transport) {
		g_transport->complete_request(req_id, status, body);
	}
}

// Initialize the application (starts app_main in a background thread)
void init_app() {
	std::cout << "[Bridge] init_app called" << std::endl;

	// Start the app in a separate thread because app.run() blocks
	app_thread = std::thread([]() {
		std::cout << "[Bridge] Calling app_main()..." << std::endl;

		// Wait for App instance to be created (main.cpp does this)
		// This is a race condition if we try to set transport immediately.
		// Better approach: We modify App::App() or main.cpp? No, we can set it
		// after main() starts but before it runs?
		// Actually, main() blocks on run().
		// We need a hook or polling?

		// BETTER: We rely on main.cpp using App::instance().
		// We can poll for instance briefly.

		// Start main on THIS thread, but we wanted to inject transport...
		// If we run main(), it blocks until shutdown.
		// So we can't inject transport *after* run() starts easily unless run() is async
		// or we do it before run().

		// Strategy: We can't easily modify main.cpp from here without hooks.
		// However, main.cpp likely sets up App and then calls run().
		// If we execute main(), it does everything.

		// ALTERNATIVE: main.cpp should check `bridge` or similar? No.

		// Let's modify `app_main` to be `app_main(std::function<void(App&)> setup)`.
		// Or simpler: We just launch main. Since `App` singleton is static,
		// we can access it from another thread? YES.

		// So:
		// 1. Launch main in thread.
		// 2. Poll for App::instance() to be non-null.
		// 3. Set transport.
		// 4. (Hope main hasn't used fetch yet? In client mode it enters loop immediately).
		// 5. App::run loop uses fetch on demand, so this is safe!

		std::thread injector([]() {
			int retries = 0;
			while (retries < 20) {
				auto* app = coroute::App::instance();
				if (app) {
					if (g_fetch_callback) {
						std::cout << "[Bridge] Injecting Fetch Transport..." << std::endl;
						auto transport = std::make_unique<coroute::CallbackFetchTransport>(g_fetch_callback);
						g_transport = transport.get();  // Save raw pointer for completion
						app->set_fetch_transport(std::move(transport));
					}
					break;
				}
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
				retries++;
			}
		});
		injector.detach();

		app_main();
		std::cout << "[Bridge] app_main() returned." << std::endl;
	});

	// Detach or join? We want it running.
	app_thread.detach();
}

// Global view callback (set by Dart)
using ViewResponseCallback = void (*)(int32_t req_id, const char* json);
static ViewResponseCallback g_view_callback = nullptr;

// Register the view callback
void register_view_callback(ViewResponseCallback callback) {
	std::cout << "[Bridge] Registering view callback" << std::endl;
	g_view_callback = callback;
}

// ... existing register_fetch_callback ...

// Helper coroutine to ensure arguments are captured by value in the frame
static coroute::Task<void> perform_async_view_request(coroute::App* app, std::string route, int32_t req_id) {
	std::string result_json;
	try {
		auto resp = co_await app->fetch_get(route);
		result_json = std::string(resp.body());
	} catch (const std::exception& e) {
		result_json = std::string("{\"error\": \"Exception: ") + e.what() + "\"}";
	} catch (...) {
		result_json = "{\"error\": \"Unknown exception\"}";
	}

	if (g_view_callback) {
		// Must allocate on heap because NativeCallable via listener is async
		// and result_json stack variable will be destroyed.
		// Dart side must free this string!
		char* json_cstr = strdup(result_json.c_str());
		g_view_callback(req_id, json_cstr);
	}
}

// Request a view asynchronously
// Dart passes an ID, checking the result via callback later
void request_view_async(int32_t req_id, const char* route) {
	auto* app = coroute::App::instance();
	if (!app || !app->io_context()) {
		if (g_view_callback) {
			g_view_callback(req_id, "{\"error\": \"App not initialized\"}");
		}
		return;
	}

	std::cout << "[Bridge] Requesting view async: " << route << " (ID: " << req_id << ")" << std::endl;
	std::string route_str = route;

	// Post task to IO context
	app->io_context()->post(
	    [app, route_str, req_id]() { perform_async_view_request(app, route_str, req_id).start_detached(); });
}

// Submit an action (POST/PUT/etc)
// Helper coroutine for submit_action (async now)
static coroute::Task<void> perform_async_submit(coroute::App* app, std::string route, std::string json_data,
                                                int32_t req_id) {
	std::string result_json;
	try {
		// We assume submit = POST for this demo
		// In reality, we might want to support PUT/DELETE or infer from route
		auto resp = co_await app->fetch_post(route, json_data);
		result_json = std::string(resp.body());
	} catch (const std::exception& e) {
		result_json = std::string("{\"error\": \"Exception: ") + e.what() + "\"}";
	} catch (...) {
		result_json = "{\"error\": \"Unknown exception\"}";
	}

	if (g_view_callback) {
		// reuse view callback for now, or add a specific action callback?
		// simplest: treat action response as a "view" update or just a JSON response
		// But main.dart expects synchronous String result from submitAction currently.
		// We need to change bridge.dart to be async for submitAction too.
		char* json_cstr = strdup(result_json.c_str());
		g_view_callback(req_id, json_cstr);
	}
}

// Submit an action (POST/PUT/etc) - ASYNC
void submit_action_async(int32_t req_id, const char* route, const char* json_data) {
	auto* app = coroute::App::instance();
	if (!app || !app->io_context()) {
		if (g_view_callback) {
			g_view_callback(req_id, "{\"error\": \"App not initialized\"}");
		}
		return;
	}

	std::cout << "[Bridge] Submit action async: " << route << " (ID: " << req_id << ")" << std::endl;
	std::string route_str = route;
	std::string json_str = json_data ? json_data : "";

	app->io_context()->post([app, route_str, json_str, req_id]() {
		perform_async_submit(app, route_str, json_str, req_id).start_detached();
	});
}

}  // extern "C"
