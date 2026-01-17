/**
 * Coroute v2 - Echo Server Example
 * 
 * A simple echo server that demonstrates async I/O.
 */

#include <coroute/coroute.hpp>
#include <iostream>
#include <csignal>

using namespace coroute;

// Global app pointer for signal handling
App* g_app = nullptr;

void signal_handler(int signal) {
    std::cout << "\nReceived signal " << signal << ", shutting down..." << std::endl;
    if (g_app) {
        g_app->stop();
    }
}

int main() {
    App app;
    g_app = &app;
    
    // Set up signal handlers for graceful shutdown
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);
    
    // Configure
    app.threads(2);
    
    // Echo endpoint - returns whatever you POST
    app.post("/echo", [](Request& req) -> Task<Response> {
        std::cout << "Echo request from client" << std::endl;
        co_return Response::ok(std::string(req.body()), std::string(req.content_type().value_or("text/plain")));
    });
    
    // JSON echo
    app.post("/json", [](Request& req) -> Task<Response> {
        co_return Response::json(std::string(req.body()));
    });
    
    // Health check
    app.get("/health", [](Request& req) -> Task<Response> {
        co_return Response::json(R"({"status": "ok", "version": "2.0.0"})");
    });
    
    // Delayed response (simulates async work)
    app.get("/delay/{ms}", [](Request& req) -> Task<Response> {
        auto ms_result = req.param<int>(0);
        if (!ms_result) {
            co_return Response::bad_request("Invalid delay value");
        }
        
        int ms = *ms_result;
        if (ms > 10000) {
            co_return Response::bad_request("Delay too long (max 10000ms)");
        }
        
        // In a real implementation, this would use async sleep
        // For now, we just return immediately
        co_return Response::ok("Delayed response after " + std::to_string(ms) + "ms");
    });
    
    std::cout << "Echo Server starting on port 8080..." << std::endl;
    std::cout << "Press Ctrl+C to stop" << std::endl;
    std::cout << std::endl;
    std::cout << "Endpoints:" << std::endl;
    std::cout << "  POST /echo   - Echo back the request body" << std::endl;
    std::cout << "  POST /json   - Echo back JSON" << std::endl;
    std::cout << "  GET  /health - Health check" << std::endl;
    std::cout << "  GET  /delay/{ms} - Delayed response" << std::endl;
    
    app.run(8080);
    
    std::cout << "Server stopped." << std::endl;
    return 0;
}
