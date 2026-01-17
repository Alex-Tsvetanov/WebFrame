#include "coroute/core/compression.hpp"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <sstream>

// zlib for gzip/deflate
#include <zlib.h>

// Brotli (optional)
#ifdef COROUTE_HAS_BROTLI
#include <brotli/encode.h>
#include <brotli/decode.h>
#endif

namespace coroute {

// ============================================================================
// Algorithm Utilities
// ============================================================================

std::string_view algorithm_name(CompressionAlgorithm algo) noexcept {
    switch (algo) {
        case CompressionAlgorithm::Gzip: return "gzip";
        case CompressionAlgorithm::Deflate: return "deflate";
        case CompressionAlgorithm::Brotli: return "br";
        case CompressionAlgorithm::Identity: return "identity";
        default: return "identity";
    }
}

namespace {

// Helper to trim whitespace
std::string_view trim(std::string_view s) {
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.front()))) {
        s.remove_prefix(1);
    }
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back()))) {
        s.remove_suffix(1);
    }
    return s;
}

// Helper to lowercase
std::string to_lower(std::string_view s) {
    std::string result(s);
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return result;
}

// Parse quality value (e.g., "gzip;q=0.8" -> 0.8)
float parse_quality(std::string_view encoding) {
    auto semicolon = encoding.find(';');
    if (semicolon == std::string_view::npos) {
        return 1.0f;  // Default quality
    }
    
    auto params = encoding.substr(semicolon + 1);
    auto q_pos = params.find("q=");
    if (q_pos == std::string_view::npos) {
        return 1.0f;
    }
    
    auto q_value = params.substr(q_pos + 2);
    auto end = q_value.find_first_of(",; ");
    if (end != std::string_view::npos) {
        q_value = q_value.substr(0, end);
    }
    
    try {
        return std::stof(std::string(q_value));
    } catch (...) {
        return 1.0f;
    }
}

// Get encoding name without quality
std::string_view get_encoding_name(std::string_view encoding) {
    auto semicolon = encoding.find(';');
    if (semicolon != std::string_view::npos) {
        encoding = encoding.substr(0, semicolon);
    }
    return trim(encoding);
}

} // anonymous namespace

CompressionAlgorithm parse_accept_encoding(std::string_view header,
                                            const std::set<CompressionAlgorithm>& enabled) {
    if (header.empty() || enabled.empty()) {
        return CompressionAlgorithm::Identity;
    }
    
    // Parse encodings with quality values
    struct EncodingQuality {
        CompressionAlgorithm algo;
        float quality;
    };
    std::vector<EncodingQuality> candidates;
    
    // Split by comma
    size_t start = 0;
    while (start < header.size()) {
        auto comma = header.find(',', start);
        auto part = (comma == std::string_view::npos) 
            ? header.substr(start) 
            : header.substr(start, comma - start);
        
        auto name = to_lower(get_encoding_name(part));
        float quality = parse_quality(part);
        
        // Skip if quality is 0
        if (quality > 0) {
            CompressionAlgorithm algo = CompressionAlgorithm::Identity;
            
            if (name == "gzip" || name == "x-gzip") {
                algo = CompressionAlgorithm::Gzip;
            } else if (name == "deflate") {
                algo = CompressionAlgorithm::Deflate;
            } else if (name == "br") {
                algo = CompressionAlgorithm::Brotli;
            }
            
            // Only add if enabled
            if (algo != CompressionAlgorithm::Identity && enabled.count(algo)) {
                // Check if Brotli is actually available
                if (algo == CompressionAlgorithm::Brotli && !compress::brotli_available()) {
                    // Skip Brotli if not compiled in
                } else {
                    candidates.push_back({algo, quality});
                }
            }
        }
        
        if (comma == std::string_view::npos) break;
        start = comma + 1;
    }
    
    if (candidates.empty()) {
        return CompressionAlgorithm::Identity;
    }
    
    // Sort by quality (descending), then by our preference order
    std::sort(candidates.begin(), candidates.end(), 
              [](const EncodingQuality& a, const EncodingQuality& b) {
                  if (a.quality != b.quality) {
                      return a.quality > b.quality;
                  }
                  // Prefer Brotli > Gzip > Deflate
                  return static_cast<int>(a.algo) < static_cast<int>(b.algo);
              });
    
    return candidates.front().algo;
}

// ============================================================================
// Compression Functions
// ============================================================================

