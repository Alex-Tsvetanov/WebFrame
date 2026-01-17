#include "coroute/core/form.hpp"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <iomanip>

namespace coroute {

// ============================================================================
// FormData Implementation
// ============================================================================

void FormData::add(FormField field) {
    size_t idx = fields_.size();
    index_[field.name].push_back(idx);
    fields_.push_back(std::move(field));
}

void FormData::add(std::string name, std::string value) {
    add(FormField{std::move(name), std::move(value)});
}

std::optional<std::string_view> FormData::get(std::string_view name) const {
    auto it = index_.find(std::string(name));
    if (it == index_.end() || it->second.empty()) {
        return std::nullopt;
    }
    return fields_[it->second[0]].value;
}

std::vector<std::string_view> FormData::get_all(std::string_view name) const {
    std::vector<std::string_view> result;
    auto it = index_.find(std::string(name));
    if (it != index_.end()) {
        for (size_t idx : it->second) {
            result.push_back(fields_[idx].value);
        }
    }
    return result;
}

const FormField* FormData::get_field(std::string_view name) const {
    auto it = index_.find(std::string(name));
    if (it == index_.end() || it->second.empty()) {
        return nullptr;
    }
    return &fields_[it->second[0]];
}

std::vector<const FormField*> FormData::get_fields(std::string_view name) const {
    std::vector<const FormField*> result;
    auto it = index_.find(std::string(name));
    if (it != index_.end()) {
        for (size_t idx : it->second) {
            result.push_back(&fields_[idx]);
        }
    }
    return result;
}

bool FormData::has(std::string_view name) const {
    return index_.count(std::string(name)) > 0;
}

// ============================================================================
// URL Encoding/Decoding
// ============================================================================

namespace {

int hex_digit(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

} // anonymous namespace

namespace form {

std::string url_decode(std::string_view encoded) {
    std::string result;
    result.reserve(encoded.size());
    
    for (size_t i = 0; i < encoded.size(); ++i) {
        char c = encoded[i];
        if (c == '+') {
            result += ' ';
        } else if (c == '%' && i + 2 < encoded.size()) {
            int hi = hex_digit(encoded[i + 1]);
            int lo = hex_digit(encoded[i + 2]);
            if (hi >= 0 && lo >= 0) {
                result += static_cast<char>((hi << 4) | lo);
                i += 2;
            } else {
                result += c;
            }
        } else {
            result += c;
        }
    }
    
    return result;
}

std::string url_encode(std::string_view decoded) {
    std::ostringstream oss;
    oss << std::hex << std::uppercase;
    
    for (char c : decoded) {
        if (std::isalnum(static_cast<unsigned char>(c)) || 
            c == '-' || c == '_' || c == '.' || c == '~') {
            oss << c;
        } else if (c == ' ') {
            oss << '+';
        } else {
            oss << '%' << std::setw(2) << std::setfill('0') 
                << static_cast<int>(static_cast<unsigned char>(c));
        }
    }
    
    return oss.str();
}

// ============================================================================
// URL-Encoded Form Parsing
// ============================================================================

expected<FormData, Error> parse_urlencoded(std::string_view body) {
    FormData data;
    
    if (body.empty()) {
        return data;
    }
    
    size_t start = 0;
    while (start < body.size()) {
        // Find end of this pair
        size_t end = body.find('&', start);
        if (end == std::string_view::npos) {
            end = body.size();
        }
        
        auto pair = body.substr(start, end - start);
        
        // Find = separator
        auto eq = pair.find('=');
        std::string name, value;
        
        if (eq == std::string_view::npos) {
            name = url_decode(pair);
        } else {
            name = url_decode(pair.substr(0, eq));
            value = url_decode(pair.substr(eq + 1));
        }
        
        if (!name.empty()) {
            data.add(std::move(name), std::move(value));
        }
        
        start = end + 1;
    }
    
    return data;
}

// ============================================================================
// Multipart Form Parsing
// ============================================================================

namespace {

std::string_view trim(std::string_view s) {
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.front()))) {
        s.remove_prefix(1);
    }
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back()))) {
        s.remove_suffix(1);
    }
    return s;
}

// Parse Content-Disposition header
// Returns: name, filename (if present)
std::pair<std::string, std::string> parse_content_disposition(std::string_view header) {
    std::string name, filename;
    
    // Find name="..."
    auto name_pos = header.find("name=\"");
    if (name_pos != std::string_view::npos) {
        name_pos += 6;
        auto end = header.find('"', name_pos);
        if (end != std::string_view::npos) {
            name = std::string(header.substr(name_pos, end - name_pos));
        }
    }
    
    // Find filename="..."
    auto filename_pos = header.find("filename=\"");
    if (filename_pos != std::string_view::npos) {
        filename_pos += 10;
        auto end = header.find('"', filename_pos);
        if (end != std::string_view::npos) {
            filename = std::string(header.substr(filename_pos, end - filename_pos));
        }
    }
    
    return {name, filename};
}

} // anonymous namespace

