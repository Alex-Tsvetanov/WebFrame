#include "coroute/core/json.hpp"

#include <cctype>
#include <cstring>
#include <charconv>

#ifdef COROUTE_HAS_SIMDJSON
// Thread-local parser for simdjson (parsers are not thread-safe)
static thread_local simdjson::ondemand::parser simdjson_parser;
#endif

namespace coroute {

// ============================================================================
// JSON Serialization
// ============================================================================

namespace {

void escape_string(std::ostream& os, std::string_view str) {
    os << '"';
    for (char c : str) {
        switch (c) {
            case '"':  os << "\\\""; break;
            case '\\': os << "\\\\"; break;
            case '\b': os << "\\b"; break;
            case '\f': os << "\\f"; break;
            case '\n': os << "\\n"; break;
            case '\r': os << "\\r"; break;
            case '\t': os << "\\t"; break;
            default:
                if (static_cast<unsigned char>(c) < 0x20) {
                    os << "\\u" << std::hex << std::setfill('0') << std::setw(4) 
                       << static_cast<int>(static_cast<unsigned char>(c));
                } else {
                    os << c;
                }
        }
    }
    os << '"';
}

void dump_impl(std::ostream& os, const JsonValue& value, int indent, int depth) {
    bool pretty = indent >= 0;
    std::string indent_str = pretty ? std::string(depth * indent, ' ') : "";
    std::string child_indent = pretty ? std::string((depth + 1) * indent, ' ') : "";
    
    if (value.is_null()) {
        os << "null";
    } else if (value.is_bool()) {
        os << (value.as_bool() ? "true" : "false");
    } else if (value.is_number()) {
        double num = value.as_number();
        if (std::isfinite(num)) {
            // Check if it's an integer
            if (num == std::floor(num) && std::abs(num) < 1e15) {
                os << static_cast<int64_t>(num);
            } else {
                os << std::setprecision(17) << num;
            }
        } else {
            os << "null";  // JSON doesn't support inf/nan
        }
    } else if (value.is_string()) {
        escape_string(os, value.as_string());
    } else if (value.is_array()) {
        const auto& arr = value.as_array();
        if (arr.empty()) {
            os << "[]";
        } else {
            os << '[';
            if (pretty) os << '\n';
            for (size_t i = 0; i < arr.size(); ++i) {
                if (pretty) os << child_indent;
                dump_impl(os, arr[i], indent, depth + 1);
                if (i + 1 < arr.size()) os << ',';
                if (pretty) os << '\n';
            }
            if (pretty) os << indent_str;
            os << ']';
        }
    } else if (value.is_object()) {
        const auto& obj = value.as_object();
        if (obj.empty()) {
            os << "{}";
        } else {
            os << '{';
            if (pretty) os << '\n';
            size_t i = 0;
            for (const auto& [key, val] : obj) {
                if (pretty) os << child_indent;
                escape_string(os, key);
                os << ':';
                if (pretty) os << ' ';
                dump_impl(os, val, indent, depth + 1);
                if (++i < obj.size()) os << ',';
                if (pretty) os << '\n';
            }
            if (pretty) os << indent_str;
            os << '}';
        }
    }
}

} // anonymous namespace

std::string JsonValue::dump(int indent) const {
    std::ostringstream os;
    dump_impl(os, *this, indent, 0);
    return os.str();
}

// ============================================================================
// JSON Parsing
// ============================================================================

namespace {

class JsonParser {
    std::string_view input_;
    size_t pos_ = 0;
    
public:
    explicit JsonParser(std::string_view input) : input_(input) {}
    
    expected<JsonValue, Error> parse() {
        skip_whitespace();
        auto result = parse_value();
        if (!result) return result;
        skip_whitespace();
        if (pos_ < input_.size()) {
            return unexpected(Error::http(HttpError::BadRequest, 
                "Unexpected characters after JSON value"));
        }
        return result;
    }

private:
    char peek() const {
        return pos_ < input_.size() ? input_[pos_] : '\0';
    }
    
    char consume() {
        return pos_ < input_.size() ? input_[pos_++] : '\0';
    }
    
    bool consume_if(char c) {
        if (peek() == c) {
            ++pos_;
            return true;
        }
        return false;
    }
    
    bool consume_if(std::string_view s) {
        if (input_.substr(pos_).starts_with(s)) {
            pos_ += s.size();
            return true;
        }
        return false;
    }
    
    void skip_whitespace() {
        while (pos_ < input_.size() && std::isspace(static_cast<unsigned char>(input_[pos_]))) {
            ++pos_;
        }
    }
    
