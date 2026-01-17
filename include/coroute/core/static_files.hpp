#pragma once

#include <string>
#include <string_view>
#include <unordered_map>
#include <filesystem>
#include <optional>
#include <functional>
#include <memory>

#include "coroute/core/request.hpp"
#include "coroute/core/response.hpp"
#include "coroute/coro/task.hpp"

namespace coroute {

// Forward declare middleware types (also defined in app.hpp)
using Next = std::function<Task<Response>(Request&)>;
using Middleware = std::function<Task<Response>(Request&, Next)>;

// ============================================================================
// MIME Types
// ============================================================================

class MimeTypes {
public:
    // Get MIME type for file extension (without dot)
    static std::string_view get(std::string_view extension) noexcept;
    
    // Get MIME type from filename/path
    static std::string_view from_path(std::string_view path) noexcept;
    
    // Register custom MIME type
    static void register_type(std::string extension, std::string mime_type);

private:
    static std::unordered_map<std::string, std::string>& custom_types();
};

// ============================================================================
// Static File Options
// ============================================================================

struct StaticFileOptions {
    // Root directory for static files
    std::filesystem::path root;
    
    // URL prefix to strip (e.g., "/static" means /static/foo.js -> foo.js)
    std::string url_prefix = "";
    
    // Index file to serve for directory requests
    std::string index_file = "index.html";
    
    // Enable directory listing (security risk, disabled by default)
    bool directory_listing = false;
    
    // Enable ETag generation for caching
    bool etag = true;
    
    // Enable Last-Modified header
    bool last_modified = true;
    
    // Max age for Cache-Control header (0 = no cache header)
    int max_age_seconds = 0;
    
    // Immutable cache (for versioned assets)
    bool immutable = false;
    
    // Enable Range request support (for resumable downloads, video streaming)
    bool range_requests = true;
    
    // Custom headers to add to all responses
    std::vector<std::pair<std::string, std::string>> custom_headers;
    
    // File size limit (0 = no limit)
    size_t max_file_size = 0;
    
    // Allowed extensions (empty = all allowed)
    std::vector<std::string> allowed_extensions;
    
    // Denied extensions (takes precedence over allowed)
    std::vector<std::string> denied_extensions = {".exe", ".dll", ".so", ".sh", ".bat", ".cmd"};
};

// ============================================================================
// Static File Server
// ============================================================================

class StaticFileServer {
public:
    explicit StaticFileServer(StaticFileOptions options);
    
    // Serve a file based on request path
    // Returns nullopt if file not found or not allowed
    Task<std::optional<Response>> serve(const Request& req) const;
    
    // Serve a specific file path
    Task<std::optional<Response>> serve_file(const std::filesystem::path& file_path,
                                              const Request& req) const;
    
    // Check if path is allowed (security checks)
    bool is_path_allowed(const std::filesystem::path& path) const;
    
    // Get options
    const StaticFileOptions& options() const noexcept { return options_; }

private:
    StaticFileOptions options_;
    
    // Generate ETag from file metadata
    std::string generate_etag(const std::filesystem::path& path,
                              std::uintmax_t size,
                              std::filesystem::file_time_type mtime) const;
    
    // Check if extension is allowed
    bool is_extension_allowed(std::string_view ext) const;
    
    // Build cache headers
    void add_cache_headers(Response& resp, 
                           const std::filesystem::path& path,
                           std::uintmax_t size,
                           std::filesystem::file_time_type mtime) const;
    
    // Check conditional request (If-None-Match, If-Modified-Since)
    bool check_not_modified(const Request& req, 
                            const std::string& etag,
                            std::filesystem::file_time_type mtime) const;
    
    // Read file contents
    std::optional<std::string> read_file(const std::filesystem::path& path) const;
    
    // Generate directory listing HTML
    std::string generate_directory_listing(const std::filesystem::path& dir,
                                           std::string_view url_path) const;
};

// ============================================================================
// Middleware Factory
// ============================================================================

// Create a middleware that serves static files
// Falls through to next handler if file not found
Middleware static_files(StaticFileOptions options);

// Convenience: serve files from a directory with default options
Middleware static_files(const std::filesystem::path& root, std::string url_prefix = "");

} // namespace coroute
