#pragma once

#include <any>
#include <atomic>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <memory>
#include <mutex>
#include <sstream>
#include <unordered_map>
#include <vector>

#include "coroute/core/auth_state.hpp"
#include "coroute/core/request.hpp"
#include "coroute/core/response.hpp"
#include "coroute/core/router.hpp"
#include "coroute/coro/cancellation.hpp"
#include "coroute/coro/task.hpp"
#include "coroute/net/io_context.hpp"
#include "coroute/net/websocket.hpp"
#include "coroute/util/object_pool.hpp"

#ifdef COROUTE_HAS_TLS
#include "coroute/net/tls.hpp"
#endif

#ifdef COROUTE_HAS_HTTP2
#include "coroute/http2/connection.hpp"
#endif

#ifdef COROUTE_HAS_TEMPLATES
#include "coroute/view/view_middleware.hpp"
#include "coroute/view/view_types.hpp"
#include <inja/inja.hpp>

#endif

namespace coroute {

// ============================================================================
// Middleware Types
// ============================================================================

// Next function type - call to continue to next middleware/handler
using Next = std::function<Task<Response>(Request &)>;

// Middleware function type - receives request and next function
using Middleware = std::function<Task<Response>(Request &, Next)>;

// ============================================================================
// App - Main application class
// ============================================================================

// Shutdown options
struct ShutdownOptions {
  std::chrono::seconds drain_timeout{
      30}; // Time to wait for connections to drain
  bool force_close_after_timeout =
      true; // Force close remaining connections after timeout
};

// Pre-compiled middleware chain - built once, executed many times
class CompiledMiddlewareChain {
  std::vector<Middleware> middleware_;
  bool compiled_ = false;

public:
  void add(Middleware mw) {
    middleware_.push_back(std::move(mw));
    compiled_ = false;
  }

  bool empty() const noexcept { return middleware_.empty(); }
  size_t size() const noexcept { return middleware_.size(); }

  // Execute the chain with a final handler
  Task<Response> execute(Request &req, Handler &handler) const {
    if (middleware_.empty()) {
      co_return co_await handler(req);
    }

    // Build chain from innermost (handler) to outermost (first middleware)
    // We capture by index to avoid lambda capture issues
    co_return co_await execute_at(0, req, handler);
  }

  // Execute with a not-found fallback
  Task<Response> execute_or_not_found(Request &req,
                                      const Handler *handler) const {
    if (middleware_.empty()) {
      if (handler) {
        co_return co_await (*handler)(req);
      }
      co_return Response::not_found();
    }

    co_return co_await execute_at_or_not_found(0, req, handler);
  }

private:
  Task<Response> execute_at(size_t idx, Request &req,
                            const Handler &handler) const {
    if (idx >= middleware_.size()) {
      co_return co_await handler(req);
    }

    // Create next function that continues the chain
    Next next = [this, idx, &handler](Request &r) -> Task<Response> {
      return execute_at(idx + 1, r, handler);
    };

    co_return co_await middleware_[idx](req, next);
  }

  Task<Response> execute_at_or_not_found(size_t idx, Request &req,
                                         const Handler *handler) const {
    if (idx >= middleware_.size()) {
      if (handler) {
        co_return co_await (*handler)(req);
      }
      co_return Response::not_found();
    }

    // Create next function that continues the chain
    Next next = [this, idx, handler](Request &r) -> Task<Response> {
      return execute_at_or_not_found(idx + 1, r, handler);
    };

    co_return co_await middleware_[idx](req, next);
  }
};

// TLS configuration for App
struct AppTlsConfig {
  std::filesystem::path cert_file;
  std::filesystem::path key_file;
  std::filesystem::path ca_file;    // Optional: for client cert verification
  std::filesystem::path chain_file; // Optional: certificate chain
  bool verify_client = false;
  std::vector<std::string> alpn_protocols; // e.g., {"h2", "http/1.1"}
};

class App {
  Router router_;
  std::unique_ptr<net::IoContext> io_ctx_;
  std::unique_ptr<net::Listener> listener_;
  CancellationSource cancel_source_;
  size_t thread_count_ = 1;
  CompiledMiddlewareChain middleware_chain_;

