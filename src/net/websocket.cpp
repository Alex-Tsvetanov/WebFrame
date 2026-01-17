#include "coroute/net/websocket.hpp"

#include <array>
#include <cstring>
#include <random>

// Use OpenSSL for SHA-1 and Base64 (available on all platforms)
#include <openssl/sha.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/buffer.h>

namespace coroute::net {

// ============================================================================
// SHA-1 and Base64 Helpers
// ============================================================================

namespace {

std::array<uint8_t, 20> sha1(const void* data, size_t len) {
    std::array<uint8_t, 20> hash{};
    SHA1(static_cast<const unsigned char*>(data), len, hash.data());
    return hash;
}

std::string base64_encode(const uint8_t* data, size_t len) {
    BIO* b64 = BIO_new(BIO_f_base64());
    BIO* mem = BIO_new(BIO_s_mem());
    b64 = BIO_push(b64, mem);
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    BIO_write(b64, data, static_cast<int>(len));
    BIO_flush(b64);
    
    BUF_MEM* bufPtr;
    BIO_get_mem_ptr(b64, &bufPtr);
    std::string result(bufPtr->data, bufPtr->length);
    BIO_free_all(b64);
    return result;
}

// WebSocket magic GUID (RFC 6455)
constexpr std::string_view WS_MAGIC_GUID = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

} // anonymous namespace

// ============================================================================
// WebSocket Handshake Functions
// ============================================================================

bool is_websocket_upgrade(const Request& req) {
    // Check for required headers
    auto connection = req.header("Connection");
    auto upgrade = req.header("Upgrade");
    auto ws_key = req.header("Sec-WebSocket-Key");
    auto ws_version = req.header("Sec-WebSocket-Version");
    
    if (!connection || !upgrade || !ws_key || !ws_version) {
        return false;
    }
    
    // Check header values (case-insensitive for Connection and Upgrade)
    bool has_upgrade = false;
    std::string conn_lower(*connection);
    for (auto& c : conn_lower) c = static_cast<char>(std::tolower(c));
    has_upgrade = conn_lower.find("upgrade") != std::string::npos;
    
    std::string upgrade_lower(*upgrade);
    for (auto& c : upgrade_lower) c = static_cast<char>(std::tolower(c));
    bool is_websocket = upgrade_lower == "websocket";
    
    // Version must be 13
    bool version_ok = *ws_version == "13";
    
    return has_upgrade && is_websocket && version_ok;
}

std::string compute_accept_key(std::string_view client_key) {
    // Concatenate client key with magic GUID
    std::string combined;
    combined.reserve(client_key.size() + WS_MAGIC_GUID.size());
    combined.append(client_key);
    combined.append(WS_MAGIC_GUID);
    
    // SHA-1 hash
    auto hash = sha1(combined.data(), combined.size());
    
    // Base64 encode
    return base64_encode(hash.data(), hash.size());
}

Response create_upgrade_response(const Request& req) {
    auto ws_key = req.header("Sec-WebSocket-Key");
    if (!ws_key) {
        return Response::bad_request("Missing Sec-WebSocket-Key header");
    }
    
    std::string accept_key = compute_accept_key(*ws_key);
    
    Response resp;
    resp.set_status(101);
    resp.set_header("Upgrade", "websocket");
    resp.set_header("Connection", "Upgrade");
    resp.set_header("Sec-WebSocket-Accept", accept_key);
    
    // Handle subprotocol negotiation if requested
    auto ws_protocol = req.header("Sec-WebSocket-Protocol");
    if (ws_protocol) {
        // For now, just echo back the first requested protocol
        // In a real implementation, this should be configurable
        auto comma = ws_protocol->find(',');
        if (comma != std::string_view::npos) {
            resp.set_header("Sec-WebSocket-Protocol", std::string(ws_protocol->substr(0, comma)));
        } else {
            resp.set_header("Sec-WebSocket-Protocol", std::string(*ws_protocol));
        }
    }
    
    return resp;
}

// ============================================================================
// WebSocket Frame Implementation
// ============================================================================

namespace {

// Frame header structure:
// Byte 0: FIN (1 bit) + RSV1-3 (3 bits) + Opcode (4 bits)
// Byte 1: MASK (1 bit) + Payload length (7 bits)
// If payload length == 126: next 2 bytes are actual length (16-bit)
// If payload length == 127: next 8 bytes are actual length (64-bit)
// If MASK bit set: next 4 bytes are masking key

struct FrameHeader {
    bool fin = true;
    bool rsv1 = false;
    bool rsv2 = false;
    bool rsv3 = false;
    WebSocketOpcode opcode = WebSocketOpcode::Text;
    bool masked = false;
    uint64_t payload_length = 0;
    std::array<uint8_t, 4> mask_key{};
};

// Parse frame header from buffer
// Returns number of bytes consumed, or 0 if not enough data
size_t parse_frame_header(const uint8_t* data, size_t len, FrameHeader& header) {
    if (len < 2) return 0;
    
    header.fin = (data[0] & 0x80) != 0;
    header.rsv1 = (data[0] & 0x40) != 0;
    header.rsv2 = (data[0] & 0x20) != 0;
    header.rsv3 = (data[0] & 0x10) != 0;
    header.opcode = static_cast<WebSocketOpcode>(data[0] & 0x0F);
    header.masked = (data[1] & 0x80) != 0;
    
    uint8_t len_byte = data[1] & 0x7F;
    size_t header_size = 2;
    
    if (len_byte <= 125) {
        header.payload_length = len_byte;
    } else if (len_byte == 126) {
        if (len < 4) return 0;
        header.payload_length = (static_cast<uint64_t>(data[2]) << 8) | data[3];
        header_size = 4;
    } else { // 127
        if (len < 10) return 0;
        header.payload_length = 0;
        for (int i = 0; i < 8; ++i) {
            header.payload_length = (header.payload_length << 8) | data[2 + i];
        }
        header_size = 10;
    }
    
    if (header.masked) {
        if (len < header_size + 4) return 0;
        std::memcpy(header.mask_key.data(), data + header_size, 4);
        header_size += 4;
    }
    
    return header_size;
}

// Serialize frame header
std::vector<uint8_t> serialize_frame_header(const FrameHeader& header) {
    std::vector<uint8_t> result;
    
    uint8_t byte0 = static_cast<uint8_t>(header.opcode);
    if (header.fin) byte0 |= 0x80;
    if (header.rsv1) byte0 |= 0x40;
    if (header.rsv2) byte0 |= 0x20;
    if (header.rsv3) byte0 |= 0x10;
    result.push_back(byte0);
    
    uint8_t byte1 = header.masked ? 0x80 : 0x00;
    if (header.payload_length <= 125) {
        byte1 |= static_cast<uint8_t>(header.payload_length);
        result.push_back(byte1);
    } else if (header.payload_length <= 0xFFFF) {
        byte1 |= 126;
        result.push_back(byte1);
        result.push_back(static_cast<uint8_t>(header.payload_length >> 8));
        result.push_back(static_cast<uint8_t>(header.payload_length & 0xFF));
    } else {
        byte1 |= 127;
        result.push_back(byte1);
        for (int i = 7; i >= 0; --i) {
            result.push_back(static_cast<uint8_t>(header.payload_length >> (i * 8)));
        }
    }
    
    if (header.masked) {
        result.insert(result.end(), header.mask_key.begin(), header.mask_key.end());
    }
    
    return result;
}

// Apply/remove XOR mask to payload
void apply_mask(uint8_t* data, size_t len, const std::array<uint8_t, 4>& mask) {
    for (size_t i = 0; i < len; ++i) {
        data[i] ^= mask[i % 4];
    }
}

} // anonymous namespace

// ============================================================================
// WebSocket Connection Implementation
// ============================================================================

class WebSocketConnectionImpl : public WebSocketConnection {
    std::unique_ptr<Connection> conn_;
    std::vector<uint8_t> read_buffer_;
    std::vector<uint8_t> fragment_buffer_;
    WebSocketOpcode fragment_opcode_ = WebSocketOpcode::Text;
    bool is_open_ = true;
    size_t max_message_size_ = 16 * 1024 * 1024;
    
public:
    explicit WebSocketConnectionImpl(std::unique_ptr<Connection> conn)
        : conn_(std::move(conn))
    {
        read_buffer_.reserve(64 * 1024);
    }
    
