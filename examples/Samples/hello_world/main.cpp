/**
 * Coroute v2 - Hello World Example
 * 
 * Demonstrates basic routing, parameter extraction, middleware, and compression.
 */

#include <coroute/coroute.hpp>
#include <iostream>

using namespace coroute;

int main() {
    App app;
    
    // Configure thread count (use all available cores)
    app.threads(std::thread::hardware_concurrency());
    
    // =========================================================================
    // Compression Middleware
    // =========================================================================
    
    // Enable gzip/deflate compression for responses > 1KB
    app.use(compression({
        .min_size = 256,  // Compress responses larger than 256 bytes
        .level = 6
    }));
    
    // =========================================================================
    // Routes
    // =========================================================================
    
    // Simple route - no parameters
    app.get("/", [](Request&) -> Task<Response> {
        co_return Response::ok("Hello, World!");
    });
    
    // Route with path parameter (positional extraction)
    // The {id} is extracted as the first parameter
    app.get<size_t>("/user/{id}", [](size_t id, Request&) -> Task<Response> {
        co_return Response::ok("User ID: " + std::to_string(id));
    });
    
    // Route with multiple parameters
    app.get<size_t, size_t>("/user/{uid}/post/{pid}", [](size_t uid, size_t pid, Request&) -> Task<Response> {
        co_return Response::json(
            R"({"user_id": )" + std::to_string(uid) + 
            R"(, "post_id": )" + std::to_string(pid) + "}"
        );
    });
    
    // Route with string parameter
    app.get<std::string>("/greet/{name}", [](std::string name, Request&) -> Task<Response> {
        co_return Response::ok("Hello, " + name + "!");
    });
    
    // POST route example
    app.post("/echo", [](Request& req) -> Task<Response> {
        co_return Response::ok(std::string(req.body()));
    });
    
    // Using ResponseBuilder
    app.get("/fancy", [](Request&) -> Task<Response> {
        co_return ResponseBuilder()
            .status(200)
            .header("X-Custom-Header", "coroute-v2")
            .content_type("text/html")
            .body("<h1>Fancy Response</h1><p>Built with ResponseBuilder</p>")
            .build();
    });
    
    // Error handling example
    app.get("/error", [](Request&) -> Task<Response> {
        co_return Response::internal_error("Something went wrong!");
    });
    
    // Large response (will be compressed if Accept-Encoding: gzip)
    app.get("/large", [](Request&) -> Task<Response> {
        std::string body = "<html><body><h1>Large Response</h1><ul>";
        for (int i = 0; i < 100; i++) {
            body += "<li>Item " + std::to_string(i) + " - Some repeated content for compression testing</li>";
        }
        body += "</ul></body></html>";
        co_return Response::html(body);
    });
    
    // Query parameter example
    app.get("/search", [](Request& req) -> Task<Response> {
        auto query = req.query<std::string>("q");
        auto page = req.query_opt<int>("page").value_or(1);
        
        if (!query) {
            co_return Response::bad_request("Missing 'q' parameter");
        }
        
        co_return Response::json(
            R"({"query": ")" + *query + R"(", "page": )" + std::to_string(page) + "}"
        );
    });
    
    std::cout << "Starting Coroute v2 on port 8080..." << std::endl;
    std::cout << "Compression: enabled (gzip/deflate, min 256 bytes)" << std::endl;
    std::cout << std::endl;
    std::cout << "Try these URLs:" << std::endl;
    std::cout << "  http://localhost:8080/" << std::endl;
    std::cout << "  http://localhost:8080/user/123" << std::endl;
    std::cout << "  http://localhost:8080/user/1/post/42" << std::endl;
    std::cout << "  http://localhost:8080/greet/World" << std::endl;
    std::cout << "  http://localhost:8080/search?q=test&page=2" << std::endl;
    std::cout << "  http://localhost:8080/fancy" << std::endl;
    std::cout << "  http://localhost:8080/large  (test compression with: curl -H 'Accept-Encoding: gzip' -v)" << std::endl;
    
    // Run the server (blocking)
    app.run(8080);
    
    return 0;
}
