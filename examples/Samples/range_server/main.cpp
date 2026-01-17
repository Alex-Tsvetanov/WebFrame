/**
 * Coroute v2 - Range Request Example
 * 
 * Demonstrates HTTP Range requests for partial content delivery.
 * Useful for resumable downloads and video streaming.
 */

#include <coroute/coroute.hpp>
#include <iostream>
#include <fstream>

using namespace coroute;

int main() {
    App app;
    app.threads(4);
    
    // =========================================================================
    // Generate a test file for range requests
    // =========================================================================
    
    // Create a 1MB test file
    const std::string test_file = "range_test.bin";
    const size_t file_size = 1024 * 1024;  // 1MB
    
    {
        std::ofstream f(test_file, std::ios::binary);
        for (size_t i = 0; i < file_size; i++) {
            char c = static_cast<char>(i % 256);
            f.write(&c, 1);
        }
        std::cout << "Created test file: " << test_file << " (" << file_size << " bytes)" << std::endl;
    }
    
    // =========================================================================
    // Routes
    // =========================================================================
    
    // Index page with instructions
    app.get("/", [](Request&) -> Task<Response> {
        co_return Response::html(R"(
<!DOCTYPE html>
<html>
<head><title>Range Request Demo</title></head>
<body>
    <h1>Coroute Range Request Demo</h1>
    
    <h2>Test Endpoints</h2>
    <ul>
        <li><a href="/file">/file</a> - 1MB binary file (supports Range requests)</li>
        <li><a href="/data">/data</a> - In-memory data with Range support</li>
        <li><a href="/video">/video</a> - Simulated video endpoint</li>
    </ul>
    
    <h2>Test with curl</h2>
    <pre>
# Full file
curl -v http://localhost:8080/file -o full.bin

# First 1KB
curl -v -H "Range: bytes=0-1023" http://localhost:8080/file -o first1k.bin

# Last 1KB  
curl -v -H "Range: bytes=-1024" http://localhost:8080/file -o last1k.bin

# Middle portion (bytes 1000-1999)
curl -v -H "Range: bytes=1000-1999" http://localhost:8080/file -o middle.bin

# From byte 500000 to end
curl -v -H "Range: bytes=500000-" http://localhost:8080/file -o second_half.bin

# Multiple ranges (multipart response)
curl -v -H "Range: bytes=0-99,500-599" http://localhost:8080/data
    </pre>
    
    <h2>Expected Responses</h2>
    <ul>
        <li><strong>200 OK</strong> - Full content (no Range header)</li>
        <li><strong>206 Partial Content</strong> - Range request satisfied</li>
        <li><strong>416 Range Not Satisfiable</strong> - Invalid range</li>
    </ul>
</body>
</html>
        )");
    });
    
    // File-based range request (uses RangeResponseBuilder with file)
    app.get("/file", [&test_file](Request& req) -> Task<Response> {
        RangeResponseBuilder builder;
        builder.file(test_file, "application/octet-stream")
               .etag("\"test-file-v1\"");
        
        co_return builder.build(req);
    });
    
    // In-memory data with range support
    app.get("/data", [](Request& req) -> Task<Response> {
        // Generate some data
        std::string data;
        data.reserve(10000);
        for (int i = 0; i < 1000; i++) {
            data += "Line " + std::to_string(i) + ": Hello, World!\n";
        }
        
        RangeResponseBuilder builder;
        builder.content(data, "text/plain")
               .etag("\"data-v1\"");
        
        co_return builder.build(req);
    });
    
    // Simulated video endpoint
    app.get("/video", [](Request& req) -> Task<Response> {
        // In a real app, this would be a video file
        // For demo, we'll use generated data
        std::string video_data(100000, 'V');  // 100KB of 'V's
        
        RangeResponseBuilder builder;
        builder.content(video_data, "video/mp4")
               .etag("\"video-v1\"");
        
        Response resp = builder.build(req);
        
        // Add video-specific headers
        resp.set_header("Accept-Ranges", "bytes");
        
        co_return resp;
    });
    
    // Manual range handling example
    app.get("/manual", [](Request& req) -> Task<Response> {
        std::string content = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
        
        // Check for Range header manually
        if (should_use_range(req)) {
            auto range_header = range::get_range(req);
            if (range_header && range_header->is_single_range()) {
                ByteRange br = range_header->ranges[0];
                if (br.normalize(static_cast<int64_t>(content.size()))) {
                    std::string partial = content.substr(
                        static_cast<size_t>(br.get_start()),
                        static_cast<size_t>(br.length())
                    );
                    co_return partial_content(
                        partial,
                        br.get_start(),
                        br.get_end(),
                        static_cast<int64_t>(content.size()),
                        "text/plain"
                    );
                }
            }
            co_return range_not_satisfiable(static_cast<int64_t>(content.size()));
        }
        
        // Full content
        Response resp = Response::ok(content, "text/plain");
        resp.set_header("Accept-Ranges", "bytes");
        co_return resp;
    });
    
    std::cout << std::endl;
    std::cout << "Range Request Server" << std::endl;
    std::cout << "====================" << std::endl;
    std::cout << "Server running on: http://localhost:8080/" << std::endl;
    std::cout << std::endl;
    std::cout << "Test commands:" << std::endl;
    std::cout << "  curl -v http://localhost:8080/file                    # Full file" << std::endl;
    std::cout << "  curl -v -H \"Range: bytes=0-1023\" http://localhost:8080/file  # First 1KB" << std::endl;
    std::cout << "  curl -v -H \"Range: bytes=-1024\" http://localhost:8080/file   # Last 1KB" << std::endl;
    std::cout << std::endl;
    
    app.run(8080);
    
    // Cleanup
    std::remove(test_file.c_str());
    
    return 0;
}
