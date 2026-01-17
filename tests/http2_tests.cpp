#include <catch2/catch_test_macros.hpp>

#ifdef COROUTE_HAS_HTTP2

#include "coroute/http2/frame.hpp"
#include "coroute/http2/hpack.hpp"
#include "coroute/http2/stream.hpp"
#include "coroute/http2/connection.hpp"

using namespace coroute;
using namespace coroute::http2;

// ============================================================================
// Frame Header Tests
// ============================================================================

TEST_CASE("HTTP/2 Frame Header Serialization", "[http2][frame]") {
    SECTION("Basic frame header") {
        FrameHeader header;
        header.length = 100;
        header.type = FrameType::Data;
        header.flags = FrameFlags::EndStream;
        header.stream_id = 1;
        
        auto bytes = header.serialize();
        
        REQUIRE(bytes.size() == 9);
        // Length (24 bits big-endian)
        REQUIRE(bytes[0] == 0);
        REQUIRE(bytes[1] == 0);
        REQUIRE(bytes[2] == 100);
        // Type
        REQUIRE(bytes[3] == 0x00);  // DATA
        // Flags
        REQUIRE(bytes[4] == 0x01);  // END_STREAM
        // Stream ID (31 bits big-endian)
        REQUIRE(bytes[5] == 0);
        REQUIRE(bytes[6] == 0);
        REQUIRE(bytes[7] == 0);
        REQUIRE(bytes[8] == 1);
    }
    
    SECTION("Large payload length") {
        FrameHeader header;
        header.length = 16384;  // 16KB
        header.type = FrameType::Headers;
        header.flags = FrameFlags::EndHeaders | FrameFlags::EndStream;
        header.stream_id = 3;
        
        auto bytes = header.serialize();
        
        // Length = 0x004000
        REQUIRE(bytes[0] == 0x00);
        REQUIRE(bytes[1] == 0x40);
        REQUIRE(bytes[2] == 0x00);
        // Type = HEADERS
        REQUIRE(bytes[3] == 0x01);
        // Flags = END_HEADERS | END_STREAM
        REQUIRE(bytes[4] == 0x05);
    }
    
    SECTION("Maximum stream ID") {
        FrameHeader header;
        header.length = 0;
        header.type = FrameType::WindowUpdate;
        header.flags = 0;
        header.stream_id = 0x7FFFFFFF;  // Max 31-bit value
        
        auto bytes = header.serialize();
        
        REQUIRE(bytes[5] == 0x7F);
        REQUIRE(bytes[6] == 0xFF);
        REQUIRE(bytes[7] == 0xFF);
        REQUIRE(bytes[8] == 0xFF);
    }
}

TEST_CASE("HTTP/2 Frame Header Parsing", "[http2][frame]") {
    SECTION("Parse valid header") {
        std::array<uint8_t, 9> data = {
            0x00, 0x00, 0x08,  // Length = 8
            0x06,              // Type = PING
            0x01,              // Flags = ACK
            0x00, 0x00, 0x00, 0x00  // Stream ID = 0
        };
        
        auto result = FrameHeader::parse(data);
        REQUIRE(result.has_value());
        
        REQUIRE(result->length == 8);
        REQUIRE(result->type == FrameType::Ping);
        REQUIRE(result->flags == FrameFlags::Ack);
        REQUIRE(result->stream_id == 0);
    }
    
    SECTION("Parse header with stream ID") {
        std::array<uint8_t, 9> data = {
            0x00, 0x01, 0x00,  // Length = 256
            0x00,              // Type = DATA
            0x00,              // Flags = none
            0x00, 0x00, 0x00, 0x05  // Stream ID = 5
        };
        
        auto result = FrameHeader::parse(data);
        REQUIRE(result.has_value());
        
        REQUIRE(result->length == 256);
        REQUIRE(result->type == FrameType::Data);
        REQUIRE(result->stream_id == 5);
    }
    
    SECTION("Reject too short data") {
        std::array<uint8_t, 5> data = {0, 0, 0, 0, 0};
        
        auto result = FrameHeader::parse(data);
        REQUIRE_FALSE(result.has_value());
    }
    
    SECTION("Roundtrip") {
        FrameHeader original;
        original.length = 12345;
        original.type = FrameType::Headers;
        original.flags = FrameFlags::EndHeaders | FrameFlags::Padded;
        original.stream_id = 99;
        
        auto bytes = original.serialize();
        auto parsed = FrameHeader::parse(bytes);
        
        REQUIRE(parsed.has_value());
        REQUIRE(parsed->length == original.length);
        REQUIRE(parsed->type == original.type);
        REQUIRE(parsed->flags == original.flags);
        REQUIRE(parsed->stream_id == original.stream_id);
    }
}