namespace compress {

bool brotli_available() noexcept {
#ifdef COROUTE_HAS_BROTLI
    return true;
#else
    return false;
#endif
}

std::optional<std::string> gzip(std::string_view data, int level) {
    if (data.empty()) {
        return std::string{};
    }
    
    // Clamp level
    level = std::clamp(level, 1, 9);
    
    z_stream stream{};
    stream.zalloc = Z_NULL;
    stream.zfree = Z_NULL;
    stream.opaque = Z_NULL;
    
    // windowBits = 15 + 16 for gzip format
    if (deflateInit2(&stream, level, Z_DEFLATED, 15 + 16, 8, Z_DEFAULT_STRATEGY) != Z_OK) {
        return std::nullopt;
    }
    
    stream.next_in = reinterpret_cast<Bytef*>(const_cast<char*>(data.data()));
    stream.avail_in = static_cast<uInt>(data.size());
    
    std::string result;
    result.resize(deflateBound(&stream, static_cast<uLong>(data.size())));
    
    stream.next_out = reinterpret_cast<Bytef*>(result.data());
    stream.avail_out = static_cast<uInt>(result.size());
    
    int ret = deflate(&stream, Z_FINISH);
    deflateEnd(&stream);
    
    if (ret != Z_STREAM_END) {
        return std::nullopt;
    }
    
    result.resize(stream.total_out);
    return result;
}

std::optional<std::string> deflate(std::string_view data, int level) {
    if (data.empty()) {
        return std::string{};
    }
    
    level = std::clamp(level, 1, 9);
    
    z_stream stream{};
    stream.zalloc = Z_NULL;
    stream.zfree = Z_NULL;
    stream.opaque = Z_NULL;
    
    // windowBits = -15 for raw deflate (no header)
    if (deflateInit2(&stream, level, Z_DEFLATED, -15, 8, Z_DEFAULT_STRATEGY) != Z_OK) {
        return std::nullopt;
    }
    
    stream.next_in = reinterpret_cast<Bytef*>(const_cast<char*>(data.data()));
    stream.avail_in = static_cast<uInt>(data.size());
    
    std::string result;
    result.resize(deflateBound(&stream, static_cast<uLong>(data.size())));
    
    stream.next_out = reinterpret_cast<Bytef*>(result.data());
    stream.avail_out = static_cast<uInt>(result.size());
    
    int ret = ::deflate(&stream, Z_FINISH);
    deflateEnd(&stream);
    
    if (ret != Z_STREAM_END) {
        return std::nullopt;
    }
    
    result.resize(stream.total_out);
    return result;
}

std::optional<std::string> brotli(std::string_view data, int level) {
#ifdef COROUTE_HAS_BROTLI
    if (data.empty()) {
        return std::string{};
    }
    
    // Brotli quality: 0-11
    level = std::clamp(level, 0, 11);
    
    size_t output_size = BrotliEncoderMaxCompressedSize(data.size());
    if (output_size == 0) {
        return std::nullopt;
    }
    
    std::string result(output_size, '\0');
    
    if (BrotliEncoderCompress(
            level,
            BROTLI_DEFAULT_WINDOW,
            BROTLI_DEFAULT_MODE,
            data.size(),
            reinterpret_cast<const uint8_t*>(data.data()),
            &output_size,
            reinterpret_cast<uint8_t*>(result.data())) != BROTLI_TRUE) {
        return std::nullopt;
    }
    
    result.resize(output_size);
    return result;
#else
    (void)data;
    (void)level;
    return std::nullopt;
#endif
}

std::optional<std::string> compress(std::string_view data, 
                                     CompressionAlgorithm algo,
                                     int level) {
    switch (algo) {
        case CompressionAlgorithm::Gzip:
            return gzip(data, level);
        case CompressionAlgorithm::Deflate:
            return deflate(data, level);
        case CompressionAlgorithm::Brotli:
            return brotli(data, level);
        case CompressionAlgorithm::Identity:
        default:
            return std::string(data);
    }
}

std::optional<std::string> gunzip(std::string_view data) {
    if (data.empty()) {
        return std::string{};
    }
    
    z_stream stream{};
    stream.zalloc = Z_NULL;
    stream.zfree = Z_NULL;
    stream.opaque = Z_NULL;
    stream.next_in = reinterpret_cast<Bytef*>(const_cast<char*>(data.data()));
    stream.avail_in = static_cast<uInt>(data.size());
    
    // windowBits = 15 + 16 for gzip format
    if (inflateInit2(&stream, 15 + 16) != Z_OK) {
        return std::nullopt;
    }
    
    std::string result;
    char buffer[4096];
    
    int ret;
    do {
        stream.next_out = reinterpret_cast<Bytef*>(buffer);
        stream.avail_out = sizeof(buffer);
        
        ret = inflate(&stream, Z_NO_FLUSH);
        if (ret == Z_STREAM_ERROR || ret == Z_DATA_ERROR || ret == Z_MEM_ERROR) {
            inflateEnd(&stream);
            return std::nullopt;
        }
        
        result.append(buffer, sizeof(buffer) - stream.avail_out);
    } while (ret != Z_STREAM_END);
    
    inflateEnd(&stream);
    return result;
}

std::optional<std::string> inflate(std::string_view data) {
    if (data.empty()) {
        return std::string{};
    }
    
    z_stream stream{};
    stream.zalloc = Z_NULL;
    stream.zfree = Z_NULL;
    stream.opaque = Z_NULL;
    stream.next_in = reinterpret_cast<Bytef*>(const_cast<char*>(data.data()));
    stream.avail_in = static_cast<uInt>(data.size());
    
    // windowBits = -15 for raw deflate
    if (inflateInit2(&stream, -15) != Z_OK) {
        return std::nullopt;
    }
    
    std::string result;
    char buffer[4096];
    
    int ret;
    do {
        stream.next_out = reinterpret_cast<Bytef*>(buffer);
        stream.avail_out = sizeof(buffer);
        
        ret = ::inflate(&stream, Z_NO_FLUSH);
        if (ret == Z_STREAM_ERROR || ret == Z_DATA_ERROR || ret == Z_MEM_ERROR) {
            inflateEnd(&stream);
            return std::nullopt;
        }
        
        result.append(buffer, sizeof(buffer) - stream.avail_out);
    } while (ret != Z_STREAM_END);
    
    inflateEnd(&stream);
    return result;
}

std::optional<std::string> brotli_decompress(std::string_view data) {
#ifdef COROUTE_HAS_BROTLI
    if (data.empty()) {
        return std::string{};
    }
    
    // Start with estimated size
    size_t output_size = data.size() * 4;
    std::string result(output_size, '\0');
    
    BrotliDecoderResult ret = BrotliDecoderDecompress(
        data.size(),
        reinterpret_cast<const uint8_t*>(data.data()),
        &output_size,
        reinterpret_cast<uint8_t*>(result.data()));
    
    if (ret == BROTLI_DECODER_RESULT_NEEDS_MORE_OUTPUT) {
        // Try with larger buffer
        output_size = data.size() * 16;
        result.resize(output_size);
        
        ret = BrotliDecoderDecompress(
            data.size(),
            reinterpret_cast<const uint8_t*>(data.data()),
            &output_size,
            reinterpret_cast<uint8_t*>(result.data()));
    }
    
    if (ret != BROTLI_DECODER_RESULT_SUCCESS) {
        return std::nullopt;
    }
    
    result.resize(output_size);
    return result;
#else
    (void)data;
    return std::nullopt;
#endif
}

} // namespace compress

