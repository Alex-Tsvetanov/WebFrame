#include "coroute/http2/stream.hpp"
#include "coroute/http2/connection.hpp"

namespace coroute::http2 {

// ============================================================================
// Stream Implementation
// ============================================================================

Stream::Stream(uint32_t id, Http2Connection* connection, int32_t initial_window_size)
    : id_(id)
    , connection_(connection)
    , local_window_size_(initial_window_size)
    , remote_window_size_(initial_window_size)
{
}

void Stream::update_local_window(int32_t delta) {
    local_window_size_ += delta;
}

void Stream::update_remote_window(int32_t delta) {
    remote_window_size_ += delta;
}

void Stream::receive_headers(std::vector<Header> headers, bool end_stream) {
    request_headers_ = std::move(headers);
    headers_complete_ = true;
    
    if (end_stream) {
        body_complete_ = true;
        transition_to(StreamState::HalfClosedRemote);
    } else {
        transition_to(StreamState::Open);
    }
}

void Stream::receive_data(std::span<const uint8_t> data, bool end_stream) {
    request_body_.insert(request_body_.end(), data.begin(), data.end());
    
    if (end_stream) {
        body_complete_ = true;
        if (state_ == StreamState::Open) {
            transition_to(StreamState::HalfClosedRemote);
        }
    }
}

expected<Request, Error> Stream::build_request() const {
    if (!headers_complete_) {
        return unexpected(Error::io(IoError::InvalidArgument, "Headers not complete"));
    }
    
    // Extract method
    auto method_str = get_method(request_headers_);
    if (method_str.empty()) {
        return unexpected(Error::io(IoError::InvalidArgument, "Missing :method"));
    }
    
    HttpMethod method;
    if (method_str == "GET") method = HttpMethod::GET;
    else if (method_str == "POST") method = HttpMethod::POST;
    else if (method_str == "PUT") method = HttpMethod::PUT;
    else if (method_str == "DELETE") method = HttpMethod::DELETE;
    else if (method_str == "HEAD") method = HttpMethod::HEAD;
    else if (method_str == "OPTIONS") method = HttpMethod::OPTIONS;
    else if (method_str == "PATCH") method = HttpMethod::PATCH;
    else {
        return unexpected(Error::io(IoError::InvalidArgument, "Unknown method"));
    }
    
    // Extract path
    auto path = get_path(request_headers_);
    if (path.empty()) {
        return unexpected(Error::io(IoError::InvalidArgument, "Missing :path"));
    }
    
    // Build request
    Request req;
    req.set_method(method);
    
    // Parse path and query string
    auto query_pos = path.find('?');
    if (query_pos != std::string_view::npos) {
        req.set_path(std::string(path.substr(0, query_pos)));
        req.set_query_string(std::string(path.substr(query_pos + 1)));
    } else {
        req.set_path(std::string(path));
    }
    
    // Copy headers (excluding pseudo-headers)
    for (const auto& h : request_headers_) {
        if (!h.is_pseudo()) {
            req.add_header(h.name, h.value);
        }
    }
    
    // Set authority as Host header if present
    auto authority = get_authority(request_headers_);
    if (!authority.empty()) {
        req.add_header("Host", std::string(authority));
    }
    
    // Set body
    if (!request_body_.empty()) {
        req.set_body(std::string(
            reinterpret_cast<const char*>(request_body_.data()),
            request_body_.size()
        ));
    }
    
    // Mark as HTTP/2
    req.set_http_version("HTTP/2");
    
    return req;
}

Task<expected<void, Error>> Stream::send_response(const Response& response) {
    if (response_headers_sent_) {
        co_return unexpected(Error::io(IoError::InvalidArgument, "Response already sent"));
    }
    
    // Build response headers
    std::vector<Header> headers;
    
    // :status pseudo-header
    headers.push_back({":status", std::to_string(response.status())});
    
    // Regular headers
    for (const auto& [name, value] : response.headers()) {
        // Convert header name to lowercase
        std::string lower_name;
        lower_name.reserve(name.size());
        for (char c : name) {
            lower_name.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
        }
        
        // Skip connection-specific headers (not allowed in HTTP/2)
        if (lower_name == "connection" || lower_name == "keep-alive" ||
            lower_name == "transfer-encoding" || lower_name == "upgrade") {
            continue;
        }
        
        headers.push_back({std::move(lower_name), value});
    }
    
    // Add content-length if body present and not already set
    const auto& body = response.body();
    bool has_content_length = false;
    for (const auto& h : headers) {
        if (h.name == "content-length") {
            has_content_length = true;
            break;
        }
    }
    if (!has_content_length && !body.empty()) {
        headers.push_back({"content-length", std::to_string(body.size())});
    }
    
    // Send headers
    bool end_stream = body.empty();
    auto result = co_await send_headers(headers, end_stream);
    if (!result) {
        co_return unexpected(result.error());
    }
    
    response_headers_sent_ = true;
    
    // Send body if present
    if (!body.empty()) {
        auto body_result = co_await send_data(
            std::span<const uint8_t>(
                reinterpret_cast<const uint8_t*>(body.data()),
                body.size()
            ),
            true  // end_stream
        );
        if (!body_result) {
            co_return unexpected(body_result.error());
        }
    }
    
    response_complete_ = true;
    transition_to(StreamState::Closed);
    
    co_return expected<void, Error>{};
}

void Stream::transition_to(StreamState new_state) {
    state_ = new_state;
}

void Stream::reset(ErrorCode error) {
    state_ = StreamState::Closed;
    // Send RST_STREAM frame
    auto frame = serialize_rst_stream_frame(id_, error);
    // Note: This is synchronous, but in practice we'd want to queue this
    // For now, we'll handle this in the connection
}

Task<expected<void, Error>> Stream::send_headers(
    std::span<const Header> headers,
    bool end_stream
) {
    // Encode headers with HPACK
    auto encoded = connection_->encoder().encode(headers);
    if (!encoded) {
        co_return unexpected(encoded.error());
    }
    
    // Serialize HEADERS frame
    auto frame = serialize_headers_frame(id_, *encoded, end_stream, true);
    
    // Send frame
    co_return co_await connection_->send_frame(frame);
}

Task<expected<void, Error>> Stream::send_data(
    std::span<const uint8_t> data,
    bool end_stream
) {
    const uint32_t max_frame_size = connection_->remote_settings().max_frame_size;
    
    size_t offset = 0;
    while (offset < data.size()) {
        // Calculate chunk size (respecting frame size and flow control)
        size_t remaining = data.size() - offset;
        size_t chunk_size = std::min(remaining, static_cast<size_t>(max_frame_size));
        
        // TODO: Check flow control window and wait if necessary
        // For now, assume we have enough window
        
        bool is_last = (offset + chunk_size >= data.size());
        bool frame_end_stream = end_stream && is_last;
        
        auto frame = serialize_data_frame(
            id_,
            data.subspan(offset, chunk_size),
            frame_end_stream
        );
        
        auto result = co_await connection_->send_frame(frame);
        if (!result) {
            co_return unexpected(result.error());
        }
        
        offset += chunk_size;
    }
    
    co_return expected<void, Error>{};
}

} // namespace coroute::http2