// ============================================================================
// Frame Serialization Tests
// ============================================================================

TEST_CASE("HTTP/2 SETTINGS Frame", "[http2][frame]") {
    SECTION("Empty SETTINGS (ACK)") {
        auto frame = serialize_settings_ack();
        
        REQUIRE(frame.size() == 9);  // Header only
        
        auto header = FrameHeader::parse(frame);
        REQUIRE(header.has_value());
        REQUIRE(header->length == 0);
        REQUIRE(header->type == FrameType::Settings);
        REQUIRE(header->has_ack());
        REQUIRE(header->stream_id == 0);
    }
    
    SECTION("SETTINGS with entries") {
        std::vector<SettingsEntry> settings = {
            {SettingsId::MaxConcurrentStreams, 100},
            {SettingsId::InitialWindowSize, 65535},
        };
        
        auto frame = serialize_settings_frame(settings, false);
        
        // 9 byte header + 2 * 6 byte settings
        REQUIRE(frame.size() == 9 + 12);
        
        auto header = FrameHeader::parse(frame);
        REQUIRE(header.has_value());
        REQUIRE(header->length == 12);
        REQUIRE(header->type == FrameType::Settings);
        REQUIRE_FALSE(header->has_ack());
    }
}

TEST_CASE("HTTP/2 PING Frame", "[http2][frame]") {
    SECTION("PING request") {
        std::array<uint8_t, 8> opaque = {1, 2, 3, 4, 5, 6, 7, 8};
        auto frame = serialize_ping_frame(opaque, false);
        
        REQUIRE(frame.size() == 9 + 8);
        
        auto header = FrameHeader::parse(frame);
        REQUIRE(header.has_value());
        REQUIRE(header->length == 8);
        REQUIRE(header->type == FrameType::Ping);
        REQUIRE_FALSE(header->has_ack());
        
        // Check opaque data
        for (int i = 0; i < 8; ++i) {
            REQUIRE(frame[9 + i] == opaque[i]);
        }
    }
    
    SECTION("PING ACK") {
        std::array<uint8_t, 8> opaque = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
        auto frame = serialize_ping_frame(opaque, true);
        
        auto header = FrameHeader::parse(frame);
        REQUIRE(header.has_value());
        REQUIRE(header->has_ack());
    }
}

TEST_CASE("HTTP/2 GOAWAY Frame", "[http2][frame]") {
    SECTION("GOAWAY without debug data") {
        auto frame = serialize_goaway_frame(5, ErrorCode::NoError);
        
        REQUIRE(frame.size() == 9 + 8);
        
        auto header = FrameHeader::parse(frame);
        REQUIRE(header.has_value());
        REQUIRE(header->length == 8);
        REQUIRE(header->type == FrameType::GoAway);
        REQUIRE(header->stream_id == 0);
    }
    
    SECTION("GOAWAY with debug data") {
        auto frame = serialize_goaway_frame(100, ErrorCode::ProtocolError, "test error");
        
        REQUIRE(frame.size() == 9 + 8 + 10);  // header + fixed + "test error"
        
        auto header = FrameHeader::parse(frame);
        REQUIRE(header.has_value());
        REQUIRE(header->length == 18);
    }
}

TEST_CASE("HTTP/2 WINDOW_UPDATE Frame", "[http2][frame]") {
    SECTION("Connection-level update") {
        auto frame = serialize_window_update_frame(0, 65535);
        
        REQUIRE(frame.size() == 9 + 4);
        
        auto header = FrameHeader::parse(frame);
        REQUIRE(header.has_value());
        REQUIRE(header->length == 4);
        REQUIRE(header->type == FrameType::WindowUpdate);
        REQUIRE(header->stream_id == 0);
    }
    
    SECTION("Stream-level update") {
        auto frame = serialize_window_update_frame(7, 32768);
        
        auto header = FrameHeader::parse(frame);
        REQUIRE(header.has_value());
        REQUIRE(header->stream_id == 7);
    }
}

