#pragma once

#include <any>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>


#include "coroute/core/error.hpp"
#include "coroute/util/expected.hpp"
#include "coroute/util/from_string.hpp"


namespace coroute {

// ============================================================================
// HTTP Method
// ============================================================================

enum class HttpMethod {
  GET,
  POST,
  PUT,
  DELETE,
  PATCH,
  HEAD,
  OPTIONS,
  CONNECT,
  TRACE,
  UNKNOWN
};

HttpMethod parse_method(std::string_view method) noexcept;
std::string_view method_to_string(HttpMethod method) noexcept;

// ============================================================================
// Request
// ============================================================================

class Request {
public:
  using Headers = std::unordered_map<std::string, std::string>;
  using QueryParams = std::unordered_map<std::string, std::string>;

private:
  HttpMethod method_ = HttpMethod::GET;
  std::string path_;
  std::string query_string_;
  std::string http_version_ = "HTTP/1.1";
  Headers headers_;
  QueryParams query_params_;
  std::string body_;

  // Route parameters (filled by router after matching)
  std::vector<std::string> route_params_;

  // Request context (for middleware to store data)
  mutable std::unordered_map<std::string, std::any> context_;

public:
  Request() = default;

  // Accessors
  HttpMethod method() const noexcept { return method_; }
  std::string_view path() const noexcept { return path_; }
  std::string_view query_string() const noexcept { return query_string_; }
  std::string_view http_version() const noexcept { return http_version_; }
  const Headers &headers() const noexcept { return headers_; }
  std::string_view body() const noexcept { return body_; }

  // Setters (for parser)
  void set_method(HttpMethod m) { method_ = m; }
  void set_method(std::string_view m) { method_ = parse_method(m); }
  void set_path(std::string p) { path_ = std::move(p); }
  void set_query_string(std::string qs) { query_string_ = std::move(qs); }
  void set_http_version(std::string v) { http_version_ = std::move(v); }
  void set_body(std::string b) { body_ = std::move(b); }

  void add_header(std::string key, std::string value) {
    headers_[std::move(key)] = std::move(value);
  }

  void add_query_param(std::string key, std::string value) {
    query_params_[std::move(key)] = std::move(value);
  }

  // Route parameters (set by router)
  void set_route_params(std::vector<std::string> params) {
    route_params_ = std::move(params);
  }

  const std::vector<std::string> &route_params() const noexcept {
    return route_params_;
  }

  const QueryParams &query_params() const noexcept { return query_params_; }

  // Get route parameter by index (0-based, positional)
  template <typename T = std::string>
  expected<T, Error> param(size_t index) const {
    if (index >= route_params_.size()) {
      return unexpected(Error::http(HttpError::BadRequest,
                                    "Route parameter index out of range: " +
                                        std::to_string(index)));
    }
    return from_string<T>(route_params_[index]);
  }

  // Get header value
  std::optional<std::string_view> header(std::string_view key) const {
    // Case-insensitive lookup would be better, but keeping simple for now
    auto it = headers_.find(std::string(key));
    if (it != headers_.end()) {
      return it->second;
    }
    return std::nullopt;
  }

  // Get query parameter
  template <typename T = std::string>
  expected<T, Error> query(std::string_view key) const {
    auto it = query_params_.find(std::string(key));
    if (it == query_params_.end()) {
      return unexpected(
          Error::http(HttpError::BadRequest,
                      "Missing query parameter: " + std::string(key)));
    }
    return from_string<T>(it->second);
  }

  // Get optional query parameter
  template <typename T = std::string>
  std::optional<T> query_opt(std::string_view key) const {
    auto it = query_params_.find(std::string(key));
    if (it == query_params_.end()) {
      return std::nullopt;
    }
    auto result = from_string<T>(it->second);
    if (result) {
      return std::move(*result);
    }
    return std::nullopt;
  }

  // Check if connection should be kept alive
  bool keep_alive() const noexcept {
    auto conn = header("Connection");
    if (conn) {
      // Case-insensitive comparison would be better
      if (*conn == "close")
        return false;
      if (*conn == "keep-alive")
        return true;
    }
    // HTTP/1.1 defaults to keep-alive
    return http_version_ == "HTTP/1.1";
  }

  // Content length
  std::optional<size_t> content_length() const {
    auto cl = header("Content-Length");
    if (cl) {
      auto result = from_string<size_t>(*cl);
      if (result)
        return *result;
    }
    return std::nullopt;
  }

  // Content type
  std::optional<std::string_view> content_type() const {
    return header("Content-Type");
  }

  // Context storage (for middleware)
  template <typename T>
  void set_context(const std::string &key, T value) const {
    context_[key] = std::move(value);
  }

  template <typename T>
  std::optional<T> get_context(const std::string &key) const {
    auto it = context_.find(key);
    if (it == context_.end())
      return std::nullopt;
    try {
      return std::any_cast<T>(it->second);
    } catch (const std::bad_any_cast &) {
      return std::nullopt;
    }
  }

  bool has_context(const std::string &key) const {
    return context_.count(key) > 0;
  }

  // Reset for object pooling
  void reset() {
    method_ = HttpMethod::GET;
    path_.clear();
    query_string_.clear();
    http_version_ = "HTTP/1.1";
    headers_.clear();
    query_params_.clear();
    body_.clear();
    route_params_.clear();
    context_.clear();
  }
};

} // namespace coroute
