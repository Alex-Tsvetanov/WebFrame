#pragma once

#include "coroute/net/io_context.hpp"
#include "coroute/core/request.hpp"
#include "coroute/core/response.hpp"
#include "coroute/coro/task.hpp"
#include "coroute/util/expected.hpp"

#include <string>
#include <string_view>
#include <vector>
#include <functional>
#include <memory>
#include <cstdint>
#include <span>

namespace coroute::net {

// ============================================================================
// WebSocket Frame Types (RFC 6455)
// ============================================================================

enum class WebSocketOpcode : uint8_t {
    Continuation = 0x0,
    Text         = 0x1,
    Binary       = 0x2,
    // 0x3-0x7 reserved for non-control frames
    Close        = 0x8,
    Ping         = 0x9,
    Pong         = 0xA,
    // 0xB-0xF reserved for control frames
};

// ============================================================================
// WebSocket Close Codes (RFC 6455 Section 7.4.1)
// ============================================================================

enum class WebSocketCloseCode : uint16_t {
    Normal           = 1000,  // Normal closure
    GoingAway        = 1001,  // Endpoint going away (e.g., server shutdown)
    ProtocolError    = 1002,  // Protocol error
    UnsupportedData  = 1003,  // Unsupported data type
    // 1004 reserved
    NoStatusReceived = 1005,  // No status code (internal use only)
    AbnormalClosure  = 1006,  // Abnormal closure (internal use only)
    InvalidPayload   = 1007,  // Invalid frame payload data
    PolicyViolation  = 1008,  // Policy violation
    MessageTooBig    = 1009,  // Message too big
    MandatoryExt     = 1010,  // Missing mandatory extension
    InternalError    = 1011,  // Internal server error
    TlsHandshake     = 1015,  // TLS handshake failure (internal use only)
};

// ============================================================================
// WebSocket Message
// ============================================================================

struct WebSocketMessage {
    WebSocketOpcode opcode;
    std::vector<uint8_t> data;
    
    // Convenience accessors
    bool is_text() const { return opcode == WebSocketOpcode::Text; }
    bool is_binary() const { return opcode == WebSocketOpcode::Binary; }
    bool is_close() const { return opcode == WebSocketOpcode::Close; }
    bool is_ping() const { return opcode == WebSocketOpcode::Ping; }
    bool is_pong() const { return opcode == WebSocketOpcode::Pong; }
    
    // Get text content (for text messages)
    std::string_view text() const {
        return std::string_view(reinterpret_cast<const char*>(data.data()), data.size());
    }
    
    // Get close code (for close messages)
    WebSocketCloseCode close_code() const {
        if (data.size() >= 2) {
            return static_cast<WebSocketCloseCode>((data[0] << 8) | data[1]);
        }
        return WebSocketCloseCode::NoStatusReceived;
    }
    
    // Get close reason (for close messages)
    std::string_view close_reason() const {
        if (data.size() > 2) {
            return std::string_view(reinterpret_cast<const char*>(data.data() + 2), data.size() - 2);
        }
        return {};
    }
};

// ============================================================================
// WebSocket Connection
// ============================================================================

class WebSocketConnection {
public:
    virtual ~WebSocketConnection() = default;
    
    // Receive next message (blocks until message arrives or connection closes)
    virtual Task<expected<WebSocketMessage, Error>> receive() = 0;
    
    // Send text message
    virtual Task<expected<void, Error>> send_text(std::string_view text) = 0;
    
    // Send binary message
    virtual Task<expected<void, Error>> send_binary(std::span<const uint8_t> data) = 0;
    
    // Send ping
    virtual Task<expected<void, Error>> ping(std::span<const uint8_t> data = {}) = 0;
    
    // Send pong (usually automatic in response to ping)
    virtual Task<expected<void, Error>> pong(std::span<const uint8_t> data = {}) = 0;
    
    // Close connection with code and reason
    virtual Task<expected<void, Error>> close(
        WebSocketCloseCode code = WebSocketCloseCode::Normal,
        std::string_view reason = "") = 0;
    
    // Check if connection is open
    virtual bool is_open() const = 0;
    
    // Get remote address
    virtual std::string remote_address() const = 0;
    
    // Get remote port
    virtual uint16_t remote_port() const = 0;
};

// ============================================================================
// WebSocket Handshake
// ============================================================================

// Check if request is a WebSocket upgrade request
bool is_websocket_upgrade(const Request& req);

// Generate WebSocket accept key from client key
std::string compute_accept_key(std::string_view client_key);

// Create WebSocket upgrade response
Response create_upgrade_response(const Request& req);

// Upgrade an HTTP connection to WebSocket
// Returns nullptr if upgrade fails
Task<expected<std::unique_ptr<WebSocketConnection>, Error>> 
upgrade_to_websocket(std::unique_ptr<Connection> conn, const Request& req);

// ============================================================================
// WebSocket Configuration
// ============================================================================

struct WebSocketConfig {
    // Maximum message size (default 16MB)
    size_t max_message_size = 16 * 1024 * 1024;
    
    // Maximum frame size (default 64KB)
    size_t max_frame_size = 64 * 1024;
    
    // Ping interval (0 = disabled)
    std::chrono::seconds ping_interval{30};
    
    // Pong timeout (0 = no timeout)
    std::chrono::seconds pong_timeout{10};
    
    // Allowed origins (empty = allow all)
    std::vector<std::string> allowed_origins;
    
    // Subprotocols supported by server
    std::vector<std::string> subprotocols;
};

} // namespace coroute::net

namespace coroute {

// Re-export for convenience
using net::WebSocketConnection;
using net::WebSocketMessage;
using net::WebSocketOpcode;
using net::WebSocketCloseCode;
using net::WebSocketConfig;

// ============================================================================
// WebSocket Handler Type
// ============================================================================

// Handler function for WebSocket connections
// Called when a WebSocket connection is established
using WebSocketHandler = std::function<Task<void>(std::unique_ptr<WebSocketConnection>)>;

} // namespace coroute