TEST_CASE("HTTP/2 RST_STREAM Frame", "[http2][frame]") {
    auto frame = serialize_rst_stream_frame(3, ErrorCode::Cancel);
    
    REQUIRE(frame.size() == 9 + 4);
    
    auto header = FrameHeader::parse(frame);
    REQUIRE(header.has_value());
    REQUIRE(header->length == 4);
    REQUIRE(header->type == FrameType::RstStream);
    REQUIRE(header->stream_id == 3);
}

TEST_CASE("HTTP/2 DATA Frame", "[http2][frame]") {
    SECTION("Empty DATA with END_STREAM") {
        auto frame = serialize_data_frame(1, {}, true);
        
        REQUIRE(frame.size() == 9);
        
        auto header = FrameHeader::parse(frame);
        REQUIRE(header.has_value());
        REQUIRE(header->length == 0);
        REQUIRE(header->type == FrameType::Data);
        REQUIRE(header->has_end_stream());
        REQUIRE(header->stream_id == 1);
    }
    
    SECTION("DATA with payload") {
        std::vector<uint8_t> payload = {0x48, 0x65, 0x6C, 0x6C, 0x6F};  // "Hello"
        auto frame = serialize_data_frame(5, payload, false);
        
        REQUIRE(frame.size() == 9 + 5);
        
        auto header = FrameHeader::parse(frame);
        REQUIRE(header.has_value());
        REQUIRE(header->length == 5);
        REQUIRE_FALSE(header->has_end_stream());
    }
}

TEST_CASE("HTTP/2 HEADERS Frame", "[http2][frame]") {
    SECTION("HEADERS with END_HEADERS and END_STREAM") {
        std::vector<uint8_t> header_block = {0x82, 0x86, 0x84};  // Some HPACK data
        auto frame = serialize_headers_frame(1, header_block, true, true);
        
        REQUIRE(frame.size() == 9 + 3);
        
        auto header = FrameHeader::parse(frame);
        REQUIRE(header.has_value());
        REQUIRE(header->length == 3);
        REQUIRE(header->type == FrameType::Headers);
        REQUIRE(header->has_end_stream());
        REQUIRE(header->has_end_headers());
        REQUIRE(header->stream_id == 1);
    }
}

// ============================================================================
// HPACK Tests
// ============================================================================

TEST_CASE("HPACK Encoder/Decoder", "[http2][hpack]") {
    HpackEncoder encoder;
    HpackDecoder decoder;
    
    SECTION("Encode and decode simple headers") {
        std::vector<Header> headers = {
            {":method", "GET"},
            {":path", "/"},
            {":scheme", "https"},
            {":authority", "example.com"},
            {"user-agent", "test/1.0"},
        };
        
        auto encoded = encoder.encode(headers);
        REQUIRE(encoded.has_value());
        REQUIRE(encoded->size() > 0);
        
        auto decoded = decoder.decode(*encoded);
        REQUIRE(decoded.has_value());
        REQUIRE(decoded->size() == headers.size());
        
        for (size_t i = 0; i < headers.size(); ++i) {
            REQUIRE((*decoded)[i].name == headers[i].name);
            REQUIRE((*decoded)[i].value == headers[i].value);
        }
    }
    
    SECTION("Encode response headers") {
        std::vector<Header> headers = {
            {":status", "200"},
            {"content-type", "text/html"},
            {"content-length", "1234"},
        };
        
        auto encoded = encoder.encode(headers);
        REQUIRE(encoded.has_value());
        
        auto decoded = decoder.decode(*encoded);
        REQUIRE(decoded.has_value());
        REQUIRE(decoded->size() == 3);
        REQUIRE((*decoded)[0].name == ":status");
        REQUIRE((*decoded)[0].value == "200");
    }
    
    SECTION("Empty headers") {
        std::vector<Header> headers;
        
        auto encoded = encoder.encode(headers);
        REQUIRE(encoded.has_value());
        REQUIRE(encoded->size() == 0);
    }
}