  // Connection tracking for graceful shutdown
  std::atomic<size_t> active_connections_{0};
  std::atomic<bool> shutting_down_{false};

  // Object pools for reduced allocations
  mutable BufferPool buffer_pool_{8192, 256};

  // TLS support
#ifdef COROUTE_HAS_TLS
  std::unique_ptr<net::TlsContext> tls_ctx_;
  std::unique_ptr<net::TlsListener> tls_listener_;
#endif
  bool tls_enabled_ = false;

  // HTTP/2 support
#ifdef COROUTE_HAS_HTTP2
  bool http2_enabled_ = true; // Enable by default when available
#endif

  // WebSocket handlers
  std::unordered_map<std::string, WebSocketHandler> ws_handlers_;

  // Authentication state (for fetch API)
  std::unique_ptr<AuthState> auth_state_;

#ifdef COROUTE_CLIENT_MODE
  // Transport for client-mode fetch (HTTP requests to remote server)
  std::unique_ptr<FetchTransport> fetch_transport_;
  std::string fetch_base_url_;
#endif

  // Template engine
#ifdef COROUTE_HAS_TEMPLATES
  std::unique_ptr<inja::Environment> template_env_;
  std::filesystem::path template_dir_{"templates"};
  bool template_caching_{true};
  std::unordered_map<std::string, std::string> template_cache_;
  mutable std::mutex template_mutex_;

  // View middleware (global, runs for all views)
  ViewMiddlewareChain global_view_middleware_;
#endif

public:
  App() = default;
  ~App() = default;

  // Non-copyable, non-movable (due to atomics)
  App(const App &) = delete;
  App &operator=(const App &) = delete;
  App(App &&) = delete;
  App &operator=(App &&) = delete;

  // Configuration
  App &threads(size_t count) {
    thread_count_ = count;
    return *this;
  }

  // Route registration (simple form)
  App &route(HttpMethod method, std::string pattern, Handler handler) {
    router_.add(method, std::move(pattern), std::move(handler));
    return *this;
  }

  // Convenience methods
  App &get(std::string pattern, Handler handler) {
    return route(HttpMethod::GET, std::move(pattern), std::move(handler));
  }

  App &post(std::string pattern, Handler handler) {
    return route(HttpMethod::POST, std::move(pattern), std::move(handler));
  }

  App &put(std::string pattern, Handler handler) {
    return route(HttpMethod::PUT, std::move(pattern), std::move(handler));
  }

  App &del(std::string pattern, Handler handler) {
    return route(HttpMethod::DELETE, std::move(pattern), std::move(handler));
  }

  // Route registration with automatic parameter extraction
  template <typename... Args, typename F>
    requires std::invocable<F, Args..., Request &>
  App &get(std::string pattern, F &&handler) {
    router_.get<Args...>(std::move(pattern), std::forward<F>(handler));
    return *this;
  }

  template <typename... Args, typename F>
    requires std::invocable<F, Args..., Request &>
  App &post(std::string pattern, F &&handler) {
    router_.post<Args...>(std::move(pattern), std::forward<F>(handler));
    return *this;
  }

  template <typename... Args, typename F>
    requires std::invocable<F, Args..., Request &>
  App &put(std::string pattern, F &&handler) {
    router_.put<Args...>(std::move(pattern), std::forward<F>(handler));
    return *this;
  }

  template <typename... Args, typename F>
    requires std::invocable<F, Args..., Request &>
  App &del(std::string pattern, F &&handler) {
    router_.del<Args...>(std::move(pattern), std::forward<F>(handler));
    return *this;
  }

  // Access router directly
  Router &router() noexcept { return router_; }
  const Router &router() const noexcept { return router_; }

  // WebSocket route registration
  App &ws(std::string path, WebSocketHandler handler) {
    ws_handlers_[std::move(path)] = std::move(handler);
    return *this;
  }

  // ========================================================================
  // Fetch API (In-Process Request Dispatch)
  // ========================================================================

  /// Set the authentication state manager.
  /// For web: use WebAuthState (no-op).
  /// For desktop/mobile: use ClientAuthState (manages cookies/tokens).
  App &set_auth_state(std::unique_ptr<AuthState> auth) {
    auth_state_ = std::move(auth);
    return *this;
  }

