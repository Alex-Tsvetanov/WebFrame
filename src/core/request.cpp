#include "coroute/core/request.hpp"
#include <algorithm>
#include <cctype>

namespace coroute {

HttpMethod parse_method(std::string_view method) noexcept {
    if (method == "GET") return HttpMethod::GET;
    if (method == "POST") return HttpMethod::POST;
    if (method == "PUT") return HttpMethod::PUT;
    if (method == "DELETE") return HttpMethod::DELETE;
    if (method == "PATCH") return HttpMethod::PATCH;
    if (method == "HEAD") return HttpMethod::HEAD;
    if (method == "OPTIONS") return HttpMethod::OPTIONS;
    if (method == "CONNECT") return HttpMethod::CONNECT;
    if (method == "TRACE") return HttpMethod::TRACE;
    return HttpMethod::UNKNOWN;
}

std::string_view method_to_string(HttpMethod method) noexcept {
    switch (method) {
        case HttpMethod::GET: return "GET";
        case HttpMethod::POST: return "POST";
        case HttpMethod::PUT: return "PUT";
        case HttpMethod::DELETE: return "DELETE";
        case HttpMethod::PATCH: return "PATCH";
        case HttpMethod::HEAD: return "HEAD";
        case HttpMethod::OPTIONS: return "OPTIONS";
        case HttpMethod::CONNECT: return "CONNECT";
        case HttpMethod::TRACE: return "TRACE";
        default: return "UNKNOWN";
    }
}

} // namespace coroute
