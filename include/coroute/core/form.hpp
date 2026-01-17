#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <optional>
#include <cstdint>

#include "coroute/core/request.hpp"
#include "coroute/core/error.hpp"
#include "coroute/util/expected.hpp"

namespace coroute {

// ============================================================================
// Form Data Types
// ============================================================================

// A single form field value
struct FormField {
    std::string name;
    std::string value;
    
    // For file uploads (multipart only)
    std::string filename;
    std::string content_type;
    bool is_file = false;
};

// Parsed form data
class FormData {
    std::vector<FormField> fields_;
    std::unordered_map<std::string, std::vector<size_t>> index_;
    
public:
    FormData() = default;
    
    // Add a field
    void add(FormField field);
    void add(std::string name, std::string value);
    
    // Get first value for a field
    std::optional<std::string_view> get(std::string_view name) const;
    
    // Get all values for a field (for multi-value fields like checkboxes)
    std::vector<std::string_view> get_all(std::string_view name) const;
    
    // Get field (includes file metadata)
    const FormField* get_field(std::string_view name) const;
    
    // Get all fields with a name
    std::vector<const FormField*> get_fields(std::string_view name) const;
    
    // Check if field exists
    bool has(std::string_view name) const;
    
    // Get typed value
    template<typename T>
    std::optional<T> get_as(std::string_view name) const;
    
    // Iterate all fields
    const std::vector<FormField>& fields() const { return fields_; }
    
    // Number of fields
    size_t size() const { return fields_.size(); }
    bool empty() const { return fields_.empty(); }
};

// ============================================================================
// Form Parsing
// ============================================================================

namespace form {

// Parse application/x-www-form-urlencoded body
expected<FormData, Error> parse_urlencoded(std::string_view body);

// Parse multipart/form-data body
// boundary should be extracted from Content-Type header
expected<FormData, Error> parse_multipart(std::string_view body, std::string_view boundary);

// Parse form from request (auto-detects content type)
expected<FormData, Error> parse(const Request& req);

// Extract boundary from Content-Type header
// Returns nullopt if not multipart or boundary not found
std::optional<std::string> extract_boundary(std::string_view content_type);

// URL decode a string
std::string url_decode(std::string_view encoded);

// URL encode a string
std::string url_encode(std::string_view decoded);

} // namespace form

// ============================================================================
// Template Implementation
// ============================================================================

template<typename T>
std::optional<T> FormData::get_as(std::string_view name) const {
    auto value = get(name);
    if (!value) return std::nullopt;
    
    if constexpr (std::is_same_v<T, std::string>) {
        return std::string(*value);
    } else if constexpr (std::is_same_v<T, int>) {
        try { return std::stoi(std::string(*value)); }
        catch (...) { return std::nullopt; }
    } else if constexpr (std::is_same_v<T, int64_t>) {
        try { return std::stoll(std::string(*value)); }
        catch (...) { return std::nullopt; }
    } else if constexpr (std::is_same_v<T, double>) {
        try { return std::stod(std::string(*value)); }
        catch (...) { return std::nullopt; }
    } else if constexpr (std::is_same_v<T, bool>) {
        return *value == "true" || *value == "1" || *value == "on";
    } else {
        static_assert(sizeof(T) == 0, "Unsupported type for get_as");
    }
}

} // namespace coroute
