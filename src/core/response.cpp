#include "coroute/core/response.hpp"
#include <sstream>

namespace coroute {

std::string Response::serialize() const {
    std::ostringstream oss;
    
    // Status line
    oss << "HTTP/1.1 " << status_ << " " << status_text_ << "\r\n";
    
    // Headers
    for (const auto& [key, value] : headers_) {
        oss << key << ": " << value << "\r\n";
    }
    
    // Empty line separating headers from body
    oss << "\r\n";
    
    // Body
    oss << body_;
    
    return oss.str();
}

std::string Response::serialize_headers() const {
    std::ostringstream oss;
    
    // Status line
    oss << "HTTP/1.1 " << status_ << " " << status_text_ << "\r\n";
    
    // Headers
    for (const auto& [key, value] : headers_) {
        oss << key << ": " << value << "\r\n";
    }
    
    // Empty line separating headers from body
    oss << "\r\n";
    
    return oss.str();
}

std::string_view Response::default_status_text(int status) noexcept {
    switch (status) {
        // 1xx Informational
        case 100: return "Continue";
        case 101: return "Switching Protocols";
        
        // 2xx Success
        case 200: return "OK";
        case 201: return "Created";
        case 202: return "Accepted";
        case 204: return "No Content";
        
        // 3xx Redirection
        case 301: return "Moved Permanently";
        case 302: return "Found";
        case 303: return "See Other";
        case 304: return "Not Modified";
        case 307: return "Temporary Redirect";
        case 308: return "Permanent Redirect";
        
        // 4xx Client Errors
        case 400: return "Bad Request";
        case 401: return "Unauthorized";
        case 403: return "Forbidden";
        case 404: return "Not Found";
        case 405: return "Method Not Allowed";
        case 408: return "Request Timeout";
        case 409: return "Conflict";
        case 410: return "Gone";
        case 413: return "Payload Too Large";
        case 414: return "URI Too Long";
        case 415: return "Unsupported Media Type";
        case 422: return "Unprocessable Entity";
        case 429: return "Too Many Requests";
        
        // 5xx Server Errors
        case 500: return "Internal Server Error";
        case 501: return "Not Implemented";
        case 502: return "Bad Gateway";
        case 503: return "Service Unavailable";
        case 504: return "Gateway Timeout";
        
        default: return "Unknown";
    }
}

} // namespace coroute
