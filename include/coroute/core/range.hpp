#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <optional>
#include <cstdint>

#include "coroute/core/request.hpp"
#include "coroute/core/response.hpp"

namespace coroute {

// ============================================================================
// Range Request Types
// ============================================================================

// A single byte range (start and end are inclusive)
struct ByteRange {
    std::optional<int64_t> start;  // nullopt means "from end"
    std::optional<int64_t> end;    // nullopt means "to end"
    
    // Normalize range against total size
    // Returns false if range is invalid/unsatisfiable
    bool normalize(int64_t total_size);
    
    // Get actual start position (after normalization)
    int64_t get_start() const { return start.value_or(0); }
    
    // Get actual end position (after normalization)  
    int64_t get_end() const { return end.value_or(0); }
    
    // Get length of range
    int64_t length() const { return get_end() - get_start() + 1; }
    
    // Format as Content-Range value: "bytes start-end/total"
    std::string to_content_range(int64_t total_size) const;
};

// Parsed Range header
struct RangeHeader {
    std::string unit = "bytes";  // Only "bytes" is supported
    std::vector<ByteRange> ranges;
    
    // Check if this is a valid byte range request
    bool is_valid() const { return unit == "bytes" && !ranges.empty(); }
    
    // Check if this is a single range (vs multipart)
    bool is_single_range() const { return ranges.size() == 1; }
};

// ============================================================================
// Range Parsing
// ============================================================================

namespace range {

// Parse Range header value
// Returns nullopt if header is malformed or not a byte range
// Examples:
//   "bytes=0-499"       -> single range, first 500 bytes
//   "bytes=500-999"     -> single range, bytes 500-999
//   "bytes=-500"        -> suffix range, last 500 bytes
//   "bytes=500-"        -> range from byte 500 to end
//   "bytes=0-0,-1"      -> multiple ranges (first and last byte)
std::optional<RangeHeader> parse(std::string_view header);

// Check if request has a Range header
bool has_range_header(const Request& req);

// Get Range header from request
std::optional<RangeHeader> get_range(const Request& req);

// Check If-Range header (returns true if range should be served)
// If-Range can be an ETag or a date
bool check_if_range(const Request& req, 
                    std::string_view etag,
                    std::string_view last_modified);

} // namespace range

// ============================================================================
// Range Response Builder
// ============================================================================

class RangeResponseBuilder {
public:
    // Set the full content and its metadata
    RangeResponseBuilder& content(std::string data, std::string content_type);
    
    // Set content from file (for large files, reads only needed portions)
    RangeResponseBuilder& file(const std::string& path, std::string content_type);
    
    // Set ETag for If-Range validation
    RangeResponseBuilder& etag(std::string value);
    
    // Set Last-Modified for If-Range validation
    RangeResponseBuilder& last_modified(std::string value);
    
    // Build response for the given request
    // Returns 206 Partial Content for valid range requests
    // Returns 200 OK if no range or If-Range fails
    // Returns 416 Range Not Satisfiable if range is invalid
    Response build(const Request& req);
    
    // Build response for specific range (ignores request headers)
    Response build_range(int64_t start, int64_t end);
    
    // Build full response (200 OK)
    Response build_full();

private:
    std::string content_;
    std::string content_type_ = "application/octet-stream";
    std::string etag_;
    std::string last_modified_;
    std::string file_path_;
    bool use_file_ = false;
    
    // Read portion of file
    std::optional<std::string> read_file_range(int64_t start, int64_t length);
    
    // Build single range response
    Response build_single_range(const ByteRange& range, int64_t total_size);
    
    // Build multipart range response
    Response build_multipart_range(const std::vector<ByteRange>& ranges, int64_t total_size);
    
    // Generate multipart boundary
    static std::string generate_boundary();
};

// ============================================================================
// Convenience Functions
// ============================================================================

// Create a 206 Partial Content response
Response partial_content(std::string_view data, 
                         int64_t start, 
                         int64_t end, 
                         int64_t total,
                         std::string content_type = "application/octet-stream");

// Create a 416 Range Not Satisfiable response
Response range_not_satisfiable(int64_t total_size);

// Check if response should use range (has valid Range header and passes If-Range)
bool should_use_range(const Request& req,
                      std::string_view etag = "",
                      std::string_view last_modified = "");

} // namespace coroute
