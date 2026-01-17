#pragma once

#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "coroute/core/request.hpp"
#include "coroute/core/response.hpp"
#include "coroute/coro/task.hpp"

namespace coroute {

// ============================================================================
// AuthState - Authentication state propagation interface
// ============================================================================

/// Abstract interface for authentication state management.
/// Represents authentication state propagation, NOT sessions or cookies.
///
/// Views (especially desktop/mobile) use this to automatically carry
/// authentication state when making app.fetch() calls to API routes.
class AuthState {
public:
  virtual ~AuthState() = default;

  /// Attach authentication info to an outgoing request.
  /// Called by App::fetch() before dispatching.
  virtual void apply(Request &req) = 0;

  /// Observe a response and update internal auth state.
  /// Called by App::fetch() after receiving response.
  /// Use to learn from Set-Cookie headers or token responses.
  virtual void observe(const Response &resp) = 0;

  /// Whether the client is currently authenticated.
  virtual bool authenticated() const = 0;

  /// Clear authentication state (logout).
  virtual void clear() = 0;
};

// ============================================================================
// WebAuthState - No-op implementation for web/browser
// ============================================================================

/// No-op AuthState for web platform.
/// Browser already manages cookies automatically.
/// Server-side rendering sees auth via Request headers.
class WebAuthState : public AuthState {
public:
  void apply(Request & /*req*/) override {
    // No-op: browser handles cookies automatically
  }

  void observe(const Response & /*resp*/) override {
    // No-op: browser handles Set-Cookie automatically
  }

  bool authenticated() const override {
    // On web, we don't track this client-side
    // Auth state comes from the incoming Request
    return false;
  }

  void clear() override {
    // No-op: browser cookie would need to be cleared via Set-Cookie
  }
};

// ============================================================================
// ClientAuthState - Token/cookie management for desktop/mobile
// ============================================================================

/// AuthState implementation for desktop and mobile clients.
/// Stores cookies and/or tokens internally, applying them to outgoing
/// requests and learning from Set-Cookie headers in responses.
class ClientAuthState : public AuthState {
public:
  void apply(Request &req) override {
    // Add stored cookies to request
    if (!cookies_.empty()) {
      std::string cookie_header;
      for (const auto &[name, value] : cookies_) {
        if (!cookie_header.empty()) {
          cookie_header += "; ";
        }
        cookie_header += name + "=" + value;
      }
      req.add_header("Cookie", cookie_header);
    }

    // Add stored bearer token if present
    if (bearer_token_) {
      req.add_header("Authorization", "Bearer " + *bearer_token_);
    }
  }

  void observe(const Response &resp) override {
    // Learn from Set-Cookie headers
    for (const auto &[key, value] : resp.headers()) {
      if (key == "Set-Cookie") {
        parse_set_cookie(value);
      }
    }
  }

  bool authenticated() const override {
    return bearer_token_.has_value() || !cookies_.empty();
  }

  void clear() override {
    cookies_.clear();
    bearer_token_.reset();
  }

  // ========================================================================
  // Additional API for managing auth state
  // ========================================================================

  /// Manually set a bearer token.
  void set_bearer_token(std::string token) { bearer_token_ = std::move(token); }

  /// Get the current bearer token.
  const std::optional<std::string> &bearer_token() const {
    return bearer_token_;
  }

  /// Manually set a cookie.
  void set_cookie(const std::string &name, const std::string &value) {
    cookies_[name] = value;
  }

  /// Get a stored cookie.
  std::optional<std::string> get_cookie(const std::string &name) const {
    auto it = cookies_.find(name);
    if (it != cookies_.end()) {
      return it->second;
    }
    return std::nullopt;
  }

  /// Get all stored cookies.
  const std::unordered_map<std::string, std::string> &cookies() const {
    return cookies_;
  }

private:
  /// Parse Set-Cookie header and store cookie.
  void parse_set_cookie(const std::string &header) {
    // Simple parsing: extract name=value before first semicolon
    auto semi_pos = header.find(';');
    std::string name_value =
        (semi_pos != std::string::npos) ? header.substr(0, semi_pos) : header;

    auto eq_pos = name_value.find('=');
    if (eq_pos != std::string::npos) {
      std::string name = name_value.substr(0, eq_pos);
      std::string value = name_value.substr(eq_pos + 1);

      // Trim whitespace
      while (!name.empty() && name.front() == ' ')
        name.erase(0, 1);
      while (!name.empty() && name.back() == ' ')
        name.pop_back();
      while (!value.empty() && value.front() == ' ')
        value.erase(0, 1);
      while (!value.empty() && value.back() == ' ')
        value.pop_back();

      if (!name.empty()) {
        cookies_[name] = value;
      }
    }
  }

  std::unordered_map<std::string, std::string> cookies_;
  std::optional<std::string> bearer_token_;
};

// ============================================================================
// FetchTransport - Interface for fetch transport
// ============================================================================

class FetchTransport {
public:
  virtual ~FetchTransport() = default;

  /// Dispatch a request and return a response.
  /// Implementations may route in-process or over HTTP.
  virtual Task<Response> dispatch(Request &req) = 0;
};

} // namespace coroute