    expected<JsonValue, Error> parse_value() {
        skip_whitespace();
        
        char c = peek();
        if (c == 'n') return parse_null();
        if (c == 't' || c == 'f') return parse_bool();
        if (c == '"') return parse_string();
        if (c == '[') return parse_array();
        if (c == '{') return parse_object();
        if (c == '-' || std::isdigit(static_cast<unsigned char>(c))) return parse_number();
        
        return unexpected(Error::http(HttpError::BadRequest, 
            "Unexpected character in JSON: '" + std::string(1, c) + "'"));
    }
    
    expected<JsonValue, Error> parse_null() {
        if (consume_if("null")) {
            return JsonValue(nullptr);
        }
        return unexpected(Error::http(HttpError::BadRequest, "Expected 'null'"));
    }
    
    expected<JsonValue, Error> parse_bool() {
        if (consume_if("true")) {
            return JsonValue(true);
        }
        if (consume_if("false")) {
            return JsonValue(false);
        }
        return unexpected(Error::http(HttpError::BadRequest, "Expected 'true' or 'false'"));
    }
    
    expected<JsonValue, Error> parse_number() {
        size_t start = pos_;
        
        // Optional minus
        consume_if('-');
        
        // Integer part
        if (peek() == '0') {
            consume();
        } else if (std::isdigit(static_cast<unsigned char>(peek()))) {
            while (std::isdigit(static_cast<unsigned char>(peek()))) {
                consume();
            }
        } else {
            return unexpected(Error::http(HttpError::BadRequest, "Invalid number"));
        }
        
        // Fractional part
        if (peek() == '.') {
            consume();
            if (!std::isdigit(static_cast<unsigned char>(peek()))) {
                return unexpected(Error::http(HttpError::BadRequest, "Invalid number"));
            }
            while (std::isdigit(static_cast<unsigned char>(peek()))) {
                consume();
            }
        }
        
        // Exponent
        if (peek() == 'e' || peek() == 'E') {
            consume();
            consume_if('+') || consume_if('-');
            if (!std::isdigit(static_cast<unsigned char>(peek()))) {
                return unexpected(Error::http(HttpError::BadRequest, "Invalid number exponent"));
            }
            while (std::isdigit(static_cast<unsigned char>(peek()))) {
                consume();
            }
        }
        
        std::string num_str(input_.substr(start, pos_ - start));
        try {
            return JsonValue(std::stod(num_str));
        } catch (...) {
            return unexpected(Error::http(HttpError::BadRequest, "Invalid number: " + num_str));
        }
    }
    
    expected<JsonValue, Error> parse_string() {
        if (!consume_if('"')) {
            return unexpected(Error::http(HttpError::BadRequest, "Expected '\"'"));
        }
        
        std::string result;
        while (peek() != '"') {
            if (peek() == '\0') {
                return unexpected(Error::http(HttpError::BadRequest, "Unterminated string"));
            }
            
            if (peek() == '\\') {
                consume();
                char esc = consume();
                switch (esc) {
                    case '"':  result += '"'; break;
                    case '\\': result += '\\'; break;
                    case '/':  result += '/'; break;
                    case 'b':  result += '\b'; break;
                    case 'f':  result += '\f'; break;
                    case 'n':  result += '\n'; break;
                    case 'r':  result += '\r'; break;
                    case 't':  result += '\t'; break;
                    case 'u': {
                        // Unicode escape
                        if (pos_ + 4 > input_.size()) {
                            return unexpected(Error::http(HttpError::BadRequest, "Invalid unicode escape"));
                        }
                        std::string hex(input_.substr(pos_, 4));
                        pos_ += 4;
                        try {
                            int codepoint = std::stoi(hex, nullptr, 16);
                            if (codepoint < 0x80) {
                                result += static_cast<char>(codepoint);
                            } else if (codepoint < 0x800) {
                                result += static_cast<char>(0xC0 | (codepoint >> 6));
                                result += static_cast<char>(0x80 | (codepoint & 0x3F));
                            } else {
                                result += static_cast<char>(0xE0 | (codepoint >> 12));
                                result += static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
                                result += static_cast<char>(0x80 | (codepoint & 0x3F));
                            }
                        } catch (...) {
                            return unexpected(Error::http(HttpError::BadRequest, "Invalid unicode escape"));
                        }
                        break;
                    }
                    default:
                        return unexpected(Error::http(HttpError::BadRequest, 
                            "Invalid escape sequence: \\" + std::string(1, esc)));
                }
            } else {
                result += consume();
            }
        }
        
        consume();  // closing quote
        return JsonValue(std::move(result));
    }
    
