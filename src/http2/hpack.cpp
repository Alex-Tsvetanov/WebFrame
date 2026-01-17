#include "coroute/http2/hpack.hpp"
#include "coroute/http2/frame.hpp"

// nghttp2.h requires standard integer types and ssize_t to be defined
#include <cstdint>
#ifdef _WIN32
#include <BaseTsd.h>
typedef SSIZE_T ssize_t;
#endif
#include <nghttp2/nghttp2.h>
#include <algorithm>
#include <cctype>

namespace coroute::http2 {

// ============================================================================
// HPACK Encoder
// ============================================================================

HpackEncoder::HpackEncoder() {
    nghttp2_hd_deflate_new(&deflater_, Constants::DefaultHeaderTableSize);
}

HpackEncoder::~HpackEncoder() {
    if (deflater_) {
        nghttp2_hd_deflate_del(deflater_);
    }
}

HpackEncoder::HpackEncoder(HpackEncoder&& other) noexcept
    : deflater_(other.deflater_)
{
    other.deflater_ = nullptr;
}

HpackEncoder& HpackEncoder::operator=(HpackEncoder&& other) noexcept {
    if (this != &other) {
        if (deflater_) {
            nghttp2_hd_deflate_del(deflater_);
        }
        deflater_ = other.deflater_;
        other.deflater_ = nullptr;
    }
    return *this;
}

expected<std::vector<uint8_t>, Error> HpackEncoder::encode(std::span<const Header> headers) {
    if (!deflater_) {
        return unexpected(Error::io(IoError::Unknown, "HPACK encoder not initialized"));
    }
    
    // Convert to nghttp2 format
    std::vector<nghttp2_nv> nvs;
    nvs.reserve(headers.size());
    
    for (const auto& h : headers) {
        nghttp2_nv nv;
        nv.name = reinterpret_cast<uint8_t*>(const_cast<char*>(h.name.data()));
        nv.namelen = h.name.size();
        nv.value = reinterpret_cast<uint8_t*>(const_cast<char*>(h.value.data()));
        nv.valuelen = h.value.size();
        nv.flags = NGHTTP2_NV_FLAG_NONE;
        nvs.push_back(nv);
    }
    
    // Calculate upper bound for encoded size
    size_t buflen = nghttp2_hd_deflate_bound(deflater_, nvs.data(), nvs.size());
    
    std::vector<uint8_t> result(buflen);
    
    // Encode
    ssize_t rv = nghttp2_hd_deflate_hd(
        deflater_,
        result.data(),
        result.size(),
        nvs.data(),
        nvs.size()
    );
    
    if (rv < 0) {
        return unexpected(Error::io(IoError::Unknown, 
            std::string("HPACK encode error: ") + nghttp2_strerror(static_cast<int>(rv))));
    }
    
    result.resize(static_cast<size_t>(rv));
    return result;
}

void HpackEncoder::set_max_table_size(size_t size) {
    if (deflater_) {
        nghttp2_hd_deflate_change_table_size(deflater_, size);
    }
}

size_t HpackEncoder::table_size() const {
    if (deflater_) {
        return nghttp2_hd_deflate_get_dynamic_table_size(deflater_);
    }
    return 0;
}

// ============================================================================
// HPACK Decoder
// ============================================================================

HpackDecoder::HpackDecoder() {
    nghttp2_hd_inflate_new(&inflater_);
}

HpackDecoder::~HpackDecoder() {
    if (inflater_) {
        nghttp2_hd_inflate_del(inflater_);
    }
}

HpackDecoder::HpackDecoder(HpackDecoder&& other) noexcept
    : inflater_(other.inflater_)
{
    other.inflater_ = nullptr;
}

HpackDecoder& HpackDecoder::operator=(HpackDecoder&& other) noexcept {
    if (this != &other) {
        if (inflater_) {
            nghttp2_hd_inflate_del(inflater_);
        }
        inflater_ = other.inflater_;
        other.inflater_ = nullptr;
    }
    return *this;
}

expected<std::vector<Header>, Error> HpackDecoder::decode(std::span<const uint8_t> data) {
    if (!inflater_) {
        return unexpected(Error::io(IoError::Unknown, "HPACK decoder not initialized"));
    }
    
    std::vector<Header> result;
    
    const uint8_t* in = data.data();
    size_t inlen = data.size();
    
    while (inlen > 0) {
        nghttp2_nv nv;
        int inflate_flags = 0;
        
        ssize_t rv = nghttp2_hd_inflate_hd2(
            inflater_,
            &nv,
            &inflate_flags,
            in,
            inlen,
            1  // in_final - we're processing complete header block
        );
        
        if (rv < 0) {
            return unexpected(Error::io(IoError::Unknown,
                std::string("HPACK decode error: ") + nghttp2_strerror(static_cast<int>(rv))));
        }
        
        in += rv;
        inlen -= static_cast<size_t>(rv);
        
        if (inflate_flags & NGHTTP2_HD_INFLATE_EMIT) {
            result.push_back({
                std::string(reinterpret_cast<const char*>(nv.name), nv.namelen),
                std::string(reinterpret_cast<const char*>(nv.value), nv.valuelen)
            });
        }
        
        if (inflate_flags & NGHTTP2_HD_INFLATE_FINAL) {
            break;
        }
    }
    
    // End header block
    nghttp2_hd_inflate_end_headers(inflater_);
    
    return result;
}

void HpackDecoder::set_max_table_size(size_t size) {
    if (inflater_) {
        nghttp2_hd_inflate_change_table_size(inflater_, size);
    }
}

size_t HpackDecoder::table_size() const {
    if (inflater_) {
        return nghttp2_hd_inflate_get_dynamic_table_size(inflater_);
    }
    return 0;
}

// ============================================================================
// Header Utilities
// ============================================================================

const Header* find_header(std::span<const Header> headers, std::string_view name) {
    for (const auto& h : headers) {
        // Pseudo-headers are case-sensitive, regular headers are case-insensitive
        if (name[0] == ':') {
            if (h.name == name) return &h;
        } else {
            if (h.name.size() == name.size()) {
                bool match = true;
                for (size_t i = 0; i < name.size(); ++i) {
                    if (std::tolower(static_cast<unsigned char>(h.name[i])) != 
                        std::tolower(static_cast<unsigned char>(name[i]))) {
                        match = false;
                        break;
                    }
                }
                if (match) return &h;
            }
        }
    }
    return nullptr;
}

std::string_view get_method(std::span<const Header> headers) {
    auto* h = find_header(headers, ":method");
    return h ? std::string_view(h->value) : std::string_view{};
}

std::string_view get_scheme(std::span<const Header> headers) {
    auto* h = find_header(headers, ":scheme");
    return h ? std::string_view(h->value) : std::string_view{};
}

std::string_view get_authority(std::span<const Header> headers) {
    auto* h = find_header(headers, ":authority");
    return h ? std::string_view(h->value) : std::string_view{};
}

std::string_view get_path(std::span<const Header> headers) {
    auto* h = find_header(headers, ":path");
    return h ? std::string_view(h->value) : std::string_view{};
}

std::string_view get_status(std::span<const Header> headers) {
    auto* h = find_header(headers, ":status");
    return h ? std::string_view(h->value) : std::string_view{};
}

bool validate_request_headers(std::span<const Header> headers) {
    // Must have :method, :scheme, :path
    // :authority is optional but recommended
    bool has_method = false;
    bool has_scheme = false;
    bool has_path = false;
    bool pseudo_ended = false;
    
    for (const auto& h : headers) {
        if (h.is_pseudo()) {
            if (pseudo_ended) {
                // Pseudo-headers must come before regular headers
                return false;
            }
            if (h.name == ":method") has_method = true;
            else if (h.name == ":scheme") has_scheme = true;
            else if (h.name == ":path") has_path = true;
            else if (h.name == ":authority") { /* optional */ }
            else if (h.name == ":status") {
                // :status is for responses only
                return false;
            }
        } else {
            pseudo_ended = true;
            // Header names must be lowercase
            for (char c : h.name) {
                if (std::isupper(static_cast<unsigned char>(c))) {
                    return false;
                }
            }
        }
    }
    
    return has_method && has_scheme && has_path;
}

bool validate_response_headers(std::span<const Header> headers) {
    // Must have :status
    bool has_status = false;
    bool pseudo_ended = false;
    
    for (const auto& h : headers) {
        if (h.is_pseudo()) {
            if (pseudo_ended) {
                return false;
            }
            if (h.name == ":status") has_status = true;
            else {
                // Only :status is allowed in responses
                return false;
            }
        } else {
            pseudo_ended = true;
            for (char c : h.name) {
                if (std::isupper(static_cast<unsigned char>(c))) {
                    return false;
                }
            }
        }
    }
    
    return has_status;
}

} // namespace coroute::http2
