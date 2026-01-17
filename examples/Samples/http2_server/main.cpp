// HTTP/2 Server Example
// Demonstrates HTTP/2 support with both h2 (TLS) and h2c (cleartext upgrade)
//
// Test with:
//   HTTPS (h2):  curl -k --http2 https://localhost:8443/
//   HTTP (h2c):  curl --http2 http://localhost:8080/
//   HTTP/1.1:    curl http://localhost:8080/

#include <coroute/coroute.hpp>
#include <iostream>

using namespace coroute;

int main() {
    App app;
    
    // Simple hello endpoint
    app.get("/", [](Request&) -> Task<Response> {
        co_return Response::ok("Hello from HTTP/2 Server!\n");
    });
    
    // Echo endpoint - returns request info
    app.get("/echo", [](Request& req) -> Task<Response> {
        std::string body = "Request Info:\n";
        body += "  Method: " + std::string(method_to_string(req.method())) + "\n";
        body += "  Path: " + std::string(req.path()) + "\n";
        body += "  HTTP Version: " + std::string(req.http_version()) + "\n";
        body += "\nHeaders:\n";
        
        for (const auto& [key, value] : req.headers()) {
            body += "  " + key + ": " + value + "\n";
        }
        
        co_return Response::ok(body);
    });
    
    // JSON endpoint
    app.get("/api/status", [](Request&) -> Task<Response> {
        co_return Response::json(R"({"status":"ok","protocol":"HTTP/2","server":"Coroute"})");
    });
    
    // Large response for testing flow control
    app.get("/large", [](Request&) -> Task<Response> {
        std::string body;
        body.reserve(1024 * 100);  // 100KB
        for (int i = 0; i < 1000; ++i) {
            body += "Line " + std::to_string(i) + ": ";
            body += std::string(90, 'x');
            body += "\n";
        }
        co_return Response::ok(body);
    });
    
    // POST endpoint
    app.post("/api/data", [](Request& req) -> Task<Response> {
        std::string json = R"({"received":true,"body_size":)" + 
                          std::to_string(req.body().size()) + "}";
        co_return Response::json(json);
    });

#ifdef COROUTE_HAS_TLS
    // Start HTTPS server with HTTP/2 (h2) on port 8443
    std::cout << "Starting HTTPS server with HTTP/2 on port 8443..." << std::endl;
    std::cout << "  Test: curl -k --http2 https://localhost:8443/" << std::endl;
    
    AppTlsConfig tls_config;
    tls_config.cert_file = "v2/examples/certs/cert.pem";
    tls_config.key_file = "v2/examples/certs/key.pem";
    // ALPN will automatically advertise h2 and http/1.1
    
    app.enable_tls(tls_config);
    app.run(8443);
#else
    // Start HTTP server with HTTP/2 upgrade (h2c) on port 8080
    std::cout << "Starting HTTP server with HTTP/2 upgrade (h2c) on port 8080..." << std::endl;
    std::cout << "  Test h2c:    curl --http2 http://localhost:8080/" << std::endl;
    std::cout << "  Test HTTP/1: curl http://localhost:8080/" << std::endl;
    
    app.run(8080);
#endif
    
    return 0;
}
