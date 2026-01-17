#include "coroute/core/static_files.hpp"
#include "coroute/core/range.hpp"

#include <fstream>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <algorithm>
#include <cctype>

namespace coroute {

// ============================================================================
// MIME Types Implementation
// ============================================================================

namespace {

// Default MIME types table
const std::unordered_map<std::string, std::string_view> default_mime_types = {
    // Text
    {"html", "text/html"},
    {"htm", "text/html"},
    {"css", "text/css"},
    {"js", "text/javascript"},
    {"mjs", "text/javascript"},
    {"json", "application/json"},
    {"xml", "application/xml"},
    {"txt", "text/plain"},
    {"csv", "text/csv"},
    {"md", "text/markdown"},
    
    // Images
    {"png", "image/png"},
    {"jpg", "image/jpeg"},
    {"jpeg", "image/jpeg"},
    {"gif", "image/gif"},
    {"svg", "image/svg+xml"},
    {"ico", "image/x-icon"},
    {"webp", "image/webp"},
    {"avif", "image/avif"},
    {"bmp", "image/bmp"},
    {"tiff", "image/tiff"},
    {"tif", "image/tiff"},
    
    // Fonts
    {"woff", "font/woff"},
    {"woff2", "font/woff2"},
    {"ttf", "font/ttf"},
    {"otf", "font/otf"},
    {"eot", "application/vnd.ms-fontobject"},
    
    // Audio
    {"mp3", "audio/mpeg"},
    {"wav", "audio/wav"},
    {"ogg", "audio/ogg"},
    {"m4a", "audio/mp4"},
    {"flac", "audio/flac"},
    
    // Video
    {"mp4", "video/mp4"},
    {"webm", "video/webm"},
    {"ogv", "video/ogg"},
    {"avi", "video/x-msvideo"},
    {"mov", "video/quicktime"},
    
    // Documents
    {"pdf", "application/pdf"},
    {"doc", "application/msword"},
    {"docx", "application/vnd.openxmlformats-officedocument.wordprocessingml.document"},
    {"xls", "application/vnd.ms-excel"},
    {"xlsx", "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet"},
    {"ppt", "application/vnd.ms-powerpoint"},
    {"pptx", "application/vnd.openxmlformats-officedocument.presentationml.presentation"},
    
    // Archives
    {"zip", "application/zip"},
    {"gz", "application/gzip"},
    {"tar", "application/x-tar"},
    {"rar", "application/vnd.rar"},
    {"7z", "application/x-7z-compressed"},
    
    // Web
    {"wasm", "application/wasm"},
    {"map", "application/json"},
    {"webmanifest", "application/manifest+json"},
};

std::string to_lower(std::string_view str) {
    std::string result(str);
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return result;
}

} // anonymous namespace

std::unordered_map<std::string, std::string>& MimeTypes::custom_types() {
    static std::unordered_map<std::string, std::string> types;
    return types;
}

std::string_view MimeTypes::get(std::string_view extension) noexcept {
    std::string ext = to_lower(extension);
    
    // Check custom types first
    auto& custom = custom_types();
    auto it = custom.find(ext);
    if (it != custom.end()) {
        return it->second;
    }
    
    // Check default types
    auto dit = default_mime_types.find(ext);
    if (dit != default_mime_types.end()) {
        return dit->second;
    }
    
    return "application/octet-stream";
}

std::string_view MimeTypes::from_path(std::string_view path) noexcept {
    auto dot_pos = path.rfind('.');
    if (dot_pos == std::string_view::npos || dot_pos == path.size() - 1) {
        return "application/octet-stream";
    }
    return get(path.substr(dot_pos + 1));
}

void MimeTypes::register_type(std::string extension, std::string mime_type) {
    custom_types()[to_lower(extension)] = std::move(mime_type);
}

// ============================================================================
// Static File Server Implementation
// ============================================================================

StaticFileServer::StaticFileServer(StaticFileOptions options)
    : options_(std::move(options))
{
    // Normalize root path
    if (!options_.root.empty()) {
        options_.root = std::filesystem::absolute(options_.root);
    }
    
    // Normalize URL prefix (ensure it starts with / and doesn't end with /)
    if (!options_.url_prefix.empty()) {
        if (options_.url_prefix.front() != '/') {
            options_.url_prefix = "/" + options_.url_prefix;
        }
        while (options_.url_prefix.size() > 1 && options_.url_prefix.back() == '/') {
            options_.url_prefix.pop_back();
        }
    }
}

bool StaticFileServer::is_extension_allowed(std::string_view ext) const {
    std::string lower_ext = to_lower(ext);
    if (lower_ext.empty() || lower_ext[0] != '.') {
        lower_ext = "." + lower_ext;
    }
    
    // Check denied list first
    for (const auto& denied : options_.denied_extensions) {
        if (to_lower(denied) == lower_ext) {
            return false;
        }
    }
    
    // If allowed list is empty, allow all (except denied)
    if (options_.allowed_extensions.empty()) {
        return true;
    }
    
    // Check allowed list
    for (const auto& allowed : options_.allowed_extensions) {
        if (to_lower(allowed) == lower_ext) {
            return true;
        }
    }
    
    return false;
}

bool StaticFileServer::is_path_allowed(const std::filesystem::path& path) const {
    // Get canonical paths for comparison
    std::error_code ec;
    auto canonical = std::filesystem::weakly_canonical(path, ec);
    if (ec) {
        return false;
    }
    
    auto root_canonical = std::filesystem::weakly_canonical(options_.root, ec);
    if (ec) {
        return false;
    }
    
    // Check that the path is within root (prevent directory traversal)
    auto [root_end, path_end] = std::mismatch(
        root_canonical.begin(), root_canonical.end(),
        canonical.begin(), canonical.end()
    );
    
    // Path must start with root
    if (root_end != root_canonical.end()) {
        return false;
    }
    
    return true;
}

std::string StaticFileServer::generate_etag(
    const std::filesystem::path& path,
    std::uintmax_t size,
    std::filesystem::file_time_type mtime) const
{
    // Simple ETag: hash of path + size + mtime
    auto mtime_ns = mtime.time_since_epoch().count();
    
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    oss << '"';
    oss << std::setw(8) << (std::hash<std::string>{}(path.string()) & 0xFFFFFFFF);
    oss << '-';
    oss << std::setw(8) << (size & 0xFFFFFFFF);
    oss << '-';
    oss << std::setw(16) << mtime_ns;
    oss << '"';
    
    return oss.str();
}

void StaticFileServer::add_cache_headers(
    Response& resp,
    const std::filesystem::path& path,
    std::uintmax_t size,
    std::filesystem::file_time_type mtime) const
{
    // ETag
    if (options_.etag) {
        resp.set_header("ETag", generate_etag(path, size, mtime));
    }
    
    // Last-Modified
    if (options_.last_modified) {
        // Convert file_time_type to system_clock for formatting
        auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
            mtime - std::filesystem::file_time_type::clock::now() + 
            std::chrono::system_clock::now()
        );
        auto time_t_val = std::chrono::system_clock::to_time_t(sctp);
        
        std::ostringstream oss;
        oss << std::put_time(std::gmtime(&time_t_val), "%a, %d %b %Y %H:%M:%S GMT");
        resp.set_header("Last-Modified", oss.str());
    }
    
