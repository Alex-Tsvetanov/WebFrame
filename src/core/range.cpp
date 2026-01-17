#include "coroute/core/range.hpp"

#include <algorithm>
#include <sstream>
#include <fstream>
#include <random>
#include <iomanip>
#include <cctype>

namespace coroute {

// ============================================================================
// ByteRange Implementation
// ============================================================================

bool ByteRange::normalize(int64_t total_size) {
    if (total_size <= 0) {
        return false;
    }
    
    // Suffix range: "-500" means last 500 bytes
    if (!start.has_value() && end.has_value()) {
        int64_t suffix_length = end.value();
        if (suffix_length <= 0) {
            return false;
        }
        if (suffix_length >= total_size) {
            // Entire file
            start = 0;
            end = total_size - 1;
        } else {
            start = total_size - suffix_length;
            end = total_size - 1;
        }
        return true;
    }
    
    // Must have start at this point
    if (!start.has_value()) {
        return false;
    }
    
    int64_t s = start.value();
    
    // Start beyond file size
    if (s >= total_size) {
        return false;
    }
    
    // Negative start is invalid
    if (s < 0) {
        return false;
    }
    
    // Open-ended range: "500-" means from 500 to end
    if (!end.has_value()) {
        end = total_size - 1;
        return true;
    }
    
    int64_t e = end.value();
    
    // End before start
    if (e < s) {
        return false;
    }
    
    // Clamp end to file size
    if (e >= total_size) {
        end = total_size - 1;
    }
    
    return true;
}

std::string ByteRange::to_content_range(int64_t total_size) const {
    std::ostringstream oss;
    oss << "bytes " << get_start() << "-" << get_end() << "/" << total_size;
    return oss.str();
}

// ============================================================================
// Range Parsing
// ============================================================================

namespace {

// Trim whitespace
std::string_view trim(std::string_view s) {
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.front()))) {
        s.remove_prefix(1);
    }
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back()))) {
        s.remove_suffix(1);
    }
    return s;
}

// Parse a single range spec like "0-499", "500-", "-500"
std::optional<ByteRange> parse_range_spec(std::string_view spec) {
    spec = trim(spec);
    if (spec.empty()) {
        return std::nullopt;
    }
    
    auto dash = spec.find('-');
    if (dash == std::string_view::npos) {
        return std::nullopt;
    }
    
    ByteRange range;
    
    // Parse start (before dash)
    auto start_str = trim(spec.substr(0, dash));
    if (!start_str.empty()) {
        try {
            range.start = std::stoll(std::string(start_str));
        } catch (...) {
            return std::nullopt;
        }
    }
    
    // Parse end (after dash)
    auto end_str = trim(spec.substr(dash + 1));
    if (!end_str.empty()) {
        try {
            range.end = std::stoll(std::string(end_str));
        } catch (...) {
            return std::nullopt;
        }
    }
    
    // Must have at least start or end
    if (!range.start.has_value() && !range.end.has_value()) {
        return std::nullopt;
    }
    
    return range;
}

} // anonymous namespace

namespace range {

std::optional<RangeHeader> parse(std::string_view header) {
    header = trim(header);
    if (header.empty()) {
        return std::nullopt;
    }
    
    // Find "bytes="
    auto eq = header.find('=');
    if (eq == std::string_view::npos) {
        return std::nullopt;
    }
    
    RangeHeader result;
    result.unit = std::string(trim(header.substr(0, eq)));
    
    // Only support bytes
    if (result.unit != "bytes") {
        return std::nullopt;
    }
    
    // Parse range specs (comma-separated)
    auto specs = header.substr(eq + 1);
    size_t start = 0;
    
    while (start < specs.size()) {
        auto comma = specs.find(',', start);
        auto spec = (comma == std::string_view::npos) 
            ? specs.substr(start) 
            : specs.substr(start, comma - start);
        
        auto range = parse_range_spec(spec);
        if (range) {
            result.ranges.push_back(*range);
        }
        
        if (comma == std::string_view::npos) {
            break;
        }
        start = comma + 1;
    }
    
    if (result.ranges.empty()) {
        return std::nullopt;
    }
    
    return result;
}

bool has_range_header(const Request& req) {
    return req.header("Range").has_value();
}

std::optional<RangeHeader> get_range(const Request& req) {
    auto header = req.header("Range");
    if (!header) {
        return std::nullopt;
    }
    return parse(*header);
}

bool check_if_range(const Request& req, 
                    std::string_view etag,
                    std::string_view last_modified) {
    auto if_range = req.header("If-Range");
    if (!if_range) {
        // No If-Range header means always serve range
        return true;
    }
    
    std::string_view value = *if_range;
    
    // If it looks like an ETag (starts with " or W/)
    if (value.starts_with('"') || value.starts_with("W/")) {
        // Compare ETags
        return !etag.empty() && value == etag;
    }
    
    // Otherwise treat as date - compare with Last-Modified
    // For simplicity, just do string comparison
    // A proper implementation would parse HTTP dates
    return !last_modified.empty() && value == last_modified;
}

} // namespace range

