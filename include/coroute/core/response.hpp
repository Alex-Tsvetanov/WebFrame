#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <utility>
#include <sstream>
#include <optional>
#include <filesystem>

namespace coroute {

// ============================================================================
// File Response Info (for zero-copy file serving)
// ============================================================================

struct FileResponseInfo {
    std::filesystem::path path;
    size_t offset = 0;
    size_t length = 0;  // 0 = entire file from offset
};

// ============================================================================
// Response
// ============================================================================

class Response {
public:
    using Header = std::pair<std::string, std::string>;
    using Headers = std::vector<Header>;

private:
    int status_ = 200;
    std::string status_text_ = "OK";
    Headers headers_;
    std::string body_;
    std::optional<FileResponseInfo> file_info_;  // For zero-copy file serving

public:
    Response() = default;
    
    Response(int status, Headers headers, std::string body)
        : status_(status)
        , status_text_(default_status_text(status))
        , headers_(std::move(headers))
        , body_(std::move(body)) {}

    // Accessors
    int status() const noexcept { return status_; }
    std::string_view status_text() const noexcept { return status_text_; }
    const Headers& headers() const noexcept { return headers_; }
    std::string_view body() const noexcept { return body_; }

    // Mutators
    void set_header(std::string key, std::string value) {
        // Check if header already exists, update if so
        for (auto& [k, v] : headers_) {
            if (k == key) {
                v = std::move(value);
                return;
            }
        }
        headers_.emplace_back(std::move(key), std::move(value));
    }
    
    // Add header (allows duplicates, needed for Set-Cookie)
    void add_header(std::string key, std::string value) {
        headers_.emplace_back(std::move(key), std::move(value));
    }
    
    void set_status(int status) {
        status_ = status;
        status_text_ = default_status_text(status);
    }
    
    void set_body(std::string body) {
        body_ = std::move(body);
    }

    // Serialize to HTTP response string
    std::string serialize() const;
    
    // Serialize headers only (for zero-copy file responses)
    std::string serialize_headers() const;

    // Static factory methods
    static Response ok(std::string body = "", std::string content_type = "text/plain") {
        Response r;
        r.status_ = 200;
        r.status_text_ = "OK";
        r.body_ = std::move(body);
        if (!r.body_.empty()) {
            r.headers_.emplace_back("Content-Type", std::move(content_type));
            r.headers_.emplace_back("Content-Length", std::to_string(r.body_.size()));
        }
        return r;
    }

    static Response json(std::string body) {
        return ok(std::move(body), "application/json");
    }

    static Response html(std::string body) {
        return ok(std::move(body), "text/html");
    }

    static Response not_found(std::string body = "Not Found") {
        Response r;
        r.status_ = 404;
        r.status_text_ = "Not Found";
        r.body_ = std::move(body);
        r.headers_.emplace_back("Content-Type", "text/plain");
        r.headers_.emplace_back("Content-Length", std::to_string(r.body_.size()));
        return r;
    }

    static Response bad_request(std::string body = "Bad Request") {
        Response r;
        r.status_ = 400;
        r.status_text_ = "Bad Request";
        r.body_ = std::move(body);
        r.headers_.emplace_back("Content-Type", "text/plain");
        r.headers_.emplace_back("Content-Length", std::to_string(r.body_.size()));
        return r;
    }

    static Response internal_error(std::string body = "Internal Server Error") {
        Response r;
        r.status_ = 500;
        r.status_text_ = "Internal Server Error";
        r.body_ = std::move(body);
        r.headers_.emplace_back("Content-Type", "text/plain");
        r.headers_.emplace_back("Content-Length", std::to_string(r.body_.size()));
        return r;
    }

    static Response redirect(std::string location, int status = 302) {
        Response r;
        r.status_ = status;
        r.status_text_ = default_status_text(status);
        r.headers_.emplace_back("Location", std::move(location));
        r.headers_.emplace_back("Content-Length", "0");
        return r;
    }

    // Reset for object pooling
    void reset() {
        status_ = 200;
        status_text_ = "OK";
        headers_.clear();
        body_.clear();
        file_info_.reset();
    }
    
    // Zero-copy file response
    void set_file(const std::filesystem::path& path, size_t offset = 0, size_t length = 0) {
        file_info_ = FileResponseInfo{path, offset, length};
        body_.clear();  // File replaces body
    }
    
    bool has_file() const noexcept { return file_info_.has_value(); }
    const FileResponseInfo& file_info() const { return *file_info_; }
    
    // Create a file response (headers only, body sent via zero-copy)
    static Response file(const std::filesystem::path& path, 
                         std::string_view content_type,
                         size_t file_size,
                         size_t offset = 0,
                         size_t length = 0) {
        Response r;
        r.status_ = 200;
        r.headers_.emplace_back("Content-Type", std::string(content_type));
        size_t actual_length = (length == 0) ? (file_size - offset) : length;
        r.headers_.emplace_back("Content-Length", std::to_string(actual_length));
        r.set_file(path, offset, actual_length);
        return r;
    }

private:
    static std::string_view default_status_text(int status) noexcept;
};

// ============================================================================
// ResponseBuilder - Mutable, fluent builder
// ============================================================================

class ResponseBuilder {
    int status_ = 200;
    Response::Headers headers_;
    std::string body_;

public:
    ResponseBuilder() = default;

    // Fluent setters
    ResponseBuilder& status(int code) {
        status_ = code;
        return *this;
    }

    ResponseBuilder& header(std::string key, std::string value) {
        headers_.emplace_back(std::move(key), std::move(value));
        return *this;
    }

    ResponseBuilder& content_type(std::string type) {
        return header("Content-Type", std::move(type));
    }

    ResponseBuilder& body(std::string content) {
        body_ = std::move(content);
        return *this;
    }

    ResponseBuilder& json_body(std::string content) {
        body_ = std::move(content);
        return content_type("application/json");
    }

    ResponseBuilder& html_body(std::string content) {
        body_ = std::move(content);
        return content_type("text/html");
    }

    // Build final response
    Response build() {
        // Add Content-Length if body is set and not already present
        bool has_content_length = false;
        for (const auto& [k, v] : headers_) {
            if (k == "Content-Length") {
                has_content_length = true;
                break;
            }
        }
        if (!has_content_length && !body_.empty()) {
            headers_.emplace_back("Content-Length", std::to_string(body_.size()));
        }
        
        return Response(status_, std::move(headers_), std::move(body_));
    }

    // Implicit conversion to Response
    operator Response() {
        return build();
    }
};

} // namespace coroute
