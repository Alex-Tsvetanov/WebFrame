#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <functional>
#include <optional>
#include <set>

#include "coroute/core/request.hpp"
#include "coroute/core/response.hpp"
#include "coroute/coro/task.hpp"

namespace coroute {

// Forward declare middleware types
using Next = std::function<Task<Response>(Request&)>;
using Middleware = std::function<Task<Response>(Request&, Next)>;

// ============================================================================
// Compression Algorithms
// ============================================================================

enum class CompressionAlgorithm {
    Gzip,       // gzip (deflate + gzip wrapper)
    Deflate,    // raw deflate
    Brotli,     // Brotli compression (if available)
    Identity    // No compression
};

// Get algorithm name for Content-Encoding header
std::string_view algorithm_name(CompressionAlgorithm algo) noexcept;

// Parse Accept-Encoding header and return preferred algorithm
// Returns Identity if no supported encoding found
CompressionAlgorithm parse_accept_encoding(std::string_view header,
                                            const std::set<CompressionAlgorithm>& enabled);

// ============================================================================
// Compression Options
// ============================================================================

struct CompressionOptions {
    // Enabled algorithms (in preference order)
    std::set<CompressionAlgorithm> algorithms = {
        CompressionAlgorithm::Brotli,
        CompressionAlgorithm::Gzip,
        CompressionAlgorithm::Deflate
    };
    
    // Minimum response size to compress (bytes)
    // Smaller responses may actually get larger after compression
    size_t min_size = 1024;
    
    // Compression level (1-9 for zlib, 0-11 for Brotli)
    // Higher = better compression, slower
    int level = 6;
    
    // Content types to compress (supports wildcards like "text/*")
    std::vector<std::string> compressible_types = {
        "text/html",
        "text/plain",
        "text/css",
        "text/javascript",
        "text/xml",
        "application/json",
        "application/javascript",
        "application/xml",
        "application/xhtml+xml",
        "application/rss+xml",
        "application/atom+xml",
        "image/svg+xml",
        "font/ttf",
        "font/otf",
        "application/wasm"
    };
    
    // Don't compress if response already has Content-Encoding
    bool skip_if_encoded = true;
    
    // Add Vary: Accept-Encoding header
    bool add_vary_header = true;
};

// ============================================================================
// Compression Functions
// ============================================================================

namespace compress {

// Check if Brotli is available (compiled with support)
bool brotli_available() noexcept;

// Compress data using gzip
// Returns empty optional on error
std::optional<std::string> gzip(std::string_view data, int level = 6);

// Compress data using deflate (raw)
std::optional<std::string> deflate(std::string_view data, int level = 6);

// Compress data using Brotli
// Returns empty optional if Brotli not available or on error
std::optional<std::string> brotli(std::string_view data, int level = 6);

// Compress using specified algorithm
std::optional<std::string> compress(std::string_view data, 
                                     CompressionAlgorithm algo,
                                     int level = 6);

// Decompress gzip data
std::optional<std::string> gunzip(std::string_view data);

// Decompress deflate data
std::optional<std::string> inflate(std::string_view data);

// Decompress Brotli data
std::optional<std::string> brotli_decompress(std::string_view data);

} // namespace compress

// ============================================================================
// Compression Middleware
// ============================================================================

class CompressionMiddleware {
public:
    explicit CompressionMiddleware(CompressionOptions options = {});
    
    // Middleware function
    Task<Response> operator()(Request& req, Next next);
    
    // Check if content type should be compressed
    bool should_compress(std::string_view content_type) const;
    
    // Get options
    const CompressionOptions& options() const noexcept { return options_; }

private:
    CompressionOptions options_;
    
    // Check if content type matches pattern (supports "text/*" wildcards)
    static bool content_type_matches(std::string_view type, std::string_view pattern);
};

// ============================================================================
// Middleware Factory
// ============================================================================

// Create compression middleware with default options
Middleware compression();

// Create compression middleware with custom options
Middleware compression(CompressionOptions options);

} // namespace coroute
