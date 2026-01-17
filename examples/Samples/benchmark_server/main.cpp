/**
 * Coroute v2 - Benchmark Server
 * 
 * Minimal server for benchmarking - no compression, simple response.
 */

#include <coroute/coroute.hpp>
#include <iostream>

using namespace coroute;

int main(int argc, char** argv) {
    App app;
    
    // Configure thread count
    size_t threads = 12;
    if (argc > 1) {
        threads = std::stoul(argv[1]);
    }
    app.threads(threads);
    
    // Simple route - minimal overhead
    app.get("/", [](Request&) -> Task<Response> {
        co_return Response::ok("Hello, World!");
    });
    
    uint16_t port = 8080;
    if (argc > 2) {
        port = static_cast<uint16_t>(std::stoul(argv[2]));
    }
    
    std::cout << "Benchmark server on port " << port << " (" << threads << " threads)" << std::endl;
    
    app.run(port);
    
    return 0;
}