    // Cache-Control
    if (options_.max_age_seconds > 0) {
        std::string cache_control = "max-age=" + std::to_string(options_.max_age_seconds);
        if (options_.immutable) {
            cache_control += ", immutable";
        }
        resp.set_header("Cache-Control", cache_control);
    }
}

bool StaticFileServer::check_not_modified(
    const Request& req,
    const std::string& etag,
    std::filesystem::file_time_type mtime) const
{
    // Check If-None-Match (ETag)
    auto if_none_match = req.header("If-None-Match");
    if (if_none_match && options_.etag) {
        // Simple comparison (doesn't handle weak ETags or multiple values)
        if (*if_none_match == etag) {
            return true;
        }
    }
    
    // Check If-Modified-Since
    auto if_modified_since = req.header("If-Modified-Since");
    if (if_modified_since && options_.last_modified) {
        // Parse the date and compare
        // For simplicity, we'll skip full parsing and just use ETag
        // A production implementation would parse HTTP dates
    }
    
    return false;
}

std::optional<std::string> StaticFileServer::read_file(const std::filesystem::path& path) const {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) {
        return std::nullopt;
    }
    
    auto size = file.tellg();
    if (size < 0) {
        return std::nullopt;
    }
    
    // Check file size limit
    if (options_.max_file_size > 0 && static_cast<size_t>(size) > options_.max_file_size) {
        return std::nullopt;
    }
    
    file.seekg(0);
    std::string content(static_cast<size_t>(size), '\0');
    if (!file.read(content.data(), size)) {
        return std::nullopt;
    }
    
    return content;
}

