// FlutterEnterpriseProject - FFI Bridge
#include "app/config.hpp"
#include "app/server.hpp"
#include <iostream>
#include <string>

#if defined(_WIN32)
#define COROUTE_EXPORT __declspec(dllexport)
#else
#define COROUTE_EXPORT __attribute__((visibility("default")))
#endif

extern "C" {
    COROUTE_EXPORT void* coroute_init(const char* config_json) {
        try {
            // Simplified: In a real app we'd parse the context JSON from Flutter
            auto config = project::Config::from_env();
            auto* server = new project::Server(config);
            
            std::thread([server]() {
                try {
                    server->run();
                } catch (const std::exception& e) {
                    std::cerr << "Server thread error: " << e.what() << "\n";
                }
            }).detach();
            
            // Give it a brief moment to initialize the IO context
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            
            return server;
        } catch (const std::exception& e) {
            std::cerr << "Engine Initialization Error: " << e.what() << "\n";
            return nullptr;
        }
    }

    COROUTE_EXPORT void coroute_shutdown(void* engine_ptr) {
        if (!engine_ptr) return;
        auto* server = static_cast<project::Server*>(engine_ptr);
        server->stop();
        delete server;
    }

    // Helper to run dispatch asynchronously
    static coroute::Task<void> perform_async_dispatch(project::Server* server, std::string method, std::string path, std::string body, void (*callback)(int, const char*)) {
        std::string result_json;
        int status_code = 500;
        try {
            coroute::Response resp;
            if (method == "GET") {
                resp = co_await server->app().fetch_get(path);
            } else if (method == "POST") {
                resp = co_await server->app().fetch_post(path, body);
            } else if (method == "PUT") {
                resp = co_await server->app().fetch_put(path, body);
            } else if (method == "DELETE") {
                resp = co_await server->app().fetch_delete(path);
            } else {
                resp = coroute::Response::bad_request("Method unhandled by bridge");
            }
            
            status_code = resp.status();
            result_json = std::string(resp.body());
        } catch (const std::exception& e) {
            result_json = std::string("{\"error\": \"Exception: ") + e.what() + "\"}";
        } catch (...) {
            result_json = "{\"error\": \"Unknown exception\"}";
        }

        // We assume Dart copies the memory from the callback string
        char* json_cstr = strdup(result_json.c_str());
        callback(status_code, json_cstr);
    }

    COROUTE_EXPORT void coroute_dispatch_request(
        void* engine_ptr, 
        const char* method, 
        const char* path, 
        const char* body, 
        void (*callback)(int, const char*)
    ) {
        if (!engine_ptr || !callback) return;
        
        auto* server = static_cast<project::Server*>(engine_ptr);
        std::string method_str = method ? method : "GET";
        std::string path_str = path ? path : "/";
        std::string body_str = body ? body : "";
        
        if (!server->app().io_context()) {
            // Synchronous execution (not running in full IO loop)
            callback(500, "{\"error\": \"Server IO Context not running\"}");
            return;
        }

        server->app().io_context()->post([server, method_str, path_str, body_str, callback]() {
            perform_async_dispatch(server, method_str, path_str, body_str, callback).start_detached();
        });
    }
}