TEST_CASE("HPACK Header Utilities", "[http2][hpack]") {
    std::vector<Header> headers = {
        {":method", "POST"},
        {":path", "/api/data"},
        {":scheme", "https"},
        {":authority", "api.example.com"},
        {"content-type", "application/json"},
        {"x-custom-header", "value"},
    };
    
    SECTION("Get pseudo-headers") {
        REQUIRE(get_method(headers) == "POST");
        REQUIRE(get_path(headers) == "/api/data");
        REQUIRE(get_scheme(headers) == "https");
        REQUIRE(get_authority(headers) == "api.example.com");
    }
    
    SECTION("Find header case-insensitive") {
        auto* ct = find_header(headers, "Content-Type");
        REQUIRE(ct != nullptr);
        REQUIRE(ct->value == "application/json");
        
        auto* custom = find_header(headers, "X-Custom-Header");
        REQUIRE(custom != nullptr);
        REQUIRE(custom->value == "value");
    }
    
    SECTION("Find non-existent header") {
        auto* missing = find_header(headers, "X-Missing");
        REQUIRE(missing == nullptr);
    }
    
    SECTION("Pseudo-header detection") {
        REQUIRE(headers[0].is_pseudo());  // :method
        REQUIRE(headers[1].is_pseudo());  // :path
        REQUIRE_FALSE(headers[4].is_pseudo());  // content-type
    }
}

TEST_CASE("HTTP/2 Header Validation", "[http2][hpack]") {
    SECTION("Valid request headers") {
        std::vector<Header> headers = {
            {":method", "GET"},
            {":scheme", "https"},
            {":path", "/"},
            {"host", "example.com"},
        };
        
        REQUIRE(validate_request_headers(headers));
    }
    
    SECTION("Missing required pseudo-header") {
        std::vector<Header> headers = {
            {":method", "GET"},
            {":scheme", "https"},
            // Missing :path
        };
        
        REQUIRE_FALSE(validate_request_headers(headers));
    }
    
    SECTION("Pseudo-headers after regular headers") {
        std::vector<Header> headers = {
            {":method", "GET"},
            {"host", "example.com"},
            {":path", "/"},  // Pseudo-header after regular
            {":scheme", "https"},
        };
        
        REQUIRE_FALSE(validate_request_headers(headers));
    }
    
    SECTION("Valid response headers") {
        std::vector<Header> headers = {
            {":status", "200"},
            {"content-type", "text/html"},
        };
        
        REQUIRE(validate_response_headers(headers));
    }
    
    SECTION("Response missing :status") {
        std::vector<Header> headers = {
            {"content-type", "text/html"},
        };
        
        REQUIRE_FALSE(validate_response_headers(headers));
    }
}

// ============================================================================
// Protocol Detection Tests
// ============================================================================

TEST_CASE("HTTP/2 Protocol Detection", "[http2][connection]") {
    SECTION("Detect HTTP/2 preface") {
        std::vector<uint8_t> preface(Constants::ClientPreface.begin(), Constants::ClientPreface.end());
        
        REQUIRE(is_http2_preface(preface));
    }
    
    SECTION("Reject HTTP/1.1 request") {
        std::string http11 = "GET / HTTP/1.1\r\n";
        std::vector<uint8_t> data(http11.begin(), http11.end());
        
        REQUIRE_FALSE(is_http2_preface(data));
    }
    
    SECTION("Reject partial preface") {
        std::string partial = "PRI * HTTP/2.0";
        std::vector<uint8_t> data(partial.begin(), partial.end());
        
        REQUIRE_FALSE(is_http2_preface(data));
    }
}

// ============================================================================
// Constants Tests
// ============================================================================

TEST_CASE("HTTP/2 Constants", "[http2][frame]") {
    REQUIRE(Constants::FrameHeaderSize == 9);
    REQUIRE(Constants::DefaultInitialWindowSize == 65535);
    REQUIRE(Constants::DefaultMaxFrameSize == 16384);
    REQUIRE(Constants::MinMaxFrameSize == 16384);
    REQUIRE(Constants::MaxMaxFrameSize == 16777215);
    REQUIRE(Constants::MaxWindowSize == 2147483647);
    REQUIRE(Constants::ClientPreface == "PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n");
}

#endif // coroute_HAS_HTTP2
