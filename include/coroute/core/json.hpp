#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <variant>
#include <optional>
#include <stdexcept>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <cstdint>

#ifdef COROUTE_HAS_SIMDJSON
#include <simdjson.h>
#endif

#include "coroute/core/request.hpp"
#include "coroute/core/response.hpp"
#include "coroute/core/error.hpp"
#include "coroute/util/expected.hpp"

namespace coroute {

// ============================================================================
// JSON Value Type
// ============================================================================

class JsonValue;

using JsonNull = std::nullptr_t;
using JsonBool = bool;
using JsonNumber = double;
using JsonString = std::string;
using JsonArray = std::vector<JsonValue>;
using JsonObject = std::unordered_map<std::string, JsonValue>;

class JsonValue {
public:
    using Variant = std::variant<JsonNull, JsonBool, JsonNumber, JsonString, JsonArray, JsonObject>;
    
private:
    Variant value_;

public:
    // Constructors
    JsonValue() : value_(nullptr) {}
    JsonValue(std::nullptr_t) : value_(nullptr) {}
    JsonValue(bool b) : value_(b) {}
    JsonValue(int i) : value_(static_cast<double>(i)) {}
    JsonValue(int64_t i) : value_(static_cast<double>(i)) {}
    JsonValue(double d) : value_(d) {}
    JsonValue(const char* s) : value_(std::string(s)) {}
    JsonValue(std::string s) : value_(std::move(s)) {}
    JsonValue(std::string_view s) : value_(std::string(s)) {}
    JsonValue(JsonArray arr) : value_(std::move(arr)) {}
    JsonValue(JsonObject obj) : value_(std::move(obj)) {}
    
    // Initializer list constructors
    JsonValue(std::initializer_list<JsonValue> list) : value_(JsonArray(list)) {}
    JsonValue(std::initializer_list<std::pair<const std::string, JsonValue>> list) 
        : value_(JsonObject(list.begin(), list.end())) {}
    
    // Type checks
    bool is_null() const { return std::holds_alternative<JsonNull>(value_); }
    bool is_bool() const { return std::holds_alternative<JsonBool>(value_); }
    bool is_number() const { return std::holds_alternative<JsonNumber>(value_); }
    bool is_string() const { return std::holds_alternative<JsonString>(value_); }
    bool is_array() const { return std::holds_alternative<JsonArray>(value_); }
    bool is_object() const { return std::holds_alternative<JsonObject>(value_); }
    
    // Accessors (throw on type mismatch)
    bool as_bool() const { return std::get<JsonBool>(value_); }
    double as_number() const { return std::get<JsonNumber>(value_); }
    int64_t as_int() const { return static_cast<int64_t>(std::get<JsonNumber>(value_)); }
    const std::string& as_string() const { return std::get<JsonString>(value_); }
    const JsonArray& as_array() const { return std::get<JsonArray>(value_); }
    const JsonObject& as_object() const { return std::get<JsonObject>(value_); }
    
    JsonArray& as_array() { return std::get<JsonArray>(value_); }
    JsonObject& as_object() { return std::get<JsonObject>(value_); }
    
    // Safe accessors (return optional)
    std::optional<bool> get_bool() const {
        if (is_bool()) return as_bool();
        return std::nullopt;
    }
    std::optional<double> get_number() const {
        if (is_number()) return as_number();
        return std::nullopt;
    }
    std::optional<std::string_view> get_string() const {
        if (is_string()) return as_string();
        return std::nullopt;
    }
    
    // Object access
    JsonValue& operator[](const std::string& key) {
        if (!is_object()) {
            value_ = JsonObject{};
        }
        return std::get<JsonObject>(value_)[key];
    }
    
    const JsonValue* get(const std::string& key) const {
        if (!is_object()) return nullptr;
        auto& obj = std::get<JsonObject>(value_);
        auto it = obj.find(key);
        return it != obj.end() ? &it->second : nullptr;
    }
    
    bool contains(const std::string& key) const {
        if (!is_object()) return false;
        return std::get<JsonObject>(value_).count(key) > 0;
    }
    
    // Array access
    JsonValue& operator[](size_t index) {
        return std::get<JsonArray>(value_)[index];
    }
    
    const JsonValue& operator[](size_t index) const {
        return std::get<JsonArray>(value_)[index];
    }
    
    size_t size() const {
        if (is_array()) return std::get<JsonArray>(value_).size();
        if (is_object()) return std::get<JsonObject>(value_).size();
        return 0;
    }
    
    // Array operations
    void push_back(JsonValue val) {
        if (!is_array()) {
            value_ = JsonArray{};
        }
        std::get<JsonArray>(value_).push_back(std::move(val));
    }
    
    // Serialization
    std::string dump(int indent = -1) const;
    
    // Comparison
    bool operator==(const JsonValue& other) const { return value_ == other.value_; }
    bool operator!=(const JsonValue& other) const { return value_ != other.value_; }
};

// ============================================================================
// JSON Parsing
// ============================================================================

namespace json {

// Parse JSON string
expected<JsonValue, Error> parse(std::string_view json);

// Parse JSON from request body
expected<JsonValue, Error> parse(const Request& req);

// Stringify JSON value
std::string stringify(const JsonValue& value, int indent = -1);

// Pretty print (indent = 2)
inline std::string pretty(const JsonValue& value) {
    return stringify(value, 2);
}

// Check if simdjson is being used for parsing
bool using_simdjson() noexcept;

} // namespace json

// ============================================================================
// Request/Response JSON Helpers
// ============================================================================

// Parse request body as JSON
inline expected<JsonValue, Error> parse_json_body(const Request& req) {
    return json::parse(req);
}

// Create JSON response from JsonValue
inline Response json_response(const JsonValue& value) {
    return Response::ok(value.dump(), "application/json");
}

// Create JSON response with status
inline Response json_response(int status, const JsonValue& value) {
    Response resp(status, {}, value.dump());
    resp.set_header("Content-Type", "application/json");
    resp.set_header("Content-Length", std::to_string(resp.body().size()));
    return resp;
}

// ============================================================================
// JSON Builder (Fluent API)
// ============================================================================

class JsonBuilder {
    JsonValue root_;
    
public:
    JsonBuilder() : root_(JsonObject{}) {}
    
    JsonBuilder& set(const std::string& key, JsonValue value) {
        root_[key] = std::move(value);
        return *this;
    }
    
    JsonBuilder& set(const std::string& key, const char* value) {
        root_[key] = std::string(value);
        return *this;
    }
    
    template<typename T>
    JsonBuilder& set(const std::string& key, T value) {
        root_[key] = JsonValue(value);
        return *this;
    }
    
    JsonValue build() { return std::move(root_); }
    
    operator JsonValue() { return build(); }
};

inline JsonBuilder json_object() {
    return JsonBuilder();
}

inline JsonArray json_array(std::initializer_list<JsonValue> values) {
    return JsonArray(values);
}

} // namespace coroute
