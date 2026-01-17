// HTTPS Server Example
// Demonstrates TLS/SSL support with Coroute
//
// To run this example, you need:
// 1. OpenSSL installed and found by CMake
// 2. A certificate and private key (see below for generating self-signed)
//
// Generate self-signed certificate for testing:
//   openssl req -x509 -newkey rsa:4096 -keyout key.pem -out cert.pem -days 365 -nodes
//
// Run with:
//   ./https_server cert.pem key.pem

#include <coroute/coroute.hpp>
#include <iostream>

using namespace coroute;

int main(int argc, char* argv[]) {
#ifdef COROUTE_HAS_TLS
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <cert.pem> <key.pem> [port]" << std::endl;
        std::cerr << "\nGenerate self-signed certificate for testing:" << std::endl;
        std::cerr << "  openssl req -x509 -newkey rsa:4096 -keyout key.pem -out cert.pem -days 365 -nodes" << std::endl;
        return 1;
    }
    
    std::string cert_file = argv[1];
    std::string key_file = argv[2];
    uint16_t port = (argc > 3) ? static_cast<uint16_t>(std::stoi(argv[3])) : 8443;
    
    App app;
    
    // Enable TLS
    app.enable_tls({
        .cert_file = cert_file,
        .key_file = key_file,
        .alpn_protocols = {"http/1.1"}  // Advertise HTTP/1.1 via ALPN
    });
    
    // Routes
    app.get("/", [](Request&) -> Task<Response> {
        co_return Response::html(R"(
            <!DOCTYPE html>
            <html>
            <head><title>HTTPS Server</title></head>
            <body>
                <h1>Hello from HTTPS!</h1>
                <p>Your connection is encrypted with TLS.</p>
                <p><a href="/info">View TLS Info</a></p>
            </body>
            </html>
        )");
    });
    
    app.get("/info", [](Request&) -> Task<Response> {
        co_return json_response(json_object()
            .set("secure", true)
            .set("protocol", "HTTPS")
            .set("message", "Connection is encrypted")
            .build());
    });
    
    app.get("/api/data", [](Request&) -> Task<Response> {
        co_return json_response(json_object()
            .set("status", "ok")
            .set("data", json_array({1, 2, 3, 4, 5}))
            .build());
    });
    
    std::cout << "Starting HTTPS server..." << std::endl;
    std::cout << "Certificate: " << cert_file << std::endl;
    std::cout << "Private Key: " << key_file << std::endl;
    std::cout << "Access at: https://localhost:" << port << "/" << std::endl;
    
    try {
        app.run(port);
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }
    
#else
    std::cerr << "TLS support not enabled. Rebuild with -Dcoroute_ENABLE_TLS=ON and OpenSSL installed." << std::endl;
    return 1;
#endif
    
    return 0;
}