    Task<expected<WebSocketMessage, Error>> receive() override {
        while (is_open_) {
            // Try to parse a frame from existing buffer
            if (!read_buffer_.empty()) {
                FrameHeader header;
                size_t header_size = parse_frame_header(read_buffer_.data(), read_buffer_.size(), header);
                
                if (header_size > 0 && read_buffer_.size() >= header_size + header.payload_length) {
                    // We have a complete frame
                    std::vector<uint8_t> payload(
                        read_buffer_.begin() + header_size,
                        read_buffer_.begin() + header_size + header.payload_length
                    );
                    
                    // Remove frame from buffer
                    read_buffer_.erase(
                        read_buffer_.begin(),
                        read_buffer_.begin() + header_size + header.payload_length
                    );
                    
                    // Unmask if needed (client frames must be masked)
                    if (header.masked) {
                        apply_mask(payload.data(), payload.size(), header.mask_key);
                    }
                    
                    // Handle control frames immediately
                    if (header.opcode == WebSocketOpcode::Ping) {
                        co_await pong(payload);
                        continue;
                    }
                    
                    if (header.opcode == WebSocketOpcode::Pong) {
                        // Ignore pongs for now
                        continue;
                    }
                    
                    if (header.opcode == WebSocketOpcode::Close) {
                        is_open_ = false;
                        // Echo close frame
                        WebSocketCloseCode code = WebSocketCloseCode::Normal;
                        std::string_view reason;
                        if (payload.size() >= 2) {
                            code = static_cast<WebSocketCloseCode>((payload[0] << 8) | payload[1]);
                            if (payload.size() > 2) {
                                reason = std::string_view(
                                    reinterpret_cast<const char*>(payload.data() + 2),
                                    payload.size() - 2
                                );
                            }
                        }
                        co_await send_close_frame(code, reason);
                        co_return WebSocketMessage{WebSocketOpcode::Close, std::move(payload)};
                    }
                    
                    // Handle data frames
                    if (header.opcode == WebSocketOpcode::Continuation) {
                        // Continuation of fragmented message
                        fragment_buffer_.insert(fragment_buffer_.end(), payload.begin(), payload.end());
                        
                        if (fragment_buffer_.size() > max_message_size_) {
                            co_await close(WebSocketCloseCode::MessageTooBig, "Message too large");
                            co_return unexpected(Error::io(IoError::Unknown, "Message too large"));
                        }
                        
                        if (header.fin) {
                            // Final fragment
                            WebSocketMessage msg{fragment_opcode_, std::move(fragment_buffer_)};
                            fragment_buffer_.clear();
                            co_return msg;
                        }
                    } else {
                        // New message
                        if (!fragment_buffer_.empty()) {
                            // Protocol error: new message while fragmented message incomplete
                            co_await close(WebSocketCloseCode::ProtocolError, "Incomplete fragmented message");
                            co_return unexpected(Error::io(IoError::Unknown, "Protocol error"));
                        }
                        
                        if (header.fin) {
                            // Complete message in single frame
                            co_return WebSocketMessage{header.opcode, std::move(payload)};
                        } else {
                            // Start of fragmented message
                            fragment_opcode_ = header.opcode;
                            fragment_buffer_ = std::move(payload);
                        }
                    }
                    
                    continue;
                }
            }
            
            // Need more data
            std::array<uint8_t, 8192> buf;
            auto result = co_await conn_->async_read(buf.data(), buf.size());
            if (!result) {
                is_open_ = false;
                co_return unexpected(result.error());
            }
            if (*result == 0) {
                is_open_ = false;
                co_return unexpected(Error::io(IoError::ConnectionReset, "Connection closed"));
            }
            
            read_buffer_.insert(read_buffer_.end(), buf.begin(), buf.begin() + *result);
        }
        
        co_return unexpected(Error::io(IoError::ConnectionReset, "Connection closed"));
    }
    