  /// Access the current auth state (may be null).
  AuthState *auth_state() noexcept { return auth_state_.get(); }
  const AuthState *auth_state() const noexcept { return auth_state_.get(); }

#ifdef COROUTE_CLIENT_MODE
  /// Set the fetch transport for client mode.
  /// Required for desktop/mobile builds to make HTTP requests.
  App &set_fetch_transport(std::unique_ptr<FetchTransport> transport,
                           std::string base_url = "") {
    fetch_transport_ = std::move(transport);
    fetch_base_url_ = std::move(base_url);
    return *this;
  }

  /// Get the base URL for fetch requests.
  const std::string &fetch_base_url() const noexcept { return fetch_base_url_; }
#endif

  /// Fetch from an internal route with automatic auth propagation.
  ///
  /// Semantics:
  /// 1. Build request from route and body
  /// 2. auth_state->apply(request) if auth_state set
  /// 3. Dispatch through middleware chain (web) or HTTP transport (client)
  /// 4. auth_state->observe(response) if auth_state set
  /// 5. Return response
  ///
  /// Auth failures return normal 401/403 Responses (no exceptions).
  /// Only throws for true invariants/transport failures.
  Task<Response> fetch(HttpMethod method, std::string_view route,
                       std::string body = "") {
    // Build request
    Request req;
    req.set_method(method);
    req.set_path(std::string(route));
    req.set_body(std::move(body));
    req.set_http_version("HTTP/1.1");

    // Apply auth state (add cookies, tokens, etc.)
    if (auth_state_) {
      auth_state_->apply(req);
    }

#ifdef COROUTE_CLIENT_MODE
    // Client mode: dispatch via HTTP transport to remote server
    if (!fetch_transport_) {
      throw std::runtime_error(
          "fetch() called in client mode without transport configured");
    }
    Response resp = co_await fetch_transport_->dispatch(req);
#else
    // Web/server mode: dispatch in-process through middleware chain
    auto match = router_.match(method, route);
    if (match) {
      req.set_route_params(std::move(match.params));
    }
    Response resp =
        co_await middleware_chain_.execute_or_not_found(req, match.handler);
#endif

    // Observe response for auth state updates
    if (auth_state_) {
      auth_state_->observe(resp);
    }

    co_return resp;
  }

  /// Convenience methods for fetch
  Task<Response> fetch_get(std::string_view route) {
    return fetch(HttpMethod::GET, route);
  }

  Task<Response> fetch_post(std::string_view route, std::string body = "") {
    return fetch(HttpMethod::POST, route, std::move(body));
  }

  Task<Response> fetch_put(std::string_view route, std::string body = "") {
    return fetch(HttpMethod::PUT, route, std::move(body));
  }

  Task<Response> fetch_delete(std::string_view route) {
    return fetch(HttpMethod::DELETE, route);
  }

  // ========================================================================
  // Fetch with Original Request (for web/server cookie forwarding)
  // ========================================================================

  /// Fetch with cookie/header forwarding from original request.
  /// Use this in web/server mode to propagate browser cookies to API calls.
  Task<Response> fetch(const Request &original, HttpMethod method,
                       std::string_view route, std::string body = "") {
    // Build request
    Request req;
    req.set_method(method);
    req.set_path(std::string(route));
    req.set_body(std::move(body));
    req.set_http_version("HTTP/1.1");

    // Forward cookies from original request (web/server mode)
    auto cookie = original.header("Cookie");
    if (cookie) {
      req.add_header("Cookie", std::string(*cookie));
    }

    // Forward authorization header if present
    auto auth = original.header("Authorization");
    if (auth) {
      req.add_header("Authorization", std::string(*auth));
    }

    // Apply auth state (may override with stored tokens for client mode)
    if (auth_state_) {
      auth_state_->apply(req);
    }

#ifdef COROUTE_CLIENT_MODE
    if (!fetch_transport_) {
      throw std::runtime_error(
          "fetch() called in client mode without transport configured");
    }
    Response resp = co_await fetch_transport_->dispatch(req);
#else
    auto match = router_.match(method, route);
    if (match) {
      req.set_route_params(std::move(match.params));
    }
    Response resp =
        co_await middleware_chain_.execute_or_not_found(req, match.handler);
#endif

    if (auth_state_) {
      auth_state_->observe(resp);
    }

    co_return resp;
  }