expected<FormData, Error> parse_multipart(std::string_view body, std::string_view boundary) {
    FormData data;
    
    if (body.empty() || boundary.empty()) {
        return data;
    }
    
    std::string delimiter = "--" + std::string(boundary);
    std::string end_delimiter = delimiter + "--";
    
    size_t pos = body.find(delimiter);
    if (pos == std::string_view::npos) {
        return unexpected(Error::http(HttpError::BadRequest, "Multipart boundary not found"));
    }
    
    pos += delimiter.size();
    
    while (pos < body.size()) {
        // Skip CRLF after boundary
        if (body.substr(pos, 2) == "\r\n") {
            pos += 2;
        } else if (body.substr(pos, 2) == "--") {
            // End of multipart
            break;
        }
        
        // Find end of headers (double CRLF)
        auto headers_end = body.find("\r\n\r\n", pos);
        if (headers_end == std::string_view::npos) {
            break;
        }
        
        // Parse headers
        auto headers_section = body.substr(pos, headers_end - pos);
        std::string content_disposition;
        std::string content_type;
        
        size_t line_start = 0;
        while (line_start < headers_section.size()) {
            auto line_end = headers_section.find("\r\n", line_start);
            if (line_end == std::string_view::npos) {
                line_end = headers_section.size();
            }
            
            auto line = headers_section.substr(line_start, line_end - line_start);
            auto colon = line.find(':');
            if (colon != std::string_view::npos) {
                auto header_name = trim(line.substr(0, colon));
                auto header_value = trim(line.substr(colon + 1));
                
                // Case-insensitive comparison
                std::string name_lower(header_name);
                std::transform(name_lower.begin(), name_lower.end(), name_lower.begin(),
                              [](unsigned char c) { return std::tolower(c); });
                
                if (name_lower == "content-disposition") {
                    content_disposition = std::string(header_value);
                } else if (name_lower == "content-type") {
                    content_type = std::string(header_value);
                }
            }
            
            line_start = line_end + 2;
        }
        
        // Parse Content-Disposition
        auto [name, filename] = parse_content_disposition(content_disposition);
        
        // Find content (until next boundary)
        pos = headers_end + 4;  // Skip \r\n\r\n
        auto next_boundary = body.find(delimiter, pos);
        if (next_boundary == std::string_view::npos) {
            break;
        }
        
        // Content is everything until \r\n before boundary
        size_t content_end = next_boundary;
        if (content_end >= 2 && body.substr(content_end - 2, 2) == "\r\n") {
            content_end -= 2;
        }
        
        std::string value(body.substr(pos, content_end - pos));
        
        // Create field
        FormField field;
        field.name = std::move(name);
        field.value = std::move(value);
        field.filename = std::move(filename);
        field.content_type = std::move(content_type);
        field.is_file = !field.filename.empty();
        
        if (!field.name.empty()) {
            data.add(std::move(field));
        }
        
        pos = next_boundary + delimiter.size();
    }
    
    return data;
}

std::optional<std::string> extract_boundary(std::string_view content_type) {
    // Look for boundary=...
    auto boundary_pos = content_type.find("boundary=");
    if (boundary_pos == std::string_view::npos) {
        return std::nullopt;
    }
    
    boundary_pos += 9;  // Skip "boundary="
    
    // Check for quoted boundary
    if (boundary_pos < content_type.size() && content_type[boundary_pos] == '"') {
        ++boundary_pos;
        auto end = content_type.find('"', boundary_pos);
        if (end == std::string_view::npos) {
            return std::nullopt;
        }
        return std::string(content_type.substr(boundary_pos, end - boundary_pos));
    }
    
    // Unquoted boundary (ends at ; or end of string)
    auto end = content_type.find(';', boundary_pos);
    if (end == std::string_view::npos) {
        end = content_type.size();
    }
    
    auto boundary = trim(content_type.substr(boundary_pos, end - boundary_pos));
    return std::string(boundary);
}

expected<FormData, Error> parse(const Request& req) {
    auto content_type = req.content_type();
    if (!content_type) {
        return unexpected(Error::http(HttpError::BadRequest, "Missing Content-Type header"));
    }
    
    if (content_type->starts_with("application/x-www-form-urlencoded")) {
        return parse_urlencoded(req.body());
    }
    
    if (content_type->starts_with("multipart/form-data")) {
        auto boundary = extract_boundary(*content_type);
        if (!boundary) {
            return unexpected(Error::http(HttpError::BadRequest, 
                "Missing boundary in multipart Content-Type"));
        }
        return parse_multipart(req.body(), *boundary);
    }
    
    return unexpected(Error::http(HttpError::UnsupportedMediaType,
        "Unsupported form content type"));
}

} // namespace form

} // namespace coroute