    Task<expected<void, Error>> send_text(std::string_view text) override {
        co_return co_await send_frame(WebSocketOpcode::Text, 
            reinterpret_cast<const uint8_t*>(text.data()), text.size());
    }
    
    Task<expected<void, Error>> send_binary(std::span<const uint8_t> data) override {
        co_return co_await send_frame(WebSocketOpcode::Binary, data.data(), data.size());
    }
    
    Task<expected<void, Error>> ping(std::span<const uint8_t> data) override {
        co_return co_await send_frame(WebSocketOpcode::Ping, data.data(), data.size());
    }
    
    Task<expected<void, Error>> pong(std::span<const uint8_t> data) override {
        co_return co_await send_frame(WebSocketOpcode::Pong, data.data(), data.size());
    }
    
    Task<expected<void, Error>> close(WebSocketCloseCode code, std::string_view reason) override {
        if (!is_open_) {
            co_return expected<void, Error>{};
        }
        is_open_ = false;
        co_return co_await send_close_frame(code, reason);
    }
    
    bool is_open() const override { return is_open_; }
    
    std::string remote_address() const override { return conn_->remote_address(); }
    
    uint16_t remote_port() const override { return conn_->remote_port(); }
    
private:
    Task<expected<void, Error>> send_frame(WebSocketOpcode opcode, const uint8_t* data, size_t len) {
        FrameHeader header;
        header.fin = true;
        header.opcode = opcode;
        header.masked = false;  // Server frames are not masked
        header.payload_length = len;
        
        auto header_bytes = serialize_frame_header(header);
        
        // Send header
        auto header_result = co_await conn_->async_write_all(header_bytes.data(), header_bytes.size());
        if (!header_result) co_return unexpected(header_result.error());
        
        // Send payload
        if (len > 0) {
            auto payload_result = co_await conn_->async_write_all(data, len);
            if (!payload_result) co_return unexpected(payload_result.error());
        }
        
        co_return expected<void, Error>{};
    }
    