std::string StaticFileServer::generate_directory_listing(
    const std::filesystem::path& dir,
    std::string_view url_path) const
{
    std::ostringstream html;
    html << "<!DOCTYPE html>\n";
    html << "<html><head><meta charset=\"utf-8\">\n";
    html << "<title>Index of " << url_path << "</title>\n";
    html << "<style>\n";
    html << "body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif; margin: 40px; }\n";
    html << "h1 { border-bottom: 1px solid #ddd; padding-bottom: 10px; }\n";
    html << "table { border-collapse: collapse; width: 100%; }\n";
    html << "th, td { text-align: left; padding: 8px 12px; border-bottom: 1px solid #eee; }\n";
    html << "th { background: #f5f5f5; }\n";
    html << "a { color: #0066cc; text-decoration: none; }\n";
    html << "a:hover { text-decoration: underline; }\n";
    html << ".size { text-align: right; color: #666; }\n";
    html << ".dir { color: #0066cc; font-weight: bold; }\n";
    html << "</style></head><body>\n";
    html << "<h1>Index of " << url_path << "</h1>\n";
    html << "<table><thead><tr><th>Name</th><th class=\"size\">Size</th></tr></thead><tbody>\n";
    
    // Parent directory link
    if (url_path != "/" && !url_path.empty()) {
        html << "<tr><td><a href=\"..\">..</a></td><td class=\"size\">-</td></tr>\n";
    }
    
    std::error_code ec;
    for (const auto& entry : std::filesystem::directory_iterator(dir, ec)) {
        if (ec) break;
        
        auto name = entry.path().filename().string();
        bool is_dir = entry.is_directory(ec);
        
        html << "<tr><td>";
        if (is_dir) {
            html << "<span class=\"dir\"><a href=\"" << name << "/\">" << name << "/</a></span>";
        } else {
            html << "<a href=\"" << name << "\">" << name << "</a>";
        }
        html << "</td><td class=\"size\">";
        
        if (is_dir) {
            html << "-";
        } else {
            auto size = entry.file_size(ec);
            if (!ec) {
                if (size < 1024) {
                    html << size << " B";
                } else if (size < 1024 * 1024) {
                    html << std::fixed << std::setprecision(1) << (size / 1024.0) << " KB";
                } else {
                    html << std::fixed << std::setprecision(1) << (size / (1024.0 * 1024.0)) << " MB";
                }
            }
        }
        html << "</td></tr>\n";
    }
    
    html << "</tbody></table></body></html>\n";
    return html.str();
}

Task<std::optional<Response>> StaticFileServer::serve(const Request& req) const {
    // Only handle GET and HEAD
    if (req.method() != HttpMethod::GET && req.method() != HttpMethod::HEAD) {
        co_return std::nullopt;
    }
    
    std::string url_path(req.path());
    
    // Check URL prefix
    if (!options_.url_prefix.empty()) {
        if (url_path.find(options_.url_prefix) != 0) {
            co_return std::nullopt;
        }
        url_path = url_path.substr(options_.url_prefix.size());
        if (url_path.empty()) {
            url_path = "/";
        }
    }
    
    // Decode URL (basic - handle %XX)
    // Note: The request parser should have already decoded this
    
    // Remove leading slash and build file path
    if (!url_path.empty() && url_path[0] == '/') {
        url_path = url_path.substr(1);
    }
    
    auto file_path = options_.root / url_path;
    
    co_return co_await serve_file(file_path, req);
}