    expected<JsonValue, Error> parse_array() {
        if (!consume_if('[')) {
            return unexpected(Error::http(HttpError::BadRequest, "Expected '['"));
        }
        
        JsonArray arr;
        skip_whitespace();
        
        if (peek() == ']') {
            consume();
            return JsonValue(std::move(arr));
        }
        
        while (true) {
            auto value = parse_value();
            if (!value) return value;
            arr.push_back(std::move(*value));
            
            skip_whitespace();
            if (consume_if(']')) {
                break;
            }
            if (!consume_if(',')) {
                return unexpected(Error::http(HttpError::BadRequest, "Expected ',' or ']' in array"));
            }
        }
        
        return JsonValue(std::move(arr));
    }
    
    expected<JsonValue, Error> parse_object() {
        if (!consume_if('{')) {
            return unexpected(Error::http(HttpError::BadRequest, "Expected '{'"));
        }
        
        JsonObject obj;
        skip_whitespace();
        
        if (peek() == '}') {
            consume();
            return JsonValue(std::move(obj));
        }
        
        while (true) {
            skip_whitespace();
            auto key = parse_string();
            if (!key) return unexpected(key.error());
            
            skip_whitespace();
            if (!consume_if(':')) {
                return unexpected(Error::http(HttpError::BadRequest, "Expected ':' after object key"));
            }
            
            auto value = parse_value();
            if (!value) return value;
            
            obj[key->as_string()] = std::move(*value);
            
            skip_whitespace();
            if (consume_if('}')) {
                break;
            }
            if (!consume_if(',')) {
                return unexpected(Error::http(HttpError::BadRequest, "Expected ',' or '}' in object"));
            }
        }
        
        return JsonValue(std::move(obj));
    }
};

} // anonymous namespace

namespace json {

#ifdef COROUTE_HAS_SIMDJSON

// Convert simdjson value to our JsonValue
static JsonValue convert_simdjson(simdjson::ondemand::value val) {
    switch (val.type()) {
        case simdjson::ondemand::json_type::null:
            return JsonValue(nullptr);
        case simdjson::ondemand::json_type::boolean:
            return JsonValue(bool(val.get_bool()));
        case simdjson::ondemand::json_type::number: {
            // Try integer first, then double
            auto int_result = val.get_int64();
            if (!int_result.error()) {
                return JsonValue(static_cast<int64_t>(int_result.value()));
            }
            return JsonValue(double(val.get_double()));
        }
        case simdjson::ondemand::json_type::string:
            return JsonValue(std::string(val.get_string().value()));
        case simdjson::ondemand::json_type::array: {
            JsonArray arr;
            for (auto element : val.get_array()) {
                arr.push_back(convert_simdjson(element.value()));
            }
            return JsonValue(std::move(arr));
        }
        case simdjson::ondemand::json_type::object: {
            JsonObject obj;
            for (auto field : val.get_object()) {
                std::string key(field.unescaped_key().value());
                obj[key] = convert_simdjson(field.value());
            }
            return JsonValue(std::move(obj));
        }
    }
    return JsonValue(nullptr);
}

expected<JsonValue, Error> parse(std::string_view json) {
    if (json.empty()) {
        return unexpected(Error::http(HttpError::BadRequest, "Empty JSON input"));
    }
    
    // simdjson requires padded input - copy to padded string
    simdjson::padded_string padded(json);
    
    auto doc = simdjson_parser.iterate(padded);
    if (doc.error()) {
        return unexpected(Error::http(HttpError::BadRequest, 
            "JSON parse error: " + std::string(simdjson::error_message(doc.error()))));
    }
    
    try {
        return convert_simdjson(doc.get_value().value());
    } catch (const simdjson::simdjson_error& e) {
        return unexpected(Error::http(HttpError::BadRequest, 
            "JSON parse error: " + std::string(e.what())));
    }
}

#else

// Fallback to custom parser when simdjson is not available
expected<JsonValue, Error> parse(std::string_view json) {
    if (json.empty()) {
        return unexpected(Error::http(HttpError::BadRequest, "Empty JSON input"));
    }
    return JsonParser(json).parse();
}

#endif // coroute_HAS_SIMDJSON

expected<JsonValue, Error> parse(const Request& req) {
    auto content_type = req.content_type();
    if (content_type && !content_type->starts_with("application/json")) {
        return unexpected(Error::http(HttpError::UnsupportedMediaType, 
            "Expected application/json content type"));
    }
    return parse(req.body());
}

std::string stringify(const JsonValue& value, int indent) {
    return value.dump(indent);
}

// Check if simdjson is being used
bool using_simdjson() noexcept {
#ifdef COROUTE_HAS_SIMDJSON
    return true;
#else
    return false;
#endif
}

} // namespace json

} // namespace coroute
