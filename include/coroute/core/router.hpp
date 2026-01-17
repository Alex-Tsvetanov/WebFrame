#pragma once

#include <functional>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>


#include <matcher/core.hpp>

#include "coroute/core/request.hpp"
#include "coroute/core/response.hpp"
#include "coroute/coro/task.hpp"
#include "coroute/util/from_string.hpp" // For extract_param

// Forward declaration for view types
namespace coroute {
struct ViewResultAny;
struct ViewTemplates;
} // namespace coroute

namespace coroute {

// ============================================================================
// Handler Types
// ============================================================================

// Basic handler: takes Request, returns Task<Response>
using Handler = std::function<Task<Response>(Request &)>;

// Handler with ResponseBuilder (for middleware pre-population)
using HandlerWithBuilder =
    std::function<Task<Response>(Request &, ResponseBuilder)>;

// View handler: takes Request, returns Task<ViewResultAny>
using ViewHandler = std::function<Task<ViewResultAny>(Request &)>;

// ============================================================================
// RouteInfo - Stored route information
// ============================================================================

struct RouteInfo {
  std::string pattern; // Original pattern like "/user/{id}"
  Handler handler;
  HttpMethod method = HttpMethod::GET;
  std::vector<std::string> param_names; // Parameter names in order
};

// ============================================================================
// ViewRouteInfo - Stored view route information (GET only)
// ============================================================================

struct ViewRouteInfo {
  std::string pattern; // Original pattern like "/user/{id}"
  ViewHandler handler;
  std::vector<std::string> param_names; // Parameter names in order
  ViewTemplates *templates = nullptr;   // Optional: for validation
};

// ============================================================================
// Router - Route collection and matching using DFA-based RegexMatcher
// ============================================================================

class Router {
  // Store route info indexed by route ID
  std::vector<RouteInfo> routes_;

  // Store view route info (GET only)
  std::vector<ViewRouteInfo> view_routes_;

  // DFA-based matcher for each HTTP method (for fast O(n) matching where n =
  // path length)
  matcher::RegexMatcher<size_t> get_matcher_;
  matcher::RegexMatcher<size_t> post_matcher_;
  matcher::RegexMatcher<size_t> put_matcher_;
  matcher::RegexMatcher<size_t> delete_matcher_;
  matcher::RegexMatcher<size_t> patch_matcher_;
  matcher::RegexMatcher<size_t> head_matcher_;
  matcher::RegexMatcher<size_t> options_matcher_;

  // View route matcher (GET only)
  matcher::RegexMatcher<size_t> view_matcher_;

public:
  Router() = default;

  // Add route with explicit method
  void add(HttpMethod method, std::string pattern, Handler handler);

  // Convenience methods
  void get(std::string pattern, Handler handler) {
    add(HttpMethod::GET, std::move(pattern), std::move(handler));
  }

  void post(std::string pattern, Handler handler) {
    add(HttpMethod::POST, std::move(pattern), std::move(handler));
  }

  void put(std::string pattern, Handler handler) {
    add(HttpMethod::PUT, std::move(pattern), std::move(handler));
  }

  void del(std::string pattern, Handler handler) {
    add(HttpMethod::DELETE, std::move(pattern), std::move(handler));
  }

  void patch(std::string pattern, Handler handler) {
    add(HttpMethod::PATCH, std::move(pattern), std::move(handler));
  }

  // Match a request and return handler + extracted params
  struct MatchResult {
    const Handler *handler = nullptr;
    std::vector<std::string> params;

    explicit operator bool() const noexcept { return handler != nullptr; }
  };

  MatchResult match(HttpMethod method, std::string_view path);

  // ========================================================================
  // View Routes (GET only)
  // ========================================================================

  /// Add a view route (GET method only)
  void add_view(std::string pattern, ViewHandler handler);

  /// Match result for view routes
  struct ViewMatchResult {
    const ViewHandler *handler = nullptr;
    std::vector<std::string> params;

    explicit operator bool() const noexcept { return handler != nullptr; }
  };

  /// Match a view route (checks GET method only)
  ViewMatchResult match_view(std::string_view path);

  /// Get all registered view routes (for template validation)
  const std::vector<ViewRouteInfo> &view_routes() const noexcept {
    return view_routes_;
  }

private:
  // Get the matcher for a given HTTP method
  matcher::RegexMatcher<size_t> &get_matcher_for(HttpMethod method);
  const matcher::RegexMatcher<size_t> &get_matcher_for(HttpMethod method) const;

  // Convert user pattern "/user/{id}" to url-matcher pattern "/user/(.+)"
  static std::pair<std::string, std::vector<std::string>>
  convert_pattern(std::string_view pattern);

public:
  // Route with automatic parameter extraction
  // Parameters are extracted positionally from the route pattern
  template <typename... Args, typename F>
    requires std::invocable<F, Args..., Request &>
  void route(HttpMethod method, std::string pattern, F &&handler) {
    add(method, std::move(pattern),
        make_handler<Args...>(std::forward<F>(handler)));
  }

  template <typename... Args, typename F>
    requires std::invocable<F, Args..., Request &>
  void get(std::string pattern, F &&handler) {
    route<Args...>(HttpMethod::GET, std::move(pattern),
                   std::forward<F>(handler));
  }

  template <typename... Args, typename F>
    requires std::invocable<F, Args..., Request &>
  void post(std::string pattern, F &&handler) {
    route<Args...>(HttpMethod::POST, std::move(pattern),
                   std::forward<F>(handler));
  }

  template <typename... Args, typename F>
    requires std::invocable<F, Args..., Request &>
  void put(std::string pattern, F &&handler) {
    route<Args...>(HttpMethod::PUT, std::move(pattern),
                   std::forward<F>(handler));
  }

  template <typename... Args, typename F>
    requires std::invocable<F, Args..., Request &>
  void del(std::string pattern, F &&handler) {
    route<Args...>(HttpMethod::DELETE, std::move(pattern),
                   std::forward<F>(handler));
  }

private:
  // Create a handler that extracts parameters and calls the user function
  template <typename... Args, typename F> static Handler make_handler(F &&f) {
    return [func = std::forward<F>(f)](Request &req) mutable -> Task<Response> {
      return invoke_with_params<Args...>(func, req,
                                         std::index_sequence_for<Args...>{});
    };
  }

  template <typename... Args, typename F, size_t... Is>
  static Task<Response> invoke_with_params(F &func, Request &req,
                                           std::index_sequence<Is...>) {
    if constexpr (sizeof...(Args) == 0) {
      // No parameters to extract
      co_return co_await func(req);
    } else {
      // Extract each parameter from route_params
      auto params = std::make_tuple(extract_param<Args>(req, Is)...);

      // Check if any extraction failed
      if (!all_valid(std::get<Is>(params)...)) {
        co_return Response::bad_request("Invalid route parameters");
      }

      // Call the handler with extracted values
      co_return co_await func(*std::get<Is>(params)..., req);
    }
  }

  template <typename T>
  static expected<T, Error> extract_param(Request &req, size_t index) {
    return req.param<T>(index);
  }

  template <typename... Ts>
  static bool all_valid(const expected<Ts, Error> &...results) {
    return (results.has_value() && ...);
  }
};

} // namespace coroute