Task<std::optional<Response>> StaticFileServer::serve_file(
    const std::filesystem::path& file_path,
    const Request& req) const
{
    // Security check
    if (!is_path_allowed(file_path)) {
        co_return std::nullopt;
    }
    
    std::error_code ec;
    auto status = std::filesystem::status(file_path, ec);
    
    if (ec || !std::filesystem::exists(status)) {
        co_return std::nullopt;
    }
    
    // Handle directory
    if (std::filesystem::is_directory(status)) {
        // Try index file
        auto index_path = file_path / options_.index_file;
        if (std::filesystem::exists(index_path, ec)) {
            co_return co_await serve_file(index_path, req);
        }
        
        // Directory listing
        if (options_.directory_listing) {
            auto listing = generate_directory_listing(file_path, req.path());
            Response resp = Response::html(std::move(listing));
            
            // Add custom headers
            for (const auto& [key, value] : options_.custom_headers) {
                resp.set_header(key, value);
            }
            
            co_return resp;
        }
        
        co_return std::nullopt;
    }
    
    // Check extension
    auto ext = file_path.extension().string();
    if (!is_extension_allowed(ext)) {
        co_return std::nullopt;
    }
    
    // Get file info
    auto size = std::filesystem::file_size(file_path, ec);
    if (ec) {
        co_return std::nullopt;
    }
    
    auto mtime = std::filesystem::last_write_time(file_path, ec);
    if (ec) {
        co_return std::nullopt;
    }
    
    // Check conditional request
    std::string etag;
    if (options_.etag) {
        etag = generate_etag(file_path, size, mtime);
    }
    
    // Generate Last-Modified string
    std::string last_modified_str;
    if (options_.last_modified) {
        auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
            mtime - std::filesystem::file_time_type::clock::now() + 
            std::chrono::system_clock::now()
        );
        auto time_t_val = std::chrono::system_clock::to_time_t(sctp);
        std::ostringstream oss;
        oss << std::put_time(std::gmtime(&time_t_val), "%a, %d %b %Y %H:%M:%S GMT");
        last_modified_str = oss.str();
    }
    
    if (check_not_modified(req, etag, mtime)) {
        Response resp;
        resp.set_status(304);
        if (!etag.empty()) {
            resp.set_header("ETag", etag);
        }
        co_return resp;
    }
    
    std::string content_type = std::string(MimeTypes::from_path(file_path.string()));
    
    // Check for Range request
    if (options_.range_requests && range::has_range_header(req)) {
        // Use RangeResponseBuilder for range requests
        RangeResponseBuilder builder;
        builder.file(file_path.string(), content_type);
        
        if (!etag.empty()) {
            builder.etag(etag);
        }
        if (!last_modified_str.empty()) {
            builder.last_modified(last_modified_str);
        }
        
        Response resp = builder.build(req);
        
        // Add custom headers
        for (const auto& [key, value] : options_.custom_headers) {
            resp.set_header(key, value);
        }
        
        co_return resp;
    }
    
    // Build response - use zero-copy file response for large files
    Response resp;
    resp.set_status(200);
    resp.set_header("Content-Type", content_type);
    resp.set_header("Content-Length", std::to_string(size));
    
    // Indicate range support
    if (options_.range_requests) {
        resp.set_header("Accept-Ranges", "bytes");
    }
    
    // Add cache headers
    add_cache_headers(resp, file_path, size, mtime);
    
    // Add custom headers
    for (const auto& [key, value] : options_.custom_headers) {
        resp.set_header(key, value);
    }
    
    // Use zero-copy for files (App will handle via TransmitFile)
    // For HEAD requests, don't set file - headers only
    if (req.method() != HttpMethod::HEAD) {
        resp.set_file(file_path, 0, size);
    }
    
    co_return resp;
}

// ============================================================================
// Middleware Factory
// ============================================================================

Middleware static_files(StaticFileOptions options) {
    auto server = std::make_shared<StaticFileServer>(std::move(options));
    
    return [server](Request& req, Next next) -> Task<Response> {
        auto result = co_await server->serve(req);
        if (result) {
            co_return std::move(*result);
        }
        // Fall through to next handler
        co_return co_await next(req);
    };
}

Middleware static_files(const std::filesystem::path& root, std::string url_prefix) {
    StaticFileOptions options;
    options.root = root;
    options.url_prefix = std::move(url_prefix);
    return static_files(std::move(options));
}

} // namespace coroute
