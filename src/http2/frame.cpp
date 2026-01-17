#include "coroute/http2/frame.hpp"

#include <cstring>

namespace coroute::http2 {

// ============================================================================
// Frame Header
// ============================================================================

std::array<uint8_t, 9> FrameHeader::serialize() const {
    std::array<uint8_t, 9> result{};
    
    // Length (24 bits, big-endian)
    result[0] = static_cast<uint8_t>((length >> 16) & 0xFF);
    result[1] = static_cast<uint8_t>((length >> 8) & 0xFF);
    result[2] = static_cast<uint8_t>(length & 0xFF);
    
    // Type (8 bits)
    result[3] = static_cast<uint8_t>(type);
    
    // Flags (8 bits)
    result[4] = flags;
    
    // Stream ID (31 bits, big-endian, high bit reserved/must be 0)
    result[5] = static_cast<uint8_t>((stream_id >> 24) & 0x7F);
    result[6] = static_cast<uint8_t>((stream_id >> 16) & 0xFF);
    result[7] = static_cast<uint8_t>((stream_id >> 8) & 0xFF);
    result[8] = static_cast<uint8_t>(stream_id & 0xFF);
    
    return result;
}

expected<FrameHeader, Error> FrameHeader::parse(std::span<const uint8_t> data) {
    if (data.size() < Constants::FrameHeaderSize) {
        return unexpected(Error::io(IoError::InvalidArgument, "Frame header too short"));
    }
    
    FrameHeader header;
    
    // Length (24 bits, big-endian)
    header.length = (static_cast<uint32_t>(data[0]) << 16) |
                    (static_cast<uint32_t>(data[1]) << 8) |
                    static_cast<uint32_t>(data[2]);
    
    // Type (8 bits)
    header.type = static_cast<FrameType>(data[3]);
    
    // Flags (8 bits)
    header.flags = data[4];
    
    // Stream ID (31 bits, big-endian)
    header.stream_id = ((static_cast<uint32_t>(data[5]) & 0x7F) << 24) |
                       (static_cast<uint32_t>(data[6]) << 16) |
                       (static_cast<uint32_t>(data[7]) << 8) |
                       static_cast<uint32_t>(data[8]);
    
    return header;
}

// ============================================================================
// Frame Serialization Utilities
// ============================================================================

std::vector<uint8_t> serialize_settings_frame(
    std::span<const SettingsEntry> settings,
    bool ack
) {
    // Each setting is 6 bytes (2 byte ID + 4 byte value)
    size_t payload_size = ack ? 0 : settings.size() * 6;
    
    FrameHeader header;
    header.length = static_cast<uint32_t>(payload_size);
    header.type = FrameType::Settings;
    header.flags = ack ? FrameFlags::Ack : 0;
    header.stream_id = 0;  // SETTINGS must be on stream 0
    
    std::vector<uint8_t> result;
    result.reserve(Constants::FrameHeaderSize + payload_size);
    
    // Add header
    auto header_bytes = header.serialize();
    result.insert(result.end(), header_bytes.begin(), header_bytes.end());
    
    // Add settings (if not ACK)
    if (!ack) {
        for (const auto& setting : settings) {
            // ID (16 bits, big-endian)
            result.push_back(static_cast<uint8_t>(static_cast<uint16_t>(setting.id) >> 8));
            result.push_back(static_cast<uint8_t>(static_cast<uint16_t>(setting.id) & 0xFF));
            
            // Value (32 bits, big-endian)
            result.push_back(static_cast<uint8_t>(setting.value >> 24));
            result.push_back(static_cast<uint8_t>((setting.value >> 16) & 0xFF));
            result.push_back(static_cast<uint8_t>((setting.value >> 8) & 0xFF));
            result.push_back(static_cast<uint8_t>(setting.value & 0xFF));
        }
    }
    
    return result;
}

std::vector<uint8_t> serialize_settings_ack() {
    return serialize_settings_frame({}, true);
}

std::vector<uint8_t> serialize_ping_frame(
    std::span<const uint8_t, 8> opaque_data,
    bool ack
) {
    FrameHeader header;
    header.length = 8;  // PING payload is always 8 bytes
    header.type = FrameType::Ping;
    header.flags = ack ? FrameFlags::Ack : 0;
    header.stream_id = 0;  // PING must be on stream 0
    
    std::vector<uint8_t> result;
    result.reserve(Constants::FrameHeaderSize + 8);
    
    auto header_bytes = header.serialize();
    result.insert(result.end(), header_bytes.begin(), header_bytes.end());
    result.insert(result.end(), opaque_data.begin(), opaque_data.end());
    
    return result;
}

std::vector<uint8_t> serialize_goaway_frame(
    uint32_t last_stream_id,
    ErrorCode error_code,
    std::string_view debug_data
) {
    size_t payload_size = 8 + debug_data.size();  // 4 bytes stream ID + 4 bytes error + debug
    
    FrameHeader header;
    header.length = static_cast<uint32_t>(payload_size);
    header.type = FrameType::GoAway;
    header.flags = 0;
    header.stream_id = 0;  // GOAWAY must be on stream 0
    
    std::vector<uint8_t> result;
    result.reserve(Constants::FrameHeaderSize + payload_size);
    
    auto header_bytes = header.serialize();
    result.insert(result.end(), header_bytes.begin(), header_bytes.end());
    
    // Last stream ID (31 bits, big-endian)
    result.push_back(static_cast<uint8_t>((last_stream_id >> 24) & 0x7F));
    result.push_back(static_cast<uint8_t>((last_stream_id >> 16) & 0xFF));
    result.push_back(static_cast<uint8_t>((last_stream_id >> 8) & 0xFF));
    result.push_back(static_cast<uint8_t>(last_stream_id & 0xFF));
    
    // Error code (32 bits, big-endian)
    uint32_t error = static_cast<uint32_t>(error_code);
    result.push_back(static_cast<uint8_t>(error >> 24));
    result.push_back(static_cast<uint8_t>((error >> 16) & 0xFF));
    result.push_back(static_cast<uint8_t>((error >> 8) & 0xFF));
    result.push_back(static_cast<uint8_t>(error & 0xFF));
    
    // Debug data
    result.insert(result.end(), debug_data.begin(), debug_data.end());
    
    return result;
}

std::vector<uint8_t> serialize_window_update_frame(
    uint32_t stream_id,
    uint32_t window_size_increment
) {
    FrameHeader header;
    header.length = 4;  // WINDOW_UPDATE payload is always 4 bytes
    header.type = FrameType::WindowUpdate;
    header.flags = 0;
    header.stream_id = stream_id;
    
    std::vector<uint8_t> result;
    result.reserve(Constants::FrameHeaderSize + 4);
    
    auto header_bytes = header.serialize();
    result.insert(result.end(), header_bytes.begin(), header_bytes.end());
    
    // Window size increment (31 bits, big-endian)
    result.push_back(static_cast<uint8_t>((window_size_increment >> 24) & 0x7F));
    result.push_back(static_cast<uint8_t>((window_size_increment >> 16) & 0xFF));
    result.push_back(static_cast<uint8_t>((window_size_increment >> 8) & 0xFF));
    result.push_back(static_cast<uint8_t>(window_size_increment & 0xFF));
    
    return result;
}

std::vector<uint8_t> serialize_rst_stream_frame(
    uint32_t stream_id,
    ErrorCode error_code
) {
    FrameHeader header;
    header.length = 4;  // RST_STREAM payload is always 4 bytes
    header.type = FrameType::RstStream;
    header.flags = 0;
    header.stream_id = stream_id;
    
    std::vector<uint8_t> result;
    result.reserve(Constants::FrameHeaderSize + 4);
    
    auto header_bytes = header.serialize();
    result.insert(result.end(), header_bytes.begin(), header_bytes.end());
    
    // Error code (32 bits, big-endian)
    uint32_t error = static_cast<uint32_t>(error_code);
    result.push_back(static_cast<uint8_t>(error >> 24));
    result.push_back(static_cast<uint8_t>((error >> 16) & 0xFF));
    result.push_back(static_cast<uint8_t>((error >> 8) & 0xFF));
    result.push_back(static_cast<uint8_t>(error & 0xFF));
    
    return result;
}

std::vector<uint8_t> serialize_data_frame(
    uint32_t stream_id,
    std::span<const uint8_t> data,
    bool end_stream
) {
    FrameHeader header;
    header.length = static_cast<uint32_t>(data.size());
    header.type = FrameType::Data;
    header.flags = end_stream ? FrameFlags::EndStream : 0;
    header.stream_id = stream_id;
    
    std::vector<uint8_t> result;
    result.reserve(Constants::FrameHeaderSize + data.size());
    
    auto header_bytes = header.serialize();
    result.insert(result.end(), header_bytes.begin(), header_bytes.end());
    result.insert(result.end(), data.begin(), data.end());
    
    return result;
}

std::vector<uint8_t> serialize_headers_frame(
    uint32_t stream_id,
    std::span<const uint8_t> header_block,
    bool end_stream,
    bool end_headers
) {
    FrameHeader header;
    header.length = static_cast<uint32_t>(header_block.size());
    header.type = FrameType::Headers;
    header.flags = 0;
    if (end_stream) header.flags |= FrameFlags::EndStream;
    if (end_headers) header.flags |= FrameFlags::EndHeaders;
    header.stream_id = stream_id;
    
    std::vector<uint8_t> result;
    result.reserve(Constants::FrameHeaderSize + header_block.size());
    
    auto header_bytes = header.serialize();
    result.insert(result.end(), header_bytes.begin(), header_bytes.end());
    result.insert(result.end(), header_block.begin(), header_block.end());
    
    return result;
}

} // namespace coroute::http2
