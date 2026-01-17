#pragma once

#include "coroute/http2/frame.hpp"
#include "coroute/http2/hpack.hpp"
#include "coroute/http2/stream.hpp"
#include "coroute/net/io_context.hpp"
#include "coroute/core/request.hpp"
#include "coroute/core/response.hpp"
#include "coroute/coro/task.hpp"

#include <unordered_map>
#include <memory>
#include <functional>
#include <mutex>
#include <atomic>

namespace coroute::http2 {

// ============================================================================
// HTTP/2 Connection Settings
// ============================================================================

struct ConnectionSettings {
    uint32_t header_table_size = Constants::DefaultHeaderTableSize;
    uint32_t enable_push = 0;  // We don't support server push
    uint32_t max_concurrent_streams = Constants::DefaultMaxConcurrentStreams;
    uint32_t initial_window_size = Constants::DefaultInitialWindowSize;
    uint32_t max_frame_size = Constants::DefaultMaxFrameSize;
    uint32_t max_header_list_size = Constants::DefaultMaxHeaderListSize;
    
    // Apply a settings entry
    void apply(SettingsId id, uint32_t value);
};

// ============================================================================
// HTTP/2 Connection
// ============================================================================

// Handler type for processing requests
using RequestHandler = std::function<Task<Response>(Request&)>;

class Http2Connection : public std::enable_shared_from_this<Http2Connection> {
    std::unique_ptr<net::Connection> conn_;
    
    // HPACK encoder/decoder
    HpackEncoder encoder_;
    HpackDecoder decoder_;
    
    // Settings
    ConnectionSettings local_settings_;   // Our settings (sent to peer)
    ConnectionSettings remote_settings_;  // Peer's settings (received)
    
    // Flow control
    int32_t local_window_size_;   // Connection-level receive window
    int32_t remote_window_size_;  // Connection-level send window
    
    // Streams
    std::unordered_map<uint32_t, std::unique_ptr<Stream>> streams_;
    uint32_t last_stream_id_ = 0;         // Last stream ID we processed
    uint32_t next_server_stream_id_ = 2;  // For server push (not used)
    std::atomic<size_t> active_streams_{0};
    
    // Connection state
    std::atomic<bool> is_open_{true};
    bool preface_received_ = false;
    bool settings_ack_received_ = false;
    bool goaway_sent_ = false;
    ErrorCode goaway_error_ = ErrorCode::NoError;
    
    // Read buffer
    std::vector<uint8_t> read_buffer_;
    
    // Request handler
    RequestHandler handler_;
    
    // Mutex for stream map access
    mutable std::mutex streams_mutex_;
    
public:
    explicit Http2Connection(std::unique_ptr<net::Connection> conn);
    ~Http2Connection();
    
    // Non-copyable, non-movable
    Http2Connection(const Http2Connection&) = delete;
    Http2Connection& operator=(const Http2Connection&) = delete;
    Http2Connection(Http2Connection&&) = delete;
    Http2Connection& operator=(Http2Connection&&) = delete;
    
    // Set request handler
    void set_handler(RequestHandler handler) { handler_ = std::move(handler); }
    
    // Run the connection (main loop)
    Task<void> run();
    
    // Check if connection is open
    bool is_open() const { return is_open_.load(std::memory_order_relaxed); }
    
    // Get active stream count
    size_t active_streams() const { return active_streams_.load(std::memory_order_relaxed); }
    
    // Graceful shutdown
    Task<void> shutdown(ErrorCode error = ErrorCode::NoError, std::string_view debug = {});
    
    // Settings accessors
    const ConnectionSettings& local_settings() const { return local_settings_; }
    const ConnectionSettings& remote_settings() const { return remote_settings_; }
    
    // Flow control
    int32_t connection_window_size() const { return remote_window_size_; }
    void update_connection_window(int32_t delta);
    
    // HPACK access (for streams)
    HpackEncoder& encoder() { return encoder_; }
    HpackDecoder& decoder() { return decoder_; }
    
    // Send frame (for streams)
    Task<expected<void, Error>> send_frame(std::span<const uint8_t> frame_data);
    
    // Get underlying connection (for streams)
    net::Connection& connection() { return *conn_; }
    
private:
    // Connection setup
    Task<expected<void, Error>> send_preface();
    Task<expected<void, Error>> receive_preface();
    
    // Frame processing
    Task<expected<void, Error>> process_frame(const FrameHeader& header, std::span<const uint8_t> payload);
    Task<expected<void, Error>> process_data_frame(const FrameHeader& header, std::span<const uint8_t> payload);
    Task<expected<void, Error>> process_headers_frame(const FrameHeader& header, std::span<const uint8_t> payload);
    Task<expected<void, Error>> process_settings_frame(const FrameHeader& header, std::span<const uint8_t> payload);
    Task<expected<void, Error>> process_ping_frame(const FrameHeader& header, std::span<const uint8_t> payload);
    Task<expected<void, Error>> process_goaway_frame(const FrameHeader& header, std::span<const uint8_t> payload);
    Task<expected<void, Error>> process_window_update_frame(const FrameHeader& header, std::span<const uint8_t> payload);
    Task<expected<void, Error>> process_rst_stream_frame(const FrameHeader& header, std::span<const uint8_t> payload);
    
    // Stream management
    Stream* get_stream(uint32_t stream_id);
    Stream* create_stream(uint32_t stream_id);
    void remove_stream(uint32_t stream_id);
    
    // Handle a complete request on a stream
    Task<void> handle_stream_request(uint32_t stream_id);
    
    // Send GOAWAY frame
    Task<expected<void, Error>> send_goaway(ErrorCode error, std::string_view debug = {});
    
    // Send WINDOW_UPDATE frame
    Task<expected<void, Error>> send_window_update(uint32_t stream_id, uint32_t increment);
    
    // Read more data into buffer
    Task<expected<size_t, Error>> read_more();
};

// ============================================================================
// Protocol Detection
// ============================================================================

// Check if data starts with HTTP/2 connection preface
bool is_http2_preface(std::span<const uint8_t> data);

// Check if this is an HTTP/1.1 upgrade request to h2c
bool is_h2c_upgrade_request(const Request& req);

// Create HTTP/2 connection from upgraded HTTP/1.1 connection
Task<expected<std::shared_ptr<Http2Connection>, Error>> upgrade_to_http2(
    std::unique_ptr<net::Connection> conn,
    const Request& upgrade_request
);

} // namespace coroute::http2
