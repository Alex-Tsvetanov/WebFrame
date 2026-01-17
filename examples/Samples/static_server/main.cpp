/**
 * Coroute v2 - Static File Server Example
 * 
 * Demonstrates serving static files with caching and directory listing.
 */

#include <coroute/coroute.hpp>
#include <iostream>
#include <filesystem>

using namespace coroute;

int main(int argc, char* argv[]) {
    // Default to current directory if no path provided
    std::filesystem::path static_root = ".";
    if (argc > 1) {
        static_root = argv[1];
    }
    
    // Verify the path exists
    if (!std::filesystem::exists(static_root)) {
        std::cerr << "Error: Directory does not exist: " << static_root << std::endl;
        return 1;
    }
    
    App app;
    app.threads(4);
    
    // Configure static file options
    StaticFileOptions options;
    options.root = static_root;
    options.index_file = "index.html";
    options.directory_listing = true;  // Enable directory listing
    options.etag = true;               // Enable ETag caching
    options.last_modified = true;      // Enable Last-Modified header
    options.max_age_seconds = 3600;    // Cache for 1 hour
    
    // Add custom headers
    options.custom_headers = {
        {"X-Powered-By", "Coroute v2"},
        {"X-Content-Type-Options", "nosniff"}
    };
    
    // Use static file middleware
    app.use(static_files(options));
    
    // API routes (these take precedence if static file not found)
    app.get("/api/status", [](Request&) -> Task<Response> {
        co_return Response::json(R"({"status": "ok", "server": "Coroute v2"})");
    });
    
    // Fallback for non-static, non-API routes
    app.get("/{path:.*}", [](Request& req) -> Task<Response> {
        co_return Response::not_found("File not found: " + std::string(req.path()));
    });
    
    std::cout << "Static File Server" << std::endl;
    std::cout << "==================" << std::endl;
    std::cout << "Serving files from: " << std::filesystem::absolute(static_root) << std::endl;
    std::cout << "Server running on: http://localhost:8080/" << std::endl;
    std::cout << std::endl;
    std::cout << "Features:" << std::endl;
    std::cout << "  - Directory listing enabled" << std::endl;
    std::cout << "  - ETag caching enabled" << std::endl;
    std::cout << "  - Cache-Control: max-age=3600" << std::endl;
    std::cout << std::endl;
    std::cout << "Try:" << std::endl;
    std::cout << "  http://localhost:8080/           - Directory listing" << std::endl;
    std::cout << "  http://localhost:8080/api/status - API endpoint" << std::endl;
    
    app.run(8080);
    
    return 0;
}