// ============================================================================
// RangeResponseBuilder Implementation
// ============================================================================

RangeResponseBuilder& RangeResponseBuilder::content(std::string data, std::string content_type) {
    content_ = std::move(data);
    content_type_ = std::move(content_type);
    use_file_ = false;
    return *this;
}

RangeResponseBuilder& RangeResponseBuilder::file(const std::string& path, std::string content_type) {
    file_path_ = path;
    content_type_ = std::move(content_type);
    use_file_ = true;
    return *this;
}

RangeResponseBuilder& RangeResponseBuilder::etag(std::string value) {
    etag_ = std::move(value);
    return *this;
}

RangeResponseBuilder& RangeResponseBuilder::last_modified(std::string value) {
    last_modified_ = std::move(value);
    return *this;
}

std::optional<std::string> RangeResponseBuilder::read_file_range(int64_t start, int64_t length) {
    std::ifstream file(file_path_, std::ios::binary);
    if (!file) {
        return std::nullopt;
    }
    
    file.seekg(start);
    if (!file) {
        return std::nullopt;
    }
    
    std::string data(static_cast<size_t>(length), '\0');
    if (!file.read(data.data(), length)) {
        // Partial read is okay at end of file
        data.resize(static_cast<size_t>(file.gcount()));
    }
    
    return data;
}

std::string RangeResponseBuilder::generate_boundary() {
    static const char chars[] = "0123456789abcdef";
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 15);
    
    std::string boundary = "----corouteBoundary";
    for (int i = 0; i < 16; i++) {
        boundary += chars[dis(gen)];
    }
    return boundary;
}

Response RangeResponseBuilder::build_full() {
    Response resp;
    resp.set_status(200);
    resp.set_header("Content-Type", content_type_);
    resp.set_header("Accept-Ranges", "bytes");
    
    if (!etag_.empty()) {
        resp.set_header("ETag", etag_);
    }
    if (!last_modified_.empty()) {
        resp.set_header("Last-Modified", last_modified_);
    }
    
    if (use_file_) {
        std::ifstream file(file_path_, std::ios::binary | std::ios::ate);
        if (file) {
            auto size = file.tellg();
            file.seekg(0);
            std::string data(static_cast<size_t>(size), '\0');
            file.read(data.data(), size);
            resp.set_body(std::move(data));
        }
    } else {
        resp.set_body(content_);
    }
    
    resp.set_header("Content-Length", std::to_string(resp.body().size()));
    return resp;
}

Response RangeResponseBuilder::build_single_range(const ByteRange& range, int64_t total_size) {
    Response resp;
    resp.set_status(206);
    resp.set_header("Content-Type", content_type_);
    resp.set_header("Accept-Ranges", "bytes");
    resp.set_header("Content-Range", range.to_content_range(total_size));
    
    if (!etag_.empty()) {
        resp.set_header("ETag", etag_);
    }
    if (!last_modified_.empty()) {
        resp.set_header("Last-Modified", last_modified_);
    }
    
    // Extract the range data
    std::string data;
    if (use_file_) {
        auto file_data = read_file_range(range.get_start(), range.length());
        if (file_data) {
            data = std::move(*file_data);
        }
    } else {
        data = content_.substr(
            static_cast<size_t>(range.get_start()),
            static_cast<size_t>(range.length())
        );
    }
    
    resp.set_header("Content-Length", std::to_string(data.size()));
    resp.set_body(std::move(data));
    return resp;
}

