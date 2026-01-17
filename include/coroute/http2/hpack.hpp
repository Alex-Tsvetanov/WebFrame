#pragma once

#include "coroute/util/expected.hpp"
#include "coroute/core/error.hpp"

#include <string>
#include <string_view>
#include <vector>
#include <span>
#include <memory>

// Forward declare nghttp2 types
typedef struct nghttp2_hd_deflater nghttp2_hd_deflater;
typedef struct nghttp2_hd_inflater nghttp2_hd_inflater;

namespace coroute::http2 {

// ============================================================================
// HTTP/2 Header
// ============================================================================

struct Header {
    std::string name;
    std::string value;
    
    // Pseudo-headers start with ':'
    bool is_pseudo() const { return !name.empty() && name[0] == ':'; }
};

// ============================================================================
// HPACK Encoder
// ============================================================================

class HpackEncoder {
    nghttp2_hd_deflater* deflater_ = nullptr;
    
public:
    HpackEncoder();
    ~HpackEncoder();
    
    // Non-copyable
    HpackEncoder(const HpackEncoder&) = delete;
    HpackEncoder& operator=(const HpackEncoder&) = delete;
    
    // Movable
    HpackEncoder(HpackEncoder&& other) noexcept;
    HpackEncoder& operator=(HpackEncoder&& other) noexcept;
    
    // Encode headers to HPACK format
    expected<std::vector<uint8_t>, Error> encode(std::span<const Header> headers);
    
    // Change dynamic table size
    void set_max_table_size(size_t size);
    
    // Get current dynamic table size
    size_t table_size() const;
};

// ============================================================================
// HPACK Decoder
// ============================================================================

class HpackDecoder {
    nghttp2_hd_inflater* inflater_ = nullptr;
    
public:
    HpackDecoder();
    ~HpackDecoder();
    
    // Non-copyable
    HpackDecoder(const HpackDecoder&) = delete;
    HpackDecoder& operator=(const HpackDecoder&) = delete;
    
    // Movable
    HpackDecoder(HpackDecoder&& other) noexcept;
    HpackDecoder& operator=(HpackDecoder&& other) noexcept;
    
    // Decode HPACK data to headers
    expected<std::vector<Header>, Error> decode(std::span<const uint8_t> data);
    
    // Change dynamic table size
    void set_max_table_size(size_t size);
    
    // Get current dynamic table size
    size_t table_size() const;
};

// ============================================================================
// Header Utilities
// ============================================================================

// Find a header by name (case-insensitive for regular headers)
const Header* find_header(std::span<const Header> headers, std::string_view name);

// Get pseudo-header value
std::string_view get_method(std::span<const Header> headers);
std::string_view get_scheme(std::span<const Header> headers);
std::string_view get_authority(std::span<const Header> headers);
std::string_view get_path(std::span<const Header> headers);
std::string_view get_status(std::span<const Header> headers);

// Validate headers according to HTTP/2 spec
bool validate_request_headers(std::span<const Header> headers);
bool validate_response_headers(std::span<const Header> headers);

} // namespace coroute::http2
