#include "coroute/core/error.hpp"
#include <sstream>

namespace coroute {

// ============================================================================
// IoError Category
// ============================================================================

namespace {

class IoErrorCategory : public std::error_category {
public:
    const char* name() const noexcept override {
        return "coroute.io";
    }
    
    std::string message(int ev) const override {
        switch (static_cast<IoError>(ev)) {
            case IoError::Success: return "Success";
            case IoError::ConnectionReset: return "Connection reset";
            case IoError::ConnectionRefused: return "Connection refused";
            case IoError::ConnectionAborted: return "Connection aborted";
            case IoError::Timeout: return "Operation timed out";
            case IoError::Cancelled: return "Operation cancelled";
            case IoError::EndOfStream: return "End of stream";
            case IoError::WouldBlock: return "Operation would block";
            case IoError::AddressInUse: return "Address already in use";
            case IoError::AddressNotAvailable: return "Address not available";
            case IoError::NetworkUnreachable: return "Network unreachable";
            case IoError::HostUnreachable: return "Host unreachable";
            case IoError::InvalidArgument: return "Invalid argument";
            case IoError::PermissionDenied: return "Permission denied";
            case IoError::Unknown: return "Unknown error";
            default: return "Unknown I/O error";
        }
    }
};

class HttpErrorCategory : public std::error_category {
public:
    const char* name() const noexcept override {
        return "coroute.http";
    }
    
    std::string message(int ev) const override {
        switch (static_cast<HttpError>(ev)) {
            case HttpError::BadRequest: return "Bad Request";
            case HttpError::Unauthorized: return "Unauthorized";
            case HttpError::Forbidden: return "Forbidden";
            case HttpError::NotFound: return "Not Found";
            case HttpError::MethodNotAllowed: return "Method Not Allowed";
            case HttpError::RequestTimeout: return "Request Timeout";
            case HttpError::PayloadTooLarge: return "Payload Too Large";
            case HttpError::UriTooLong: return "URI Too Long";
            case HttpError::UnsupportedMediaType: return "Unsupported Media Type";
            case HttpError::TooManyRequests: return "Too Many Requests";
            case HttpError::Internal: return "Internal Server Error";
            case HttpError::NotImplemented: return "Not Implemented";
            case HttpError::BadGateway: return "Bad Gateway";
            case HttpError::ServiceUnavailable: return "Service Unavailable";
            case HttpError::GatewayTimeout: return "Gateway Timeout";
            default: return "Unknown HTTP error";
        }
    }
};

const IoErrorCategory io_category_instance{};
const HttpErrorCategory http_category_instance{};

} // anonymous namespace

const std::error_category& io_error_category() noexcept {
    return io_category_instance;
}

const std::error_category& http_error_category() noexcept {
    return http_category_instance;
}

std::error_code make_error_code(IoError e) noexcept {
    return {static_cast<int>(e), io_error_category()};
}

std::error_code make_error_code(HttpError e) noexcept {
    return {static_cast<int>(e), http_error_category()};
}

// ============================================================================
// Error Implementation
// ============================================================================

std::error_code Error::code() const noexcept {
    if (is_io()) {
        return make_error_code(std::get<IoError>(inner_));
    }
    if (is_http()) {
        return make_error_code(std::get<HttpError>(inner_));
    }
    if (is_system()) {
        return std::get<std::error_code>(inner_);
    }
    return {};
}

std::string Error::to_string() const {
    std::ostringstream oss;
    
    if (is_io()) {
        oss << "IoError::" << io_error_category().message(static_cast<int>(std::get<IoError>(inner_)));
    } else if (is_http()) {
        auto e = std::get<HttpError>(inner_);
        oss << "HttpError::" << static_cast<int>(e) << " " << http_error_category().message(static_cast<int>(e));
    } else if (is_system()) {
        auto ec = std::get<std::error_code>(inner_);
        oss << "SystemError::" << ec.category().name() << ":" << ec.value() << " " << ec.message();
    }
    
    if (!message_.empty()) {
        oss << " - " << message_;
    }
    
    return oss.str();
}

} // namespace coroute
