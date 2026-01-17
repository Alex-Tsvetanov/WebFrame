/**
 * Coroute v2 - Streaming Server Example
 * 
 * Demonstrates chunked transfer encoding for streaming responses.
 */

#include <coroute/coroute.hpp>
#include <iostream>

using namespace coroute;

int main() {
    App app;
    app.threads(4);
    
    // =========================================================================
    // Regular route (for comparison)
    // =========================================================================
    
    app.get("/", [](Request&) -> Task<Response> {
        co_return Response::html(R"(
<!DOCTYPE html>
<html>
<head><title>Streaming Demo</title></head>
<body>
    <h1>Coroute Streaming Demo</h1>
    <h2>Endpoints:</h2>
    <ul>
        <li><a href="/stream">/stream</a> - Chunked streaming (5 chunks, 1s delay each)</li>
        <li><a href="/countdown">/countdown</a> - Countdown timer stream</li>
        <li><a href="/events">/events</a> - Server-Sent Events style</li>
        <li><a href="/large">/large</a> - Large chunked response</li>
    </ul>
</body>
</html>
        )");
    });
    
    // =========================================================================
    // Basic streaming endpoint
    // =========================================================================
    
    // Note: For true streaming, we need access to the connection.
    // This example shows the chunked encoding utilities.
    // In a real app, you'd use the ChunkedResponse class with the connection.
    
    app.get("/stream", [](Request&) -> Task<Response> {
        // For now, we'll simulate by building a chunked body
        // Real streaming would use ChunkedResponse with connection access
        std::string body;
        for (int i = 1; i <= 5; i++) {
            body += "Chunk " + std::to_string(i) + " of 5\n";
        }
        
        Response resp = Response::ok(body, "text/plain");
        resp.set_header("X-Note", "This is a simulated stream - real streaming uses ChunkedResponse");
        co_return resp;
    });
    
    // =========================================================================
    // Countdown endpoint
    // =========================================================================
    
    app.get("/countdown", [](Request&) -> Task<Response> {
        std::string body;
        for (int i = 10; i >= 0; i--) {
            if (i == 0) {
                body += "Liftoff!\n";
            } else {
                body += std::to_string(i) + "...\n";
            }
        }
        co_return Response::ok(body, "text/plain");
    });
    
    // =========================================================================
    // Server-Sent Events style endpoint
    // =========================================================================
    
    app.get("/events", [](Request&) -> Task<Response> {
        std::string body;
        for (int i = 1; i <= 5; i++) {
            body += "event: message\n";
            body += "data: Event number " + std::to_string(i) + "\n\n";
        }
        
        Response resp = Response::ok(body, "text/event-stream");
        resp.set_header("Cache-Control", "no-cache");
        co_return resp;
    });
    
    // =========================================================================
    // Large response endpoint
    // =========================================================================
    
    app.get("/large", [](Request&) -> Task<Response> {
        // Generate a large response that would benefit from chunked encoding
        std::string body;
        body.reserve(100 * 1024);  // 100KB
        
        for (int i = 0; i < 1000; i++) {
            body += "Line " + std::to_string(i) + ": ";
            body += std::string(80, 'x');  // 80 chars of padding
            body += "\n";
        }
        
        co_return Response::ok(body, "text/plain");
    });
    
    // =========================================================================
    // Chunked encoding info endpoint
    // =========================================================================
    
    app.get("/api/chunked-info", [](Request&) -> Task<Response> {
        // Demonstrate the chunked encoding utilities
        std::string chunk1 = chunked::encode_chunk("Hello, ");
        std::string chunk2 = chunked::encode_chunk("World!");
        std::string final_chunk = chunked::encode_final_chunk();
        
        std::string json = R"({
    "description": "Chunked Transfer Encoding Demo",
    "chunk1_encoded": ")" + chunk1.substr(0, chunk1.size() - 2) + R"(\r\n",
    "chunk2_encoded": ")" + chunk2.substr(0, chunk2.size() - 2) + R"(\r\n",
    "final_chunk": "0\r\n\r\n",
    "note": "Real streaming uses ChunkedResponse with direct connection access"
})";
        
        co_return Response::json(json);
    });
    
    std::cout << "Streaming Server Demo" << std::endl;
    std::cout << "=====================" << std::endl;
    std::cout << "Server running on: http://localhost:8080/" << std::endl;
    std::cout << std::endl;
    std::cout << "Endpoints:" << std::endl;
    std::cout << "  /           - Index page" << std::endl;
    std::cout << "  /stream     - Basic streaming demo" << std::endl;
    std::cout << "  /countdown  - Countdown timer" << std::endl;
    std::cout << "  /events     - SSE-style events" << std::endl;
    std::cout << "  /large      - Large response" << std::endl;
    std::cout << "  /api/chunked-info - Chunked encoding info" << std::endl;
    
    app.run(8080);
    
    return 0;
}