  /// Convenience methods with original request forwarding
  Task<Response> fetch_get(const Request &original, std::string_view route) {
    return fetch(original, HttpMethod::GET, route);
  }

  Task<Response> fetch_post(const Request &original, std::string_view route,
                            std::string body = "") {
    return fetch(original, HttpMethod::POST, route, std::move(body));
  }

  // ========================================================================
  // View Route Registration & Middleware
  // ========================================================================
#ifdef COROUTE_HAS_TEMPLATES

  /// Add global view middleware (runs for all views, UI-level concerns only).
  App &use_view(ViewMiddleware mw) {
    global_view_middleware_.add(std::move(mw));
    return *this;
  }

  /// Register a view route with typed ViewModel.
  /// Handler signature: (Request&) -> View<VM>
  template <typename VM, typename Handler>
    requires requires(Handler h, Request &r) {
      { h(r) } -> std::same_as<View<VM>>;
    }
  App &view(std::string_view path, Handler &&handler) {
    std::string path_str(path);
    auto wrapper = [this, path_str, h = std::forward<Handler>(handler)](
                       Request &req) mutable -> Task<ViewResultAny> {
      // Run global view middleware
      ViewExecutionContext ctx{.route = path_str, .view_name = ""};
      co_await global_view_middleware_.execute(ctx);

      // Call handler
      ViewResult<VM> result = co_await h(req);
      co_return ViewResultAny(std::move(result));
    };
    router_.add_view(std::string(path), std::move(wrapper));
    return *this;
  }

  /// Register a view route with context access.
  /// Handler signature: (Request&, ViewExecutionContext&) -> View<VM>
  template <typename VM, typename Handler>
    requires requires(Handler h, Request &r, ViewExecutionContext &ctx) {
      { h(r, ctx) } -> std::same_as<View<VM>>;
    }
  App &view(std::string_view path, Handler &&handler) {
    std::string path_str(path);
    auto wrapper = [this, path_str, h = std::forward<Handler>(handler)](
                       Request &req) mutable -> Task<ViewResultAny> {
      // Run global view middleware
      ViewExecutionContext ctx{.route = path_str, .view_name = ""};
      co_await global_view_middleware_.execute(ctx);

      // Call handler with context
      ViewResult<VM> result = co_await h(req, ctx);
      co_return ViewResultAny(std::move(result));
    };
    router_.add_view(std::string(path), std::move(wrapper));
    return *this;
  }

  /// Register a view route with per-route middleware.
  template <typename VM, typename Handler>
    requires requires(Handler h, Request &r, ViewExecutionContext &ctx) {
      { h(r, ctx) } -> std::same_as<View<VM>>;
    }
  App &view(std::string_view path, std::vector<ViewMiddleware> per_route_mw,
            Handler &&handler) {
    std::string path_str(path);
    auto wrapper = [this, path_str, per_mw = std::move(per_route_mw),
                    h = std::forward<Handler>(handler)](
                       Request &req) mutable -> Task<ViewResultAny> {
      // Run global view middleware
      ViewExecutionContext ctx{.route = path_str, .view_name = ""};
      co_await global_view_middleware_.execute(ctx);

      // Run per-route middleware
      for (const auto &mw : per_mw) {
        co_await mw(ctx);
      }

      // Call handler with context
      ViewResult<VM> result = co_await h(req, ctx);
      co_return ViewResultAny(std::move(result));
    };
    router_.add_view(std::string(path), std::move(wrapper));
    return *this;
  }

#endif // COROUTE_HAS_TEMPLATES

  // Middleware registration
  // Middleware is called in order: first registered = outermost (runs first on
  // request, last on response)
  App &use(Middleware middleware) {
    middleware_chain_.add(std::move(middleware));
    return *this;
  }

  // Run the server (blocking)
  void run(uint16_t port);

  // Run the server (async)
  Task<void> run_async(uint16_t port);

  // Enable TLS/HTTPS
#ifdef COROUTE_HAS_TLS
  App &enable_tls(const AppTlsConfig &config);
  bool tls_enabled() const noexcept { return tls_enabled_; }
#endif

