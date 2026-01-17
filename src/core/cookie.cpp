#include "coroute/core/cookie.hpp"

#include <sstream>
#include <iomanip>
#include <cctype>

namespace coroute {

// ============================================================================
// Cookie Implementation
// ============================================================================

namespace {

std::string format_http_date(std::chrono::system_clock::time_point tp) {
    auto time_t_val = std::chrono::system_clock::to_time_t(tp);
    std::ostringstream oss;
    oss << std::put_time(std::gmtime(&time_t_val), "%a, %d %b %Y %H:%M:%S GMT");
    return oss.str();
}

std::string_view trim(std::string_view s) {
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.front()))) {
        s.remove_prefix(1);
    }
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back()))) {
        s.remove_suffix(1);
    }
    return s;
}

} // anonymous namespace

std::string Cookie::to_header() const {
    std::ostringstream oss;
    
    // Name=Value
    oss << name << "=" << value;
    
    // Domain
    if (domain) {
        oss << "; Domain=" << *domain;
    }
    
    // Path
    if (path) {
        oss << "; Path=" << *path;
    }
    
    // Max-Age
    if (max_age) {
        oss << "; Max-Age=" << max_age->count();
    }
    
    // Expires
    if (expires) {
        oss << "; Expires=" << format_http_date(*expires);
    }
    
    // Secure
    if (secure) {
        oss << "; Secure";
    }
    
    // HttpOnly
    if (http_only) {
        oss << "; HttpOnly";
    }
    
    // SameSite
    switch (same_site) {
        case SameSite::None:
            oss << "; SameSite=None";
            break;
        case SameSite::Lax:
            oss << "; SameSite=Lax";
            break;
        case SameSite::Strict:
            oss << "; SameSite=Strict";
            break;
    }
    
    return oss.str();
}

Cookie Cookie::expired(std::string name) {
    Cookie c;
    c.name = std::move(name);
    c.value = "";
    c.max_age = std::chrono::seconds(0);
    c.expires = std::chrono::system_clock::time_point{};  // Unix epoch
    return c;
}

// ============================================================================
// CookieJar Implementation
// ============================================================================

CookieJar CookieJar::parse(std::string_view header) {
    CookieJar jar;
    
    size_t start = 0;
    while (start < header.size()) {
        // Find end of this cookie (semicolon or end)
        size_t end = header.find(';', start);
        if (end == std::string_view::npos) {
            end = header.size();
        }
        
        auto pair = trim(header.substr(start, end - start));
        
        // Find = separator
        auto eq = pair.find('=');
        if (eq != std::string_view::npos) {
            auto name = trim(pair.substr(0, eq));
            auto value = trim(pair.substr(eq + 1));
            
            // Remove quotes if present
            if (value.size() >= 2 && value.front() == '"' && value.back() == '"') {
                value = value.substr(1, value.size() - 2);
            }
            
            if (!name.empty()) {
                jar.cookies_[std::string(name)] = std::string(value);
            }
        }
        
        start = end + 1;
    }
    
    return jar;
}

CookieJar CookieJar::from_request(const Request& req) {
    auto cookie_header = req.header("Cookie");
    if (!cookie_header) {
        return CookieJar{};
    }
    return parse(*cookie_header);
}

std::optional<std::string_view> CookieJar::get(std::string_view name) const {
    auto it = cookies_.find(std::string(name));
    if (it == cookies_.end()) {
        return std::nullopt;
    }
    return it->second;
}

bool CookieJar::has(std::string_view name) const {
    return cookies_.count(std::string(name)) > 0;
}

void CookieJar::set(std::string name, std::string value) {
    cookies_[std::move(name)] = std::move(value);
}

// ============================================================================
// Response Helpers
// ============================================================================

void set_cookie(Response& resp, const Cookie& cookie) {
    resp.add_header("Set-Cookie", cookie.to_header());
}

void set_cookies(Response& resp, const std::vector<Cookie>& cookies) {
    for (const auto& cookie : cookies) {
        set_cookie(resp, cookie);
    }
}

void delete_cookie(Response& resp, std::string_view name,
                   std::string_view path,
                   std::string_view domain) {
    Cookie c = Cookie::expired(std::string(name));
    if (!path.empty()) {
        c.path = std::string(path);
    }
    if (!domain.empty()) {
        c.domain = std::string(domain);
    }
    set_cookie(resp, c);
}

} // namespace coroute
