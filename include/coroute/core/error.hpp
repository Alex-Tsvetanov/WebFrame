#pragma once

#include <variant>
#include <string>
#include <string_view>
#include <system_error>

namespace coroute {

// ============================================================================
// I/O Errors (transport layer)
// ============================================================================

enum class IoError {
    Success = 0,
    ConnectionReset,
    ConnectionRefused,
    ConnectionAborted,
    Timeout,
    Cancelled,
    EndOfStream,
    WouldBlock,
    AddressInUse,
    AddressNotAvailable,
    NetworkUnreachable,
    HostUnreachable,
    InvalidArgument,
    PermissionDenied,
    Unknown
};

// ============================================================================
// HTTP Errors (protocol layer)
// ============================================================================

enum class HttpError {
    // 4xx Client Errors
    BadRequest = 400,
    Unauthorized = 401,
    Forbidden = 403,
    NotFound = 404,
    MethodNotAllowed = 405,
    RequestTimeout = 408,
    PayloadTooLarge = 413,
    UriTooLong = 414,
    UnsupportedMediaType = 415,
    TooManyRequests = 429,
    
    // 5xx Server Errors
    Internal = 500,
    NotImplemented = 501,
    BadGateway = 502,
    ServiceUnavailable = 503,
    GatewayTimeout = 504
};

} // namespace coroute

// Enable std::error_code integration - MUST be before make_error_code declarations
template<>
struct std::is_error_code_enum<coroute::IoError> : std::true_type {};

template<>
struct std::is_error_code_enum<coroute::HttpError> : std::true_type {};

namespace coroute {

// Now declare make_error_code after the enum specializations
const std::error_category& io_error_category() noexcept;
std::error_code make_error_code(IoError e) noexcept;

const std::error_category& http_error_category() noexcept;
std::error_code make_error_code(HttpError e) noexcept;

// ============================================================================
// Unified Error Type
// ============================================================================

class Error {
public:
    using Variant = std::variant<IoError, HttpError, std::error_code>;

private:
    Variant inner_;
    std::string message_;

public:
    // Constructors
    Error() : inner_(IoError::Success) {}
    
    Error(IoError e, std::string message = "")
        : inner_(e), message_(std::move(message)) {}
    
    Error(HttpError e, std::string message = "")
        : inner_(e), message_(std::move(message)) {}
    
    Error(std::error_code ec, std::string message = "")
        : inner_(ec), message_(std::move(message)) {}

    // Factory methods
    static Error io(IoError e, std::string msg = "") {
        return Error(e, std::move(msg));
    }
    
    static Error http(HttpError e, std::string msg = "") {
        return Error(e, std::move(msg));
    }
    
    static Error http(int status, std::string msg = "") {
        return Error(static_cast<HttpError>(status), std::move(msg));
    }
    
    static Error system(std::error_code ec) {
        return Error(ec);
    }
    
    static Error cancelled() {
        return Error(IoError::Cancelled, "Operation cancelled");
    }
    
    static Error timeout() {
        return Error(IoError::Timeout, "Operation timed out");
    }

    // Type checks
    bool is_io() const noexcept {
        return std::holds_alternative<IoError>(inner_);
    }
    
    bool is_http() const noexcept {
        return std::holds_alternative<HttpError>(inner_);
    }
    
    bool is_system() const noexcept {
        return std::holds_alternative<std::error_code>(inner_);
    }
    
    bool is_cancelled() const noexcept {
        return is_io() && std::get<IoError>(inner_) == IoError::Cancelled;
    }
    
    bool is_timeout() const noexcept {
        return is_io() && std::get<IoError>(inner_) == IoError::Timeout;
    }

    // Accessors
    IoError io_error() const noexcept {
        return is_io() ? std::get<IoError>(inner_) : IoError::Unknown;
    }
    
    HttpError http_error() const noexcept {
        return is_http() ? std::get<HttpError>(inner_) : HttpError::Internal;
    }
    
    std::error_code system_error() const noexcept {
        return is_system() ? std::get<std::error_code>(inner_) : std::error_code{};
    }

    // Convert to std::error_code (for interop)
    std::error_code code() const noexcept;

    // HTTP status code (returns 500 for non-HTTP errors)
    int http_status() const noexcept {
        if (is_http()) {
            return static_cast<int>(std::get<HttpError>(inner_));
        }
        if (is_io()) {
            auto e = std::get<IoError>(inner_);
            switch (e) {
                case IoError::Timeout: return 408;
                case IoError::Cancelled: return 499; // Client Closed Request
                default: return 500;
            }
        }
        return 500;
    }

    // Human-readable message
    std::string_view message() const noexcept {
        return message_;
    }
    
    // Full description
    std::string to_string() const;

    // Comparison
    bool operator==(const Error& other) const noexcept {
        return inner_ == other.inner_;
    }
    
    bool operator!=(const Error& other) const noexcept {
        return !(*this == other);
    }

    // Boolean conversion (true if error, false if success)
    explicit operator bool() const noexcept {
        if (is_io()) return std::get<IoError>(inner_) != IoError::Success;
        if (is_http()) return true; // HTTP errors are always errors
        if (is_system()) return static_cast<bool>(std::get<std::error_code>(inner_));
        return false;
    }
};

} // namespace coroute
