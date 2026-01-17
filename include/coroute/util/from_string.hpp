#pragma once

#include <string>
#include <string_view>
#include <charconv>
#include <sstream>
#include <optional>
#include <type_traits>

#include "coroute/util/expected.hpp"
#include "coroute/core/error.hpp"

namespace coroute {

// ============================================================================
// FromString trait - Convert string to type T
// ============================================================================

template<typename T, typename = void>
struct FromString {
    // Default implementation using operator>>
    static expected<T, Error> parse(std::string_view s) {
        T value;
        std::istringstream iss{std::string(s)};
        if (iss >> value) {
            // Check that we consumed the entire string
            if (iss.eof() || (iss >> std::ws).eof()) {
                return value;
            }
        }
        return unexpected(Error::http(HttpError::BadRequest, 
            "Failed to parse parameter: " + std::string(s)));
    }
};

// ============================================================================
// Specializations for integral types (using std::from_chars for performance)
// ============================================================================

template<typename T>
struct FromString<T, std::enable_if_t<std::is_integral_v<T> && !std::is_same_v<T, bool>>> {
    static expected<T, Error> parse(std::string_view s) {
        if (s.empty()) {
            return unexpected(Error::http(HttpError::BadRequest, "Empty parameter"));
        }
        
        T value;
        auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), value);
        
        if (ec == std::errc{} && ptr == s.data() + s.size()) {
            return value;
        }
        
        if (ec == std::errc::result_out_of_range) {
            return unexpected(Error::http(HttpError::BadRequest, 
                "Parameter out of range: " + std::string(s)));
        }
        
        return unexpected(Error::http(HttpError::BadRequest, 
            "Invalid integer: " + std::string(s)));
    }
};

// ============================================================================
// Specialization for floating point types
// ============================================================================

template<typename T>
struct FromString<T, std::enable_if_t<std::is_floating_point_v<T>>> {
    static expected<T, Error> parse(std::string_view s) {
        if (s.empty()) {
            return unexpected(Error::http(HttpError::BadRequest, "Empty parameter"));
        }
        
        // std::from_chars for floating point may not be available on all compilers
        // Fall back to stringstream for portability
        T value;
        std::istringstream iss{std::string(s)};
        iss >> value;
        
        if (!iss.fail() && (iss.eof() || (iss >> std::ws).eof())) {
            return value;
        }
        
        return unexpected(Error::http(HttpError::BadRequest, 
            "Invalid number: " + std::string(s)));
    }
};

// ============================================================================
// Specialization for bool
// ============================================================================

template<>
struct FromString<bool> {
    static expected<bool, Error> parse(std::string_view s) {
        if (s == "true" || s == "1" || s == "yes" || s == "on") {
            return true;
        }
        if (s == "false" || s == "0" || s == "no" || s == "off") {
            return false;
        }
        return unexpected(Error::http(HttpError::BadRequest, 
            "Invalid boolean: " + std::string(s)));
    }
};

// ============================================================================
// Specialization for std::string (identity)
// ============================================================================

template<>
struct FromString<std::string> {
    static expected<std::string, Error> parse(std::string_view s) {
        return std::string(s);
    }
};

// ============================================================================
// Specialization for std::string_view (identity, but careful with lifetime!)
// ============================================================================

template<>
struct FromString<std::string_view> {
    static expected<std::string_view, Error> parse(std::string_view s) {
        return s;
    }
};

// ============================================================================
// Specialization for std::optional<T>
// ============================================================================

template<typename T>
struct FromString<std::optional<T>> {
    static expected<std::optional<T>, Error> parse(std::string_view s) {
        if (s.empty()) {
            return std::optional<T>{std::nullopt};
        }
        auto result = FromString<T>::parse(s);
        if (result) {
            return std::optional<T>{std::move(*result)};
        }
        return unexpected(result.error());
    }
};

// ============================================================================
// Helper function
// ============================================================================

template<typename T>
expected<T, Error> from_string(std::string_view s) {
    return FromString<T>::parse(s);
}

// ============================================================================
// Concept for types that can be parsed from string
// ============================================================================

template<typename T>
concept Parseable = requires(std::string_view s) {
    { FromString<T>::parse(s) } -> std::same_as<expected<T, Error>>;
};

} // namespace coroute
