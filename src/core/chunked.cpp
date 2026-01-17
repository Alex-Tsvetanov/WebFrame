#include "coroute/core/chunked.hpp"

#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cctype>

namespace coroute {

// ============================================================================
// Chunked Encoding Utilities
// ============================================================================

namespace chunked {

std::string encode_chunk(std::string_view data) {
    if (data.empty()) {
        return "";  // Don't send empty chunks (use encode_final_chunk for termination)
    }
    
    std::ostringstream oss;
    oss << std::hex << data.size() << "\r\n";
    oss.write(data.data(), data.size());
    oss << "\r\n";
    return oss.str();
}

std::string encode_final_chunk() {
    return "0\r\n\r\n";
}

std::string encode_final_chunk(const std::vector<std::pair<std::string, std::string>>& trailers) {
    std::ostringstream oss;
    oss << "0\r\n";
    
    for (const auto& [key, value] : trailers) {
        oss << key << ": " << value << "\r\n";
    }
    
    oss << "\r\n";
    return oss.str();
}

int64_t parse_chunk_size(std::string_view hex_size) {
    // Trim whitespace
    while (!hex_size.empty() && std::isspace(static_cast<unsigned char>(hex_size.front()))) {
        hex_size.remove_prefix(1);
    }
    while (!hex_size.empty() && std::isspace(static_cast<unsigned char>(hex_size.back()))) {
        hex_size.remove_suffix(1);
    }
    
    // Handle chunk extensions (ignore everything after semicolon)
    auto semicolon = hex_size.find(';');
    if (semicolon != std::string_view::npos) {
        hex_size = hex_size.substr(0, semicolon);
    }
    
    if (hex_size.empty()) {
        return -1;
    }
    
    int64_t result = 0;
    for (char c : hex_size) {
        result *= 16;
        if (c >= '0' && c <= '9') {
            result += c - '0';
        } else if (c >= 'a' && c <= 'f') {
            result += c - 'a' + 10;
        } else if (c >= 'A' && c <= 'F') {
            result += c - 'A' + 10;
        } else {
            return -1;  // Invalid character
        }
    }
    
    return result;
}

} // namespace chunked

// ============================================================================
// ChunkedResponse Implementation
// ============================================================================

std::string_view ChunkedResponse::status_text(int status) noexcept {
    switch (status) {
        case 200: return "OK";
        case 201: return "Created";
        case 204: return "No Content";
        case 400: return "Bad Request";
        case 404: return "Not Found";
        case 500: return "Internal Server Error";
        default: return "Unknown";
    }
}

Task<expected<void, Error>> ChunkedResponse::send_headers() {
    if (!conn_) {
        co_return unexpected(Error::http(HttpError::Internal, "No connection set"));
    }
    
    if (headers_sent_) {
        co_return expected<void, Error>{};  // Already sent
    }
    
    std::ostringstream oss;
    
    // Status line
    oss << "HTTP/1.1 " << status_ << " " << status_text(status_) << "\r\n";
    
    // Add Transfer-Encoding header
    oss << "Transfer-Encoding: chunked\r\n";
    
    // Add Trailer header if we have trailers
    if (!trailers_.empty()) {
        oss << "Trailer: ";
        for (size_t i = 0; i < trailers_.size(); ++i) {
            if (i > 0) oss << ", ";
            oss << trailers_[i].first;
        }
        oss << "\r\n";
    }
    
    // User headers
    for (const auto& [key, value] : headers_) {
        // Skip Content-Length (incompatible with chunked)
        if (key == "Content-Length") continue;
        // Skip Transfer-Encoding (we set it)
        if (key == "Transfer-Encoding") continue;
        oss << key << ": " << value << "\r\n";
    }
    
    // End of headers
    oss << "\r\n";
    
    std::string header_data = oss.str();
    auto result = co_await conn_->async_write_all(header_data.data(), header_data.size());
    
    if (!result) {
        co_return unexpected(result.error());
    }
    
    headers_sent_ = true;
    co_return expected<void, Error>{};
}

Task<expected<void, Error>> ChunkedResponse::write(std::string_view data) {
    if (!conn_) {
        co_return unexpected(Error::http(HttpError::Internal, "No connection set"));
    }
    
    if (finished_) {
        co_return unexpected(Error::http(HttpError::Internal, "Response already finished"));
    }
    
    // Send headers on first write
    if (!headers_sent_) {
        auto result = co_await send_headers();
        if (!result) {
            co_return result;
        }
    }
    
    // Don't send empty chunks
    if (data.empty()) {
        co_return expected<void, Error>{};
    }
    
    // Encode and send chunk
    std::string chunk = chunked::encode_chunk(data);
    auto result = co_await conn_->async_write_all(chunk.data(), chunk.size());
    
    if (!result) {
        co_return unexpected(result.error());
    }
    
    co_return expected<void, Error>{};
}

Task<expected<void, Error>> ChunkedResponse::finish() {
    if (!conn_) {
        co_return unexpected(Error::http(HttpError::Internal, "No connection set"));
    }
    
    if (finished_) {
        co_return expected<void, Error>{};  // Already finished
    }
    
    // Send headers if not sent yet (empty response)
    if (!headers_sent_) {
        auto result = co_await send_headers();
        if (!result) {
            co_return result;
        }
    }
    
    // Send final chunk with trailers
    std::string final_chunk;
    if (trailers_.empty()) {
        final_chunk = chunked::encode_final_chunk();
    } else {
        final_chunk = chunked::encode_final_chunk(trailers_);
    }
    
    auto result = co_await conn_->async_write_all(final_chunk.data(), final_chunk.size());
    
    if (!result) {
        co_return unexpected(result.error());
    }
    
    finished_ = true;
    co_return expected<void, Error>{};
}

// ============================================================================
// ChunkedBodyReader Implementation
// ============================================================================

Task<expected<std::string, Error>> ChunkedBodyReader::read_line() {
    std::string line;
    
    // First check buffer for existing data
    auto crlf_pos = buffer_.find("\r\n");
    if (crlf_pos != std::string::npos) {
        line = buffer_.substr(0, crlf_pos);
        buffer_.erase(0, crlf_pos + 2);
        co_return line;
    }
    
    // Read more data
    char buf[1024];
    while (true) {
        auto result = co_await conn_->async_read(buf, sizeof(buf));
        if (!result) {
            co_return unexpected(result.error());
        }
        
        if (*result == 0) {
            // Connection closed
            co_return unexpected(Error::http(HttpError::BadRequest, "Connection closed while reading chunk"));
        }
        
        buffer_.append(buf, *result);
        
        crlf_pos = buffer_.find("\r\n");
        if (crlf_pos != std::string::npos) {
            line = buffer_.substr(0, crlf_pos);
            buffer_.erase(0, crlf_pos + 2);
            co_return line;
        }
        
        // Prevent buffer from growing too large
        if (buffer_.size() > 8192) {
            co_return unexpected(Error::http(HttpError::BadRequest, "Chunk size line too long"));
        }
    }
}

Task<expected<std::string, Error>> ChunkedBodyReader::read_bytes(size_t n) {
    std::string result;
    result.reserve(n);
    
    // Use buffered data first
    if (!buffer_.empty()) {
        size_t to_use = std::min(buffer_.size(), n);
        result.append(buffer_, 0, to_use);
        buffer_.erase(0, to_use);
    }
    
    // Read remaining
    while (result.size() < n) {
        char buf[4096];
        size_t to_read = std::min(sizeof(buf), n - result.size());
        
        auto read_result = co_await conn_->async_read(buf, to_read);
        if (!read_result) {
            co_return unexpected(read_result.error());
        }
        
        if (*read_result == 0) {
            co_return unexpected(Error::http(HttpError::BadRequest, "Connection closed while reading chunk data"));
        }
        
        result.append(buf, *read_result);
    }
    
    co_return result;
}

Task<expected<std::string, Error>> ChunkedBodyReader::read_chunk() {
    if (finished_) {
        co_return std::string{};
    }
    
    // Read chunk size line
    auto size_line = co_await read_line();
    if (!size_line) {
        co_return unexpected(size_line.error());
    }
    
    int64_t chunk_size = chunked::parse_chunk_size(*size_line);
    if (chunk_size < 0) {
        co_return unexpected(Error::http(HttpError::BadRequest, "Invalid chunk size"));
    }
    
    // Final chunk
    if (chunk_size == 0) {
        // Read trailers (if any) until empty line
        while (true) {
            auto trailer_line = co_await read_line();
            if (!trailer_line) {
                co_return unexpected(trailer_line.error());
            }
            
            if (trailer_line->empty()) {
                break;  // End of trailers
            }
            
            // Parse trailer
            auto colon = trailer_line->find(':');
            if (colon != std::string::npos) {
                std::string key = trailer_line->substr(0, colon);
                std::string value = trailer_line->substr(colon + 1);
                // Trim leading whitespace from value
                while (!value.empty() && std::isspace(static_cast<unsigned char>(value.front()))) {
                    value.erase(0, 1);
                }
                trailers_.emplace_back(std::move(key), std::move(value));
            }
        }
        
        finished_ = true;
        co_return std::string{};
    }
    
    // Read chunk data
    auto chunk_data = co_await read_bytes(static_cast<size_t>(chunk_size));
    if (!chunk_data) {
        co_return unexpected(chunk_data.error());
    }
    
    // Read trailing CRLF
    auto crlf = co_await read_bytes(2);
    if (!crlf) {
        co_return unexpected(crlf.error());
    }
    
    if (*crlf != "\r\n") {
        co_return unexpected(Error::http(HttpError::BadRequest, "Missing CRLF after chunk data"));
    }
    
    co_return std::move(*chunk_data);
}

Task<expected<std::string, Error>> ChunkedBodyReader::read_all(size_t max_size) {
    std::string result;
    
    while (!finished_) {
        auto chunk = co_await read_chunk();
        if (!chunk) {
            co_return unexpected(chunk.error());
        }
        
        if (chunk->empty()) {
            break;  // Final chunk
        }
        
        if (result.size() + chunk->size() > max_size) {
            co_return unexpected(Error::http(HttpError::PayloadTooLarge, "Chunked body too large"));
        }
        
        result.append(*chunk);
    }
    
    co_return result;
}

// ============================================================================
// Streaming Response Helper
// ============================================================================

Task<Response> streaming_response(
    net::Connection* conn,
    std::string content_type,
    StreamCallback callback)
{
    ChunkedResponse resp(conn);
    resp.content_type(std::move(content_type));
    
    // Call the callback to generate chunks
    bool continue_streaming = true;
    while (continue_streaming) {
        continue_streaming = co_await callback(resp);
    }
    
    // Finish the response
    auto result = co_await resp.finish();
    if (!result) {
        // Return an error response (though headers may already be sent)
        co_return Response::internal_error("Streaming error");
    }
    
    // Return empty response since we already wrote directly
    // The caller should not serialize this
    Response empty_resp;
    empty_resp.set_status(0);  // Signal that response was already sent
    co_return empty_resp;
}

} // namespace coroute
