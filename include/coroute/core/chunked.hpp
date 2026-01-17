#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <functional>
#include <memory>

#include "coroute/core/response.hpp"
#include "coroute/core/error.hpp"
#include "coroute/util/expected.hpp"
#include "coroute/coro/task.hpp"
#include "coroute/net/io_context.hpp"

namespace coroute {

// ============================================================================
// Chunked Encoding Utilities
// ============================================================================

namespace chunked {

// Encode a single chunk (data -> "size\r\ndata\r\n")
std::string encode_chunk(std::string_view data);

// Encode the final empty chunk ("0\r\n\r\n")
std::string encode_final_chunk();

// Encode final chunk with trailers
std::string encode_final_chunk(const std::vector<std::pair<std::string, std::string>>& trailers);

// Parse chunk size from hex string (returns size, or -1 on error)
int64_t parse_chunk_size(std::string_view hex_size);

} // namespace chunked

// ============================================================================
// ChunkedResponse - For streaming responses without known Content-Length
// ============================================================================

class ChunkedResponse {
public:
    using Header = std::pair<std::string, std::string>;
    using Headers = std::vector<Header>;
    using Trailer = Header;
    using Trailers = std::vector<Trailer>;

private:
    int status_ = 200;
    Headers headers_;
    Trailers trailers_;
    bool headers_sent_ = false;
    bool finished_ = false;
    net::Connection* conn_ = nullptr;

public:
    ChunkedResponse() = default;
    explicit ChunkedResponse(net::Connection* conn) : conn_(conn) {}
    
    // Set connection (must be called before writing)
    void set_connection(net::Connection* conn) { conn_ = conn; }
    
    // Status and headers (must be set before first write)
    ChunkedResponse& status(int code) {
        status_ = code;
        return *this;
    }
    
    ChunkedResponse& header(std::string key, std::string value) {
        headers_.emplace_back(std::move(key), std::move(value));
        return *this;
    }
    
    ChunkedResponse& content_type(std::string type) {
        return header("Content-Type", std::move(type));
    }
    
    // Add a trailer (sent after all chunks)
    ChunkedResponse& trailer(std::string key, std::string value) {
        trailers_.emplace_back(std::move(key), std::move(value));
        return *this;
    }
    
    // Write a chunk of data
    // First call sends headers automatically
    Task<expected<void, Error>> write(std::string_view data);
    
    // Write a chunk from string (convenience)
    Task<expected<void, Error>> write(const std::string& data) {
        return write(std::string_view(data));
    }
    
    // Finish the response (sends final chunk and trailers)
    Task<expected<void, Error>> finish();
    
    // Check if headers have been sent
    bool headers_sent() const noexcept { return headers_sent_; }
    
    // Check if response is finished
    bool finished() const noexcept { return finished_; }

private:
    // Send HTTP headers with Transfer-Encoding: chunked
    Task<expected<void, Error>> send_headers();
    
    // Get status text
    static std::string_view status_text(int status) noexcept;
};

// ============================================================================
// ChunkedBodyReader - For reading chunked request bodies
// ============================================================================

class ChunkedBodyReader {
public:
    explicit ChunkedBodyReader(net::Connection* conn) : conn_(conn) {}
    
    // Read the entire chunked body into a string
    // Use this for small bodies where you want all data at once
    Task<expected<std::string, Error>> read_all(size_t max_size = 10 * 1024 * 1024);
    
    // Read next chunk
    // Returns empty string when all chunks have been read
    Task<expected<std::string, Error>> read_chunk();
    
    // Check if all chunks have been read
    bool finished() const noexcept { return finished_; }
    
    // Get trailers (available after finished)
    const std::vector<std::pair<std::string, std::string>>& trailers() const noexcept {
        return trailers_;
    }

private:
    net::Connection* conn_;
    bool finished_ = false;
    std::vector<std::pair<std::string, std::string>> trailers_;
    std::string buffer_;  // Leftover data from previous reads
    
    // Read a line (up to \r\n)
    Task<expected<std::string, Error>> read_line();
    
    // Read exactly n bytes
    Task<expected<std::string, Error>> read_bytes(size_t n);
};

// ============================================================================
// Streaming Response Helper
// ============================================================================

// Callback type for streaming data
// Return false to stop streaming
using StreamCallback = std::function<Task<bool>(ChunkedResponse&)>;

// Create a streaming response that calls the callback to generate chunks
// The callback should call response.write() to send data
// Example:
//   return streaming_response(conn, "text/plain", [](ChunkedResponse& resp) -> Task<bool> {
//       for (int i = 0; i < 10; i++) {
//           co_await resp.write("chunk " + std::to_string(i) + "\n");
//       }
//       co_return false; // done
//   });
Task<Response> streaming_response(
    net::Connection* conn,
    std::string content_type,
    StreamCallback callback
);

} // namespace coroute