  // HTTP/2 configuration
#ifdef COROUTE_HAS_HTTP2
  App &enable_http2(bool enable = true) {
    http2_enabled_ = enable;
    return *this;
  }
  bool http2_enabled() const noexcept { return http2_enabled_; }
#endif

  // ========================================================================
  // Template Engine (Jinja2-style via inja)
  // ========================================================================
#ifdef COROUTE_HAS_TEMPLATES
  // Set the template directory
  App &set_templates(const std::filesystem::path &dir) {
    template_dir_ = dir;
    if (!template_env_) {
      template_env_ = std::make_unique<inja::Environment>();
    }
    template_env_->set_search_included_templates_in_files(true);
    return *this;
  }

  // Enable/disable template caching
  App &set_template_caching(bool enabled) {
    template_caching_ = enabled;
    return *this;
  }

  // Render a template string with data
  std::string render(std::string_view template_str,
                     const nlohmann::json &data) {
    ensure_template_env();
    return template_env_->render(std::string(template_str), data);
  }

  // Render a template file with data (like v1's app.render())
  std::string render(const std::string &filename, const nlohmann::json &data) {
    ensure_template_env();

    if (template_caching_) {
      std::lock_guard lock(template_mutex_);
      auto it = template_cache_.find(filename);
      if (it != template_cache_.end()) {
        return template_env_->render(it->second, data);
      }
    }

    auto path = template_dir_ / filename;
    auto tmpl = template_env_->parse_template(path.string());
    std::string result = template_env_->render(tmpl, data);

    if (template_caching_) {
      std::lock_guard lock(template_mutex_);
      // Cache the template content
      std::ifstream file(path);
      if (file) {
        std::ostringstream ss;
        ss << file.rdbuf();
        template_cache_[filename] = ss.str();
      }
    }

    return result;
  }

  // Render template to Response
  Response render_html(const std::string &filename,
                       const nlohmann::json &data) {
    return Response::html(render(filename, data));
  }

  // Add custom template callback
  void add_template_callback(
      const std::string &name, int num_args,
      const std::function<nlohmann::json(inja::Arguments &)> &callback) {
    ensure_template_env();
    template_env_->add_callback(name, num_args, callback);
  }

  // Clear template cache
  void clear_template_cache() {
    std::lock_guard lock(template_mutex_);
    template_cache_.clear();
  }

  // Access inja environment for advanced configuration
  inja::Environment &template_env() {
    ensure_template_env();
    return *template_env_;
  }

  // Access template directory for view renderer
  const std::filesystem::path &template_dir() const noexcept {
    return template_dir_;
  }

private:
  void ensure_template_env() {
    if (!template_env_) {
      template_env_ = std::make_unique<inja::Environment>();
    }
  }

public:
#endif // coroute_HAS_TEMPLATES

  // Stop the server (immediate)
  void stop();

  // Graceful shutdown - stops accepting, waits for connections to drain
  void shutdown(ShutdownOptions options = {});

  // Check if server is shutting down
  bool is_shutting_down() const {
    return shutting_down_.load(std::memory_order_relaxed);
  }

  // Get active connection count
  size_t active_connections() const {
    return active_connections_.load(std::memory_order_relaxed);
  }

  // Get cancellation token for graceful shutdown
  CancellationToken cancellation_token() const {
    return cancel_source_.token();
  }

private:
  // Handle a single connection
  Task<void> handle_connection(std::unique_ptr<net::Connection> conn);

  // Parse HTTP request from connection
  Task<expected<Request, Error>> parse_request(net::Connection &conn);

  // Check if request is a WebSocket upgrade and handle it
  // Returns true if handled as WebSocket, false if should continue as HTTP
  Task<bool> try_websocket_upgrade(std::unique_ptr<net::Connection> &conn,
                                   Request &req);

  // Check if request is an HTTP/2 upgrade (h2c) and handle it
  // Returns true if handled as HTTP/2, false if should continue as HTTP/1.1
#ifdef COROUTE_HAS_HTTP2
  Task<bool> try_http2_upgrade(std::unique_ptr<net::Connection> &conn,
                               Request &req);

  // Handle HTTP/2 connection
  Task<void>
  handle_http2_connection(std::shared_ptr<http2::Http2Connection> h2_conn);
#endif
};

} // namespace coroute
