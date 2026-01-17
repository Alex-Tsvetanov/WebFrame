#include "coroute/http2/connection.hpp"

#include <cstring>

namespace coroute::http2 {

// ============================================================================
// Connection Settings
// ============================================================================

void ConnectionSettings::apply(SettingsId id, uint32_t value) {
    switch (id) {
        case SettingsId::HeaderTableSize:
            header_table_size = value;
            break;
        case SettingsId::EnablePush:
            enable_push = value;
            break;
        case SettingsId::MaxConcurrentStreams:
            max_concurrent_streams = value;
            break;
        case SettingsId::InitialWindowSize:
            initial_window_size = value;
            break;
        case SettingsId::MaxFrameSize:
            max_frame_size = value;
            break;
        case SettingsId::MaxHeaderListSize:
            max_header_list_size = value;
            break;
    }
}

// ============================================================================
// HTTP/2 Connection
// ============================================================================

Http2Connection::Http2Connection(std::unique_ptr<net::Connection> conn)
    : conn_(std::move(conn))
    , local_window_size_(Constants::DefaultInitialWindowSize)
    , remote_window_size_(Constants::DefaultInitialWindowSize)
{
    read_buffer_.reserve(64 * 1024);
}

Http2Connection::~Http2Connection() {
    is_open_.store(false, std::memory_order_relaxed);
}

Task<void> Http2Connection::run() {
    // Send our SETTINGS
    auto preface_result = co_await send_preface();
    if (!preface_result) {
        co_return;
    }
    
    // Receive client preface
    auto recv_preface_result = co_await receive_preface();
    if (!recv_preface_result) {
        co_return;
    }
    
    // Main frame processing loop
    while (is_open_.load(std::memory_order_relaxed)) {
        // Ensure we have at least a frame header
        while (read_buffer_.size() < Constants::FrameHeaderSize) {
            auto read_result = co_await read_more();
            if (!read_result) {
                is_open_.store(false, std::memory_order_relaxed);
                co_return;
            }
            if (*read_result == 0) {
                // Connection closed
                is_open_.store(false, std::memory_order_relaxed);
                co_return;
            }
        }
        
        // Parse frame header
        auto header_result = FrameHeader::parse(read_buffer_);
        if (!header_result) {
            co_await send_goaway(ErrorCode::ProtocolError, "Invalid frame header");
            co_return;
        }
        
        const auto& header = *header_result;
        
        // Validate frame size
        if (header.length > local_settings_.max_frame_size) {
            co_await send_goaway(ErrorCode::FrameSizeError, "Frame too large");
            co_return;
        }
        
        // Ensure we have the full frame
        size_t frame_size = Constants::FrameHeaderSize + header.length;
        while (read_buffer_.size() < frame_size) {
            auto read_result = co_await read_more();
            if (!read_result || *read_result == 0) {
                is_open_.store(false, std::memory_order_relaxed);
                co_return;
            }
        }
        
        // Extract payload
        std::span<const uint8_t> payload(
            read_buffer_.data() + Constants::FrameHeaderSize,
            header.length
        );
        
        // Process frame
        auto process_result = co_await process_frame(header, payload);
        if (!process_result) {
            // Error already handled (GOAWAY sent or connection closed)
            co_return;
        }
        
        // Remove processed frame from buffer
        read_buffer_.erase(read_buffer_.begin(), read_buffer_.begin() + frame_size);
    }
}

Task<void> Http2Connection::shutdown(ErrorCode error, std::string_view debug) {
    if (goaway_sent_) {
        co_return;
    }
    
    co_await send_goaway(error, debug);
    is_open_.store(false, std::memory_order_relaxed);
}

void Http2Connection::update_connection_window(int32_t delta) {
    remote_window_size_ += delta;
}

Task<expected<void, Error>> Http2Connection::send_frame(std::span<const uint8_t> frame_data) {
    auto result = co_await conn_->async_write_all(frame_data.data(), frame_data.size());
    if (!result) {
        co_return unexpected(result.error());
    }
    co_return expected<void, Error>{};
}

// ============================================================================
// Connection Setup
// ============================================================================

Task<expected<void, Error>> Http2Connection::send_preface() {
    // Server sends SETTINGS frame as preface
    std::vector<SettingsEntry> settings = {
        {SettingsId::MaxConcurrentStreams, local_settings_.max_concurrent_streams},
        {SettingsId::InitialWindowSize, local_settings_.initial_window_size},
        {SettingsId::MaxFrameSize, local_settings_.max_frame_size},
        {SettingsId::MaxHeaderListSize, local_settings_.max_header_list_size},
        {SettingsId::EnablePush, 0},  // We don't support server push
    };
    
    auto frame = serialize_settings_frame(settings, false);
    co_return co_await send_frame(frame);
}

Task<expected<void, Error>> Http2Connection::receive_preface() {
    // Client must send connection preface: magic string + SETTINGS
    constexpr size_t preface_len = Constants::ClientPreface.size();
    
    // Read until we have the preface
    while (read_buffer_.size() < preface_len) {
        auto result = co_await read_more();
        if (!result) {
            co_return unexpected(result.error());
        }
        if (*result == 0) {
            co_return unexpected(Error::io(IoError::ConnectionReset, "Connection closed during preface"));
        }
    }
    
    // Verify magic string
    if (std::memcmp(read_buffer_.data(), Constants::ClientPreface.data(), preface_len) != 0) {
        co_await send_goaway(ErrorCode::ProtocolError, "Invalid connection preface");
        co_return unexpected(Error::io(IoError::InvalidArgument, "Invalid HTTP/2 preface"));
    }
    
    // Remove preface from buffer
    read_buffer_.erase(read_buffer_.begin(), read_buffer_.begin() + preface_len);
    preface_received_ = true;
    
    co_return expected<void, Error>{};
}

// ============================================================================
// Frame Processing
// ============================================================================

Task<expected<void, Error>> Http2Connection::process_frame(
    const FrameHeader& header,
    std::span<const uint8_t> payload
) {
    switch (header.type) {
        case FrameType::Data:
            co_return co_await process_data_frame(header, payload);
            
        case FrameType::Headers:
            co_return co_await process_headers_frame(header, payload);
            
        case FrameType::Settings:
            co_return co_await process_settings_frame(header, payload);
            
        case FrameType::Ping:
            co_return co_await process_ping_frame(header, payload);
            
        case FrameType::GoAway:
            co_return co_await process_goaway_frame(header, payload);
            
        case FrameType::WindowUpdate:
            co_return co_await process_window_update_frame(header, payload);
            
        case FrameType::RstStream:
            co_return co_await process_rst_stream_frame(header, payload);
            
        case FrameType::Priority:
            // Ignore priority frames (deprecated)
            co_return expected<void, Error>{};
            
        case FrameType::PushPromise:
            // We don't support server push, and clients shouldn't send this
            co_await send_goaway(ErrorCode::ProtocolError, "PUSH_PROMISE not allowed from client");
            co_return unexpected(Error::io(IoError::InvalidArgument, "PUSH_PROMISE not supported"));
            
        case FrameType::Continuation:
            // Should be handled as part of HEADERS processing
            co_await send_goaway(ErrorCode::ProtocolError, "Unexpected CONTINUATION");
            co_return unexpected(Error::io(IoError::InvalidArgument, "Unexpected CONTINUATION"));
            
        default:
            // Unknown frame types must be ignored (RFC 7540 Section 4.1)
            co_return expected<void, Error>{};
    }
}

Task<expected<void, Error>> Http2Connection::process_data_frame(
    const FrameHeader& header,
    std::span<const uint8_t> payload
) {
    if (header.stream_id == 0) {
        co_await send_goaway(ErrorCode::ProtocolError, "DATA on stream 0");
        co_return unexpected(Error::io(IoError::InvalidArgument, "DATA on stream 0"));
    }
    
    auto* stream = get_stream(header.stream_id);
    if (!stream) {
        // Stream doesn't exist - send RST_STREAM
        auto rst = serialize_rst_stream_frame(header.stream_id, ErrorCode::StreamClosed);
        co_await send_frame(rst);
        co_return expected<void, Error>{};
    }
    
    // Handle padding if present
    std::span<const uint8_t> data = payload;
    if (header.has_padded() && !payload.empty()) {
        uint8_t pad_length = payload[0];
        if (pad_length >= payload.size()) {
            co_await send_goaway(ErrorCode::ProtocolError, "Invalid padding");
            co_return unexpected(Error::io(IoError::InvalidArgument, "Invalid padding"));
        }
        data = payload.subspan(1, payload.size() - 1 - pad_length);
    }
    
    // Pass data to stream
    stream->receive_data(data, header.has_end_stream());
    
    // Update flow control window
    local_window_size_ -= static_cast<int32_t>(payload.size());
    stream->update_local_window(-static_cast<int32_t>(payload.size()));
    
    // Send WINDOW_UPDATE if needed
    if (local_window_size_ < Constants::DefaultInitialWindowSize / 2) {
        int32_t increment = Constants::DefaultInitialWindowSize - local_window_size_;
        co_await send_window_update(0, static_cast<uint32_t>(increment));
        local_window_size_ += increment;
    }
    
    // Check if request is complete
    if (stream->is_request_complete() && handler_) {
        // Handle request in separate coroutine
        handle_stream_request(header.stream_id).start_detached();
    }
    
    co_return expected<void, Error>{};
}

Task<expected<void, Error>> Http2Connection::process_headers_frame(
    const FrameHeader& header,
    std::span<const uint8_t> payload
) {
    if (header.stream_id == 0) {
        co_await send_goaway(ErrorCode::ProtocolError, "HEADERS on stream 0");
        co_return unexpected(Error::io(IoError::InvalidArgument, "HEADERS on stream 0"));
    }
    
    // Handle padding and priority
    std::span<const uint8_t> header_block = payload;
    size_t offset = 0;
    size_t pad_length = 0;
    
    // PADDED flag - first byte is pad length
    if (header.has_padded() && !payload.empty()) {
        pad_length = payload[0];
        offset = 1;
    }
    
    // PRIORITY flag - 5 bytes of priority data (stream dependency + weight)
    if (header.has_priority()) {
        offset += 5;  // Skip E + Stream Dependency (4 bytes) + Weight (1 byte)
    }
    
    // Validate we have enough data
    if (offset + pad_length > payload.size()) {
        co_await send_goaway(ErrorCode::ProtocolError, "Invalid padding/priority");
        co_return unexpected(Error::io(IoError::InvalidArgument, "Invalid padding/priority"));
    }
    
    header_block = payload.subspan(offset, payload.size() - offset - pad_length);
    
    // TODO: Handle CONTINUATION frames if END_HEADERS is not set
    if (!header.has_end_headers()) {
        // For now, we don't support fragmented headers
        co_await send_goaway(ErrorCode::InternalError, "CONTINUATION not yet supported");
        co_return unexpected(Error::io(IoError::Unknown, "CONTINUATION not supported"));
    }
    
    // Decode headers
    auto headers_result = decoder_.decode(header_block);
    if (!headers_result) {
        co_await send_goaway(ErrorCode::CompressionError, "HPACK decode failed");
        co_return unexpected(headers_result.error());
    }
    
    // Validate headers
    if (!validate_request_headers(*headers_result)) {
        auto rst = serialize_rst_stream_frame(header.stream_id, ErrorCode::ProtocolError);
        co_await send_frame(rst);
        co_return expected<void, Error>{};
    }
    
    // Create or get stream
    auto* stream = get_stream(header.stream_id);
    if (!stream) {
        // New stream
        if (header.stream_id <= last_stream_id_) {
            co_await send_goaway(ErrorCode::ProtocolError, "Stream ID not increasing");
            co_return unexpected(Error::io(IoError::InvalidArgument, "Invalid stream ID"));
        }
        
        // Check concurrent stream limit
        if (active_streams_.load() >= local_settings_.max_concurrent_streams) {
            auto rst = serialize_rst_stream_frame(header.stream_id, ErrorCode::RefusedStream);
            co_await send_frame(rst);
            co_return expected<void, Error>{};
        }
        
        stream = create_stream(header.stream_id);
        last_stream_id_ = header.stream_id;
    }
    
    // Pass headers to stream
    stream->receive_headers(std::move(*headers_result), header.has_end_stream());
    
    // Check if request is complete (no body expected)
    if (stream->is_request_complete() && handler_) {
        handle_stream_request(header.stream_id).start_detached();
    }
    
    co_return expected<void, Error>{};
}

Task<expected<void, Error>> Http2Connection::process_settings_frame(
    const FrameHeader& header,
    std::span<const uint8_t> payload
) {
    if (header.stream_id != 0) {
        co_await send_goaway(ErrorCode::ProtocolError, "SETTINGS on non-zero stream");
        co_return unexpected(Error::io(IoError::InvalidArgument, "SETTINGS on non-zero stream"));
    }
    
    if (header.has_ack()) {
        // ACK for our SETTINGS
        if (payload.size() != 0) {
            co_await send_goaway(ErrorCode::FrameSizeError, "SETTINGS ACK with payload");
            co_return unexpected(Error::io(IoError::InvalidArgument, "SETTINGS ACK with payload"));
        }
        settings_ack_received_ = true;
        co_return expected<void, Error>{};
    }
    
    // Parse settings
    if (payload.size() % 6 != 0) {
        co_await send_goaway(ErrorCode::FrameSizeError, "Invalid SETTINGS size");
        co_return unexpected(Error::io(IoError::InvalidArgument, "Invalid SETTINGS size"));
    }
    
    for (size_t i = 0; i < payload.size(); i += 6) {
        uint16_t id = (static_cast<uint16_t>(payload[i]) << 8) | payload[i + 1];
        uint32_t value = (static_cast<uint32_t>(payload[i + 2]) << 24) |
                         (static_cast<uint32_t>(payload[i + 3]) << 16) |
                         (static_cast<uint32_t>(payload[i + 4]) << 8) |
                         static_cast<uint32_t>(payload[i + 5]);
        
        // Validate settings
        switch (static_cast<SettingsId>(id)) {
            case SettingsId::EnablePush:
                if (value > 1) {
                    co_await send_goaway(ErrorCode::ProtocolError, "Invalid ENABLE_PUSH value");
                    co_return unexpected(Error::io(IoError::InvalidArgument, "Invalid ENABLE_PUSH"));
                }
                break;
                
            case SettingsId::InitialWindowSize:
                if (value > Constants::MaxWindowSize) {
                    co_await send_goaway(ErrorCode::FlowControlError, "Window size too large");
                    co_return unexpected(Error::io(IoError::InvalidArgument, "Window too large"));
                }
                // Update all stream windows
                {
                    std::lock_guard lock(streams_mutex_);
                    int32_t delta = static_cast<int32_t>(value) - 
                                    static_cast<int32_t>(remote_settings_.initial_window_size);
                    for (auto& [id, stream] : streams_) {
                        stream->update_remote_window(delta);
                    }
                }
                break;
                
            case SettingsId::MaxFrameSize:
                if (value < Constants::MinMaxFrameSize || value > Constants::MaxMaxFrameSize) {
                    co_await send_goaway(ErrorCode::ProtocolError, "Invalid MAX_FRAME_SIZE");
                    co_return unexpected(Error::io(IoError::InvalidArgument, "Invalid MAX_FRAME_SIZE"));
                }
                break;
                
            default:
                // Unknown settings are ignored
                break;
        }
        
        remote_settings_.apply(static_cast<SettingsId>(id), value);
    }
    
    // Update HPACK encoder table size if changed
    encoder_.set_max_table_size(remote_settings_.header_table_size);
    
    // Send ACK
    auto ack = serialize_settings_ack();
    co_return co_await send_frame(ack);
}

Task<expected<void, Error>> Http2Connection::process_ping_frame(
    const FrameHeader& header,
    std::span<const uint8_t> payload
) {
    if (header.stream_id != 0) {
        co_await send_goaway(ErrorCode::ProtocolError, "PING on non-zero stream");
        co_return unexpected(Error::io(IoError::InvalidArgument, "PING on non-zero stream"));
    }
    
    if (payload.size() != 8) {
        co_await send_goaway(ErrorCode::FrameSizeError, "Invalid PING size");
        co_return unexpected(Error::io(IoError::InvalidArgument, "Invalid PING size"));
    }
    
    if (header.has_ack()) {
        // Response to our PING - ignore for now
        co_return expected<void, Error>{};
    }
    
    // Send PING ACK with same opaque data
    std::array<uint8_t, 8> opaque_data;
    std::memcpy(opaque_data.data(), payload.data(), 8);
    auto pong = serialize_ping_frame(opaque_data, true);
    co_return co_await send_frame(pong);
}

Task<expected<void, Error>> Http2Connection::process_goaway_frame(
    const FrameHeader& header,
    std::span<const uint8_t> payload
) {
    if (header.stream_id != 0) {
        co_await send_goaway(ErrorCode::ProtocolError, "GOAWAY on non-zero stream");
        co_return unexpected(Error::io(IoError::InvalidArgument, "GOAWAY on non-zero stream"));
    }
    
    if (payload.size() < 8) {
        co_return unexpected(Error::io(IoError::InvalidArgument, "Invalid GOAWAY size"));
    }
    
    // Parse GOAWAY
    uint32_t last_stream = ((static_cast<uint32_t>(payload[0]) & 0x7F) << 24) |
                           (static_cast<uint32_t>(payload[1]) << 16) |
                           (static_cast<uint32_t>(payload[2]) << 8) |
                           static_cast<uint32_t>(payload[3]);
    
    uint32_t error = (static_cast<uint32_t>(payload[4]) << 24) |
                     (static_cast<uint32_t>(payload[5]) << 16) |
                     (static_cast<uint32_t>(payload[6]) << 8) |
                     static_cast<uint32_t>(payload[7]);
    
    (void)last_stream;  // We don't retry streams
    goaway_error_ = static_cast<ErrorCode>(error);
    
    // Close connection
    is_open_.store(false, std::memory_order_relaxed);
    
    co_return expected<void, Error>{};
}

Task<expected<void, Error>> Http2Connection::process_window_update_frame(
    const FrameHeader& header,
    std::span<const uint8_t> payload
) {
    if (payload.size() != 4) {
        co_await send_goaway(ErrorCode::FrameSizeError, "Invalid WINDOW_UPDATE size");
        co_return unexpected(Error::io(IoError::InvalidArgument, "Invalid WINDOW_UPDATE size"));
    }
    
    uint32_t increment = ((static_cast<uint32_t>(payload[0]) & 0x7F) << 24) |
                         (static_cast<uint32_t>(payload[1]) << 16) |
                         (static_cast<uint32_t>(payload[2]) << 8) |
                         static_cast<uint32_t>(payload[3]);
    
    if (increment == 0) {
        if (header.stream_id == 0) {
            co_await send_goaway(ErrorCode::ProtocolError, "Zero WINDOW_UPDATE increment");
        } else {
            auto rst = serialize_rst_stream_frame(header.stream_id, ErrorCode::ProtocolError);
            co_await send_frame(rst);
        }
        co_return expected<void, Error>{};
    }
    
    if (header.stream_id == 0) {
        // Connection-level window update
        remote_window_size_ += static_cast<int32_t>(increment);
        if (remote_window_size_ > static_cast<int32_t>(Constants::MaxWindowSize)) {
            co_await send_goaway(ErrorCode::FlowControlError, "Window overflow");
            co_return unexpected(Error::io(IoError::InvalidArgument, "Window overflow"));
        }
    } else {
        // Stream-level window update
        auto* stream = get_stream(header.stream_id);
        if (stream) {
            stream->update_remote_window(static_cast<int32_t>(increment));
        }
    }
    
    co_return expected<void, Error>{};
}

Task<expected<void, Error>> Http2Connection::process_rst_stream_frame(
    const FrameHeader& header,
    std::span<const uint8_t> payload
) {
    if (header.stream_id == 0) {
        co_await send_goaway(ErrorCode::ProtocolError, "RST_STREAM on stream 0");
        co_return unexpected(Error::io(IoError::InvalidArgument, "RST_STREAM on stream 0"));
    }
    
    if (payload.size() != 4) {
        co_await send_goaway(ErrorCode::FrameSizeError, "Invalid RST_STREAM size");
        co_return unexpected(Error::io(IoError::InvalidArgument, "Invalid RST_STREAM size"));
    }
    
    // Remove stream
    remove_stream(header.stream_id);
    
    co_return expected<void, Error>{};
}

// ============================================================================
// Stream Management
// ============================================================================

Stream* Http2Connection::get_stream(uint32_t stream_id) {
    std::lock_guard lock(streams_mutex_);
    auto it = streams_.find(stream_id);
    return it != streams_.end() ? it->second.get() : nullptr;
}

Stream* Http2Connection::create_stream(uint32_t stream_id) {
    std::lock_guard lock(streams_mutex_);
    auto stream = std::make_unique<Stream>(
        stream_id,
        this,
        static_cast<int32_t>(local_settings_.initial_window_size)
    );
    auto* ptr = stream.get();
    streams_[stream_id] = std::move(stream);
    active_streams_.fetch_add(1, std::memory_order_relaxed);
    return ptr;
}

void Http2Connection::remove_stream(uint32_t stream_id) {
    std::lock_guard lock(streams_mutex_);
    if (streams_.erase(stream_id) > 0) {
        active_streams_.fetch_sub(1, std::memory_order_relaxed);
    }
}

Task<void> Http2Connection::handle_stream_request(uint32_t stream_id) {
    auto* stream = get_stream(stream_id);
    if (!stream || !handler_) {
        co_return;
    }
    
    // Build request
    auto req_result = stream->build_request();
    if (!req_result) {
        stream->reset(ErrorCode::InternalError);
        remove_stream(stream_id);
        co_return;
    }
    
    // Call handler
    Response response;
    
    try {
        response = co_await handler_(*req_result);
    } catch (const std::exception& e) {
        response = Response::internal_error(e.what());
    } catch (...) {
        response = Response::internal_error("Unknown error");
    }
    
    // Send response
    co_await stream->send_response(response);
    
    remove_stream(stream_id);
}

Task<expected<void, Error>> Http2Connection::send_goaway(ErrorCode error, std::string_view debug) {
    if (goaway_sent_) {
        co_return expected<void, Error>{};
    }
    
    goaway_sent_ = true;
    auto frame = serialize_goaway_frame(last_stream_id_, error, debug);
    co_return co_await send_frame(frame);
}

Task<expected<void, Error>> Http2Connection::send_window_update(uint32_t stream_id, uint32_t increment) {
    auto frame = serialize_window_update_frame(stream_id, increment);
    co_return co_await send_frame(frame);
}

Task<expected<size_t, Error>> Http2Connection::read_more() {
    std::array<uint8_t, 16384> buf;
    auto result = co_await conn_->async_read(buf.data(), buf.size());
    if (!result) {
        co_return unexpected(result.error());
    }
    
    read_buffer_.insert(read_buffer_.end(), buf.begin(), buf.begin() + *result);
    co_return *result;
}

// ============================================================================
// Protocol Detection
// ============================================================================

bool is_http2_preface(std::span<const uint8_t> data) {
    if (data.size() < Constants::ClientPreface.size()) {
        return false;
    }
    return std::memcmp(data.data(), Constants::ClientPreface.data(), 
                       Constants::ClientPreface.size()) == 0;
}

bool is_h2c_upgrade_request(const Request& req) {
    auto connection = req.header("Connection");
    auto upgrade = req.header("Upgrade");
    auto settings = req.header("HTTP2-Settings");
    
    if (!connection || !upgrade || !settings) {
        return false;
    }
    
    // Check for upgrade token
    std::string conn_lower(*connection);
    for (auto& c : conn_lower) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    
    bool has_upgrade = conn_lower.find("upgrade") != std::string::npos;
    bool has_settings = conn_lower.find("http2-settings") != std::string::npos;
    
    std::string upgrade_lower(*upgrade);
    for (auto& c : upgrade_lower) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    bool is_h2c = upgrade_lower == "h2c";
    
    return has_upgrade && has_settings && is_h2c;
}

Task<expected<std::shared_ptr<Http2Connection>, Error>> upgrade_to_http2(
    std::unique_ptr<net::Connection> conn,
    const Request& upgrade_request
) {
    // Send 101 Switching Protocols response
    std::string response = 
        "HTTP/1.1 101 Switching Protocols\r\n"
        "Connection: Upgrade\r\n"
        "Upgrade: h2c\r\n"
        "\r\n";
    
    auto write_result = co_await conn->async_write_all(response.data(), response.size());
    if (!write_result) {
        co_return unexpected(write_result.error());
    }
    
    // Create HTTP/2 connection
    auto h2_conn = std::make_shared<Http2Connection>(std::move(conn));
    
    // TODO: Process the HTTP2-Settings header from upgrade request
    // For now, we use default settings
    
    co_return h2_conn;
}

} // namespace coroute::http2
