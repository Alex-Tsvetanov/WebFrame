#pragma once

#include "coroute/http2/frame.hpp"
#include "coroute/http2/hpack.hpp"
#include "coroute/core/request.hpp"
#include "coroute/core/response.hpp"
#include "coroute/coro/task.hpp"

#include <cstdint>
#include <memory>
#include <functional>

namespace coroute::http2 {

// ============================================================================
// HTTP/2 Stream States (RFC 7540 Section 5.1)
// ============================================================================

enum class StreamState {
    Idle,
    ReservedLocal,   // Server push (not implementing)
    ReservedRemote,  // Server push (not implementing)
    Open,
    HalfClosedLocal,
    HalfClosedRemote,
    Closed,
};

// ============================================================================
// HTTP/2 Stream
// ============================================================================

class Http2Connection;  // Forward declaration

class Stream {
    uint32_t id_;
    StreamState state_ = StreamState::Idle;
    Http2Connection* connection_;
    
    // Flow control
    int32_t local_window_size_;   // Our receive window
    int32_t remote_window_size_;  // Peer's receive window
    
    // Request building
    std::vector<Header> request_headers_;
    std::vector<uint8_t> request_body_;
    bool headers_complete_ = false;
    bool body_complete_ = false;
    
    // Response
    bool response_headers_sent_ = false;
    bool response_complete_ = false;
    
public:
    Stream(uint32_t id, Http2Connection* connection, int32_t initial_window_size);
    ~Stream() = default;
    
    // Non-copyable, non-movable (managed by connection)
    Stream(const Stream&) = delete;
    Stream& operator=(const Stream&) = delete;
    Stream(Stream&&) = delete;
    Stream& operator=(Stream&&) = delete;
    
    // Accessors
    uint32_t id() const { return id_; }
    StreamState state() const { return state_; }
    bool is_open() const { return state_ == StreamState::Open || 
                                  state_ == StreamState::HalfClosedLocal ||
                                  state_ == StreamState::HalfClosedRemote; }
    
    // Flow control
    int32_t local_window_size() const { return local_window_size_; }
    int32_t remote_window_size() const { return remote_window_size_; }
    void update_local_window(int32_t delta);
    void update_remote_window(int32_t delta);
    
    // Request handling
    void receive_headers(std::vector<Header> headers, bool end_stream);
    void receive_data(std::span<const uint8_t> data, bool end_stream);
    bool is_request_complete() const { return headers_complete_ && body_complete_; }
    
    // Build Request object from received headers/body
    expected<Request, Error> build_request() const;
    
    // Response handling
    Task<expected<void, Error>> send_response(const Response& response);
    
    // State transitions
    void transition_to(StreamState new_state);
    
    // Reset stream with error
    void reset(ErrorCode error);
    
private:
    // Send headers frame
    Task<expected<void, Error>> send_headers(
        std::span<const Header> headers,
        bool end_stream
    );
    
    // Send data frames (handles flow control and fragmentation)
    Task<expected<void, Error>> send_data(
        std::span<const uint8_t> data,
        bool end_stream
    );
};

// ============================================================================
// Stream ID Utilities
// ============================================================================

namespace StreamId {
    // Stream 0 is the connection control stream
    constexpr uint32_t Connection = 0;
    
    // Client-initiated streams are odd
    inline bool is_client_initiated(uint32_t id) { return id != 0 && (id & 1) == 1; }
    
    // Server-initiated streams are even (for push, which we don't implement)
    inline bool is_server_initiated(uint32_t id) { return id != 0 && (id & 1) == 0; }
}

} // namespace coroute::http2
