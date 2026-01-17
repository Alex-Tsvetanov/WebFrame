#pragma once

#include "coroute/util/expected.hpp"
#include "coroute/core/error.hpp"

#include <cstdint>
#include <vector>
#include <span>
#include <string_view>
#include <array>

namespace coroute::http2 {

// ============================================================================
// HTTP/2 Frame Types (RFC 7540 Section 6)
// ============================================================================

enum class FrameType : uint8_t {
    Data          = 0x0,
    Headers       = 0x1,
    Priority      = 0x2,  // Deprecated, but must handle
    RstStream     = 0x3,
    Settings      = 0x4,
    PushPromise   = 0x5,  // Not implementing server push
    Ping          = 0x6,
    GoAway        = 0x7,
    WindowUpdate  = 0x8,
    Continuation  = 0x9,
};

// ============================================================================
// HTTP/2 Frame Flags
// ============================================================================

namespace FrameFlags {
    // DATA frame flags
    constexpr uint8_t EndStream  = 0x01;
    constexpr uint8_t Padded     = 0x08;
    
    // HEADERS frame flags
    constexpr uint8_t EndHeaders = 0x04;
    constexpr uint8_t Priority   = 0x20;
    // Also uses EndStream and Padded
    
    // SETTINGS frame flags
    constexpr uint8_t Ack        = 0x01;
    
    // PING frame flags
    // Uses Ack
    
    // CONTINUATION frame flags
    // Uses EndHeaders
}

// ============================================================================
// HTTP/2 Error Codes (RFC 7540 Section 7)
// ============================================================================

enum class ErrorCode : uint32_t {
    NoError            = 0x0,
    ProtocolError      = 0x1,
    InternalError      = 0x2,
    FlowControlError   = 0x3,
    SettingsTimeout    = 0x4,
    StreamClosed       = 0x5,
    FrameSizeError     = 0x6,
    RefusedStream      = 0x7,
    Cancel             = 0x8,
    CompressionError   = 0x9,
    ConnectError       = 0xa,
    EnhanceYourCalm    = 0xb,
    InadequateSecurity = 0xc,
    Http11Required     = 0xd,
};

// ============================================================================
// HTTP/2 Settings Parameters (RFC 7540 Section 6.5.2)
// ============================================================================

enum class SettingsId : uint16_t {
    HeaderTableSize      = 0x1,
    EnablePush           = 0x2,
    MaxConcurrentStreams = 0x3,
    InitialWindowSize    = 0x4,
    MaxFrameSize         = 0x5,
    MaxHeaderListSize    = 0x6,
};

// ============================================================================
// HTTP/2 Frame Header (9 bytes)
// ============================================================================

struct FrameHeader {
    uint32_t length;      // 24 bits - payload length
    FrameType type;       // 8 bits
    uint8_t flags;        // 8 bits
    uint32_t stream_id;   // 31 bits (high bit reserved)
    
    // Serialize to wire format (9 bytes)
    std::array<uint8_t, 9> serialize() const;
    
    // Parse from wire format
    static expected<FrameHeader, Error> parse(std::span<const uint8_t> data);
    
    // Check flags
    bool has_end_stream() const { return flags & FrameFlags::EndStream; }
    bool has_end_headers() const { return flags & FrameFlags::EndHeaders; }
    bool has_padded() const { return flags & FrameFlags::Padded; }
    bool has_priority() const { return flags & FrameFlags::Priority; }
    bool has_ack() const { return flags & FrameFlags::Ack; }
};

// ============================================================================
// Settings Entry
// ============================================================================

struct SettingsEntry {
    SettingsId id;
    uint32_t value;
};

// ============================================================================
// Frame Constants
// ============================================================================

namespace Constants {
    // Frame header size
    constexpr size_t FrameHeaderSize = 9;
    
    // Default settings values
    constexpr uint32_t DefaultHeaderTableSize = 4096;
    constexpr uint32_t DefaultEnablePush = 1;
    constexpr uint32_t DefaultMaxConcurrentStreams = 100;  // Unlimited by spec, but we limit
    constexpr uint32_t DefaultInitialWindowSize = 65535;   // 64KB - 1
    constexpr uint32_t DefaultMaxFrameSize = 16384;        // 16KB
    constexpr uint32_t DefaultMaxHeaderListSize = 8192;    // 8KB
    
    // Limits
    constexpr uint32_t MinMaxFrameSize = 16384;            // 16KB minimum
    constexpr uint32_t MaxMaxFrameSize = 16777215;         // 16MB - 1 (24-bit max)
    constexpr uint32_t MaxWindowSize = 2147483647;         // 2GB - 1 (31-bit max)
    
    // Connection preface
    constexpr std::string_view ClientPreface = "PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n";
}

// ============================================================================
// Frame Parsing/Serialization Utilities
// ============================================================================

// Serialize a SETTINGS frame
std::vector<uint8_t> serialize_settings_frame(
    std::span<const SettingsEntry> settings,
    bool ack = false
);

// Serialize a SETTINGS ACK frame
std::vector<uint8_t> serialize_settings_ack();

// Serialize a PING frame
std::vector<uint8_t> serialize_ping_frame(
    std::span<const uint8_t, 8> opaque_data,
    bool ack = false
);

// Serialize a GOAWAY frame
std::vector<uint8_t> serialize_goaway_frame(
    uint32_t last_stream_id,
    ErrorCode error_code,
    std::string_view debug_data = {}
);

// Serialize a WINDOW_UPDATE frame
std::vector<uint8_t> serialize_window_update_frame(
    uint32_t stream_id,
    uint32_t window_size_increment
);

// Serialize a RST_STREAM frame
std::vector<uint8_t> serialize_rst_stream_frame(
    uint32_t stream_id,
    ErrorCode error_code
);

// Serialize a DATA frame
std::vector<uint8_t> serialize_data_frame(
    uint32_t stream_id,
    std::span<const uint8_t> data,
    bool end_stream = false
);

// Serialize a HEADERS frame (without HPACK - that's done separately)
std::vector<uint8_t> serialize_headers_frame(
    uint32_t stream_id,
    std::span<const uint8_t> header_block,
    bool end_stream = false,
    bool end_headers = true
);

} // namespace coroute::http2