// ============================================================================
// Compression Middleware
// ============================================================================

CompressionMiddleware::CompressionMiddleware(CompressionOptions options)
    : options_(std::move(options))
{
    // Remove Brotli from enabled if not available
    if (!compress::brotli_available()) {
        options_.algorithms.erase(CompressionAlgorithm::Brotli);
    }
}

bool CompressionMiddleware::content_type_matches(std::string_view type, std::string_view pattern) {
    // Extract just the media type (ignore charset, etc.)
    auto semicolon = type.find(';');
    if (semicolon != std::string_view::npos) {
        type = type.substr(0, semicolon);
    }
    type = trim(type);
    
    // Check for wildcard
    if (pattern.ends_with("/*")) {
        auto prefix = pattern.substr(0, pattern.size() - 1);  // "text/"
        return type.substr(0, prefix.size()) == prefix;
    }
    
    // Exact match (case-insensitive)
    return to_lower(type) == to_lower(pattern);
}

bool CompressionMiddleware::should_compress(std::string_view content_type) const {
    for (const auto& pattern : options_.compressible_types) {
        if (content_type_matches(content_type, pattern)) {
            return true;
        }
    }
    return false;
}

Task<Response> CompressionMiddleware::operator()(Request& req, Next next) {
    // Get response from next handler
    Response resp = co_await next(req);
    
    // Skip if response already has Content-Encoding
    if (options_.skip_if_encoded) {
        for (const auto& [key, value] : resp.headers()) {
            if (key == "Content-Encoding") {
                co_return resp;
            }
        }
    }
    
    // Check body size
    if (resp.body().size() < options_.min_size) {
        co_return resp;
    }
    
    // Check content type
    std::string_view content_type;
    for (const auto& [key, value] : resp.headers()) {
        if (key == "Content-Type") {
            content_type = value;
            break;
        }
    }
    
    if (content_type.empty() || !should_compress(content_type)) {
        co_return resp;
    }
    
    // Parse Accept-Encoding
    auto accept_encoding = req.header("Accept-Encoding");
    if (!accept_encoding) {
        co_return resp;
    }
    
    auto algo = parse_accept_encoding(*accept_encoding, options_.algorithms);
    if (algo == CompressionAlgorithm::Identity) {
        co_return resp;
    }
    
    // Compress
    auto compressed = compress::compress(resp.body(), algo, options_.level);
    if (!compressed || compressed->size() >= resp.body().size()) {
        // Compression failed or didn't help
        co_return resp;
    }
    
    // Update response
    resp.set_body(std::move(*compressed));
    resp.set_header("Content-Encoding", std::string(algorithm_name(algo)));
    resp.set_header("Content-Length", std::to_string(resp.body().size()));
    
    // Add Vary header
    if (options_.add_vary_header) {
        resp.set_header("Vary", "Accept-Encoding");
    }
    
    co_return resp;
}

// ============================================================================
// Middleware Factory
// ============================================================================

Middleware compression() {
    return compression(CompressionOptions{});
}

Middleware compression(CompressionOptions options) {
    auto middleware = std::make_shared<CompressionMiddleware>(std::move(options));
    
    return [middleware](Request& req, Next next) -> Task<Response> {
        co_return co_await (*middleware)(req, std::move(next));
    };
}

} // namespace coroute