Response RangeResponseBuilder::build_multipart_range(const std::vector<ByteRange>& ranges, int64_t total_size) {
    std::string boundary = generate_boundary();
    
    Response resp;
    resp.set_status(206);
    resp.set_header("Content-Type", "multipart/byteranges; boundary=" + boundary);
    resp.set_header("Accept-Ranges", "bytes");
    
    if (!etag_.empty()) {
        resp.set_header("ETag", etag_);
    }
    if (!last_modified_.empty()) {
        resp.set_header("Last-Modified", last_modified_);
    }
    
    std::ostringstream body;
    
    for (const auto& range : ranges) {
        body << "--" << boundary << "\r\n";
        body << "Content-Type: " << content_type_ << "\r\n";
        body << "Content-Range: " << range.to_content_range(total_size) << "\r\n";
        body << "\r\n";
        
        // Extract range data
        if (use_file_) {
            auto file_data = read_file_range(range.get_start(), range.length());
            if (file_data) {
                body << *file_data;
            }
        } else {
            body << content_.substr(
                static_cast<size_t>(range.get_start()),
                static_cast<size_t>(range.length())
            );
        }
        body << "\r\n";
    }
    
    body << "--" << boundary << "--\r\n";
    
    std::string body_str = body.str();
    resp.set_header("Content-Length", std::to_string(body_str.size()));
    resp.set_body(std::move(body_str));
    return resp;
}

Response RangeResponseBuilder::build(const Request& req) {
    // Get total size
    int64_t total_size;
    if (use_file_) {
        std::ifstream file(file_path_, std::ios::binary | std::ios::ate);
        if (!file) {
            return Response::not_found();
        }
        total_size = file.tellg();
    } else {
        total_size = static_cast<int64_t>(content_.size());
    }
    
    // Check for Range header
    auto range_header = range::get_range(req);
    if (!range_header || !range_header->is_valid()) {
        return build_full();
    }
    
    // Check If-Range
    if (!range::check_if_range(req, etag_, last_modified_)) {
        return build_full();
    }
    
    // Normalize and validate ranges
    std::vector<ByteRange> valid_ranges;
    for (auto range : range_header->ranges) {
        if (range.normalize(total_size)) {
            valid_ranges.push_back(range);
        }
    }
    
    if (valid_ranges.empty()) {
        return range_not_satisfiable(total_size);
    }
    
    // Single range vs multipart
    if (valid_ranges.size() == 1) {
        return build_single_range(valid_ranges[0], total_size);
    } else {
        return build_multipart_range(valid_ranges, total_size);
    }
}

Response RangeResponseBuilder::build_range(int64_t start, int64_t end) {
    int64_t total_size;
    if (use_file_) {
        std::ifstream file(file_path_, std::ios::binary | std::ios::ate);
        if (!file) {
            return Response::not_found();
        }
        total_size = file.tellg();
    } else {
        total_size = static_cast<int64_t>(content_.size());
    }
    
    ByteRange range;
    range.start = start;
    range.end = end;
    
    if (!range.normalize(total_size)) {
        return range_not_satisfiable(total_size);
    }
    
    return build_single_range(range, total_size);
}

// ============================================================================
// Convenience Functions
// ============================================================================

Response partial_content(std::string_view data, 
                         int64_t start, 
                         int64_t end, 
                         int64_t total,
                         std::string content_type) {
    Response resp;
    resp.set_status(206);
    resp.set_header("Content-Type", std::move(content_type));
    resp.set_header("Accept-Ranges", "bytes");
    
    std::ostringstream range;
    range << "bytes " << start << "-" << end << "/" << total;
    resp.set_header("Content-Range", range.str());
    
    resp.set_body(std::string(data));
    resp.set_header("Content-Length", std::to_string(data.size()));
    
    return resp;
}

Response range_not_satisfiable(int64_t total_size) {
    Response resp;
    resp.set_status(416);
    resp.set_header("Content-Range", "bytes */" + std::to_string(total_size));
    resp.set_header("Content-Length", "0");
    return resp;
}

bool should_use_range(const Request& req,
                      std::string_view etag,
                      std::string_view last_modified) {
    if (!range::has_range_header(req)) {
        return false;
    }
    
    return range::check_if_range(req, etag, last_modified);
}

} // namespace coroute