    Task<expected<void, Error>> send_close_frame(WebSocketCloseCode code, std::string_view reason) {
        std::vector<uint8_t> payload;
        payload.push_back(static_cast<uint8_t>(static_cast<uint16_t>(code) >> 8));
        payload.push_back(static_cast<uint8_t>(static_cast<uint16_t>(code) & 0xFF));
        payload.insert(payload.end(), reason.begin(), reason.end());
        
        co_return co_await send_frame(WebSocketOpcode::Close, payload.data(), payload.size());
    }
};

// ============================================================================
// Upgrade Function
// ============================================================================

Task<expected<std::unique_ptr<WebSocketConnection>, Error>> 
upgrade_to_websocket(std::unique_ptr<Connection> conn, const Request& req) {
    if (!is_websocket_upgrade(req)) {
        co_return unexpected(Error::io(IoError::InvalidArgument, "Not a WebSocket upgrade request"));
    }
    
    // Create and send upgrade response
    auto response = create_upgrade_response(req);
    
    // Serialize response
    std::string resp_str;
    resp_str += "HTTP/1.1 ";
    resp_str += std::to_string(response.status());
    resp_str += " ";
    resp_str += response.status_text();
    resp_str += "\r\n";
    
    for (const auto& [name, value] : response.headers()) {
        resp_str += name;
        resp_str += ": ";
        resp_str += value;
        resp_str += "\r\n";
    }
    resp_str += "\r\n";
    
    auto result = co_await conn->async_write_all(resp_str.data(), resp_str.size());
    if (!result) {
        co_return unexpected(result.error());
    }
    
    co_return std::make_unique<WebSocketConnectionImpl>(std::move(conn));
}

} // namespace coroute::net
