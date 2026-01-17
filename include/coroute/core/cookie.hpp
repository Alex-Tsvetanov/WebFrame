#pragma once

#include <string>
#include <string_view>
#include <optional>
#include <unordered_map>
#include <chrono>
#include <vector>

#include "coroute/core/request.hpp"
#include "coroute/core/response.hpp"

namespace coroute {

// ============================================================================
// Cookie SameSite Policy
// ============================================================================

enum class SameSite {
    None,       // Cookie sent with all requests (requires Secure)
    Lax,        // Cookie sent with top-level navigations and GET from third-party
    Strict      // Cookie only sent with same-site requests
};

// ============================================================================
// Cookie (for setting)
// ============================================================================

struct Cookie {
    std::string name;
    std::string value;
    
    // Optional attributes
    std::optional<std::string> domain = std::nullopt;
    std::optional<std::string> path = std::nullopt;
    std::optional<std::chrono::seconds> max_age = std::nullopt;
    std::optional<std::chrono::system_clock::time_point> expires = std::nullopt;
    bool secure = false;
    bool http_only = false;
    SameSite same_site = SameSite::Lax;
    
    // Fluent setters
    Cookie& set_domain(std::string d) { domain = std::move(d); return *this; }
    Cookie& set_path(std::string p) { path = std::move(p); return *this; }
    Cookie& set_max_age(std::chrono::seconds age) { max_age = age; return *this; }
    Cookie& set_expires(std::chrono::system_clock::time_point exp) { expires = exp; return *this; }
    Cookie& set_secure(bool s = true) { secure = s; return *this; }
    Cookie& set_http_only(bool h = true) { http_only = h; return *this; }
    Cookie& set_same_site(SameSite ss) { same_site = ss; return *this; }
    
    // Serialize to Set-Cookie header value
    std::string to_header() const;
    
    // Create a cookie that expires immediately (for deletion)
    static Cookie expired(std::string name);
};

// ============================================================================
// Cookie Jar (parsed cookies from request)
// ============================================================================

class CookieJar {
    std::unordered_map<std::string, std::string> cookies_;
    
public:
    CookieJar() = default;
    
    // Parse from Cookie header value
    static CookieJar parse(std::string_view header);
    
    // Parse from request
    static CookieJar from_request(const Request& req);
    
    // Get cookie value
    std::optional<std::string_view> get(std::string_view name) const;
    
    // Check if cookie exists
    bool has(std::string_view name) const;
    
    // Get all cookies
    const std::unordered_map<std::string, std::string>& all() const { return cookies_; }
    
    // Number of cookies
    size_t size() const { return cookies_.size(); }
    bool empty() const { return cookies_.empty(); }
    
    // Add cookie (for building)
    void set(std::string name, std::string value);
};

// ============================================================================
// Response Cookie Helpers
// ============================================================================

// Add Set-Cookie header to response
void set_cookie(Response& resp, const Cookie& cookie);

// Add multiple cookies
void set_cookies(Response& resp, const std::vector<Cookie>& cookies);

// Delete a cookie (sets expired cookie)
void delete_cookie(Response& resp, std::string_view name, 
                   std::string_view path = "/",
                   std::string_view domain = "");

// ============================================================================
// Convenience Functions
// ============================================================================

// Parse cookies from request
inline CookieJar cookies(const Request& req) {
    return CookieJar::from_request(req);
}

// Create a simple cookie
inline Cookie cookie(std::string name, std::string value) {
    return Cookie{std::move(name), std::move(value)};
}

// Create a secure session cookie
inline Cookie session_cookie(std::string name, std::string value) {
    Cookie c{std::move(name), std::move(value)};
    c.http_only = true;
    c.secure = true;
    c.same_site = SameSite::Strict;
    c.path = "/";
    return c;
}

} // namespace coroute
