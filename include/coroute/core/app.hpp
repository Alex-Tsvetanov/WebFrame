#pragma once

#include <any>
#include <chrono>
#include <atomic>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <memory>
#include <mutex>
#include <optional>
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
#include "coroute/view/deferred.hpp"
#include "coroute/view/deferred_stream.hpp"
#include "coroute/view/view_middleware.hpp"
#include "coroute/view/view_types.hpp"
#include <inja/inja.hpp>

#endif

namespace coroute
{

	namespace http3
	{
		// Forward declared on purpose. Naming the type here would pull ngtcp2 and
		// nghttp3 into every translation unit that includes app.hpp, for the sake of one
		// pointer.
		class Http3EndpointGroup;
		struct Http3Stats;
	}  // namespace http3

	namespace net
	{
		// Same reasoning. The classification window's deadline is an implementation
		// detail of the accept path, and only the two coroutines that share the window
		// name the type. io_context.hpp above already declares everything else in net
		// that this header needs.
		class Deadline;
	}  // namespace net


	// ============================================================================
	// Middleware Types
	// ============================================================================

	// Next function type - call to continue to next middleware/handler
	using Next = std::function<Task<Response>(Request&)>;

	// Middleware function type - receives request and next function
	using Middleware = std::function<Task<Response>(Request&, Next)>;

	// ============================================================================
	// App - Main application class
	// ============================================================================

	// Shutdown options
	struct ShutdownOptions
	{
		std::chrono::seconds drain_timeout{30};  // Time to wait for connections to drain
		bool force_close_after_timeout = true;   // Force close remaining connections after timeout
	};

	// Pre-compiled middleware chain - built once, executed many times
	class CompiledMiddlewareChain
	{
		std::vector<Middleware> middleware_;
		bool compiled_ = false;

	public:
		void add(Middleware mw)
		{
			middleware_.push_back(std::move(mw));
			compiled_ = false;
		}

		bool empty() const noexcept { return middleware_.empty(); }
		size_t size() const noexcept { return middleware_.size(); }

		// Execute the chain with a final handler
		Task<Response> execute(Request& req, const Handler& handler) const
		{
			if (middleware_.empty())
			{
				co_return co_await handler(req);
			}

			// Build chain from innermost (handler) to outermost (first middleware)
			// We capture by index to avoid lambda capture issues
			co_return co_await execute_at(0, req, handler);
		}

		// Execute with a not-found fallback
		Task<Response> execute_or_not_found(Request& req, const Handler* handler) const
		{
			if (middleware_.empty())
			{
				if (handler)
				{
					co_return co_await (*handler)(req);
				}
				co_return Response::not_found();
			}

			co_return co_await execute_at_or_not_found(0, req, handler);
		}

	private:
		Task<Response> execute_at(size_t idx, Request& req, const Handler& handler) const
		{
			if (idx >= middleware_.size())
			{
				co_return co_await handler(req);
			}

			// Create next function that continues the chain
			Next next = [this, idx, &handler](Request& r) -> Task<Response> { return execute_at(idx + 1, r, handler); };

			co_return co_await middleware_[idx](req, next);
		}

		Task<Response> execute_at_or_not_found(size_t idx, Request& req, const Handler* handler) const
		{
			if (idx >= middleware_.size())
			{
				if (handler)
				{
					co_return co_await (*handler)(req);
				}
				co_return Response::not_found();
			}

			// Create next function that continues the chain
			Next next = [this, idx, handler](Request& r) -> Task<Response>
			{ return execute_at_or_not_found(idx + 1, r, handler); };

			co_return co_await middleware_[idx](req, next);
		}
	};

	// TLS configuration for App
	struct AppTlsConfig
	{
		std::filesystem::path cert_file;
		std::filesystem::path key_file;
		std::filesystem::path ca_file;     // Optional: for client cert verification
		std::filesystem::path chain_file;  // Optional: certificate chain
		bool verify_client = false;
		std::vector<std::string> alpn_protocols;  // e.g., {"h2", "http/1.1"}
	};

	class App
	{
		Router router_;
		std::unique_ptr<net::IoContext> io_ctx_;
		std::unique_ptr<net::Listener> listener_;
		CancellationSource cancel_source_;
		size_t thread_count_ = 1;
		CompiledMiddlewareChain middleware_chain_;

		// Which event loop run() will build. See io_backend().
		net::IoBackend io_backend_ = net::IoBackend::Default;

		// See backlog() and enable_protocol_detection() for why these are here.
		int backlog_ = 1024;
		bool protocol_detection_ = true;

		// How long a peer may take to reveal which protocol it speaks. Matches the
		// 30 s the backends already default Connection::set_timeout to, which is the
		// nearest thing to a precedent in this codebase. Zero switches it off.
		std::chrono::milliseconds handshake_timeout_{30000};

		// How long an established HTTP/1.1 connection may sit without a byte moving.
		// Enforced by net::IdleTimeout, since no backend enforces set_timeout. Zero
		// switches it off, which restores the behaviour this codebase had while the
		// timeout was configured and ignored.
		std::chrono::milliseconds keep_alive_timeout_{30000};

		// How many requests one connection may carry before the server closes it.
		// Zero means no limit. A limit is a defence against a client holding a
		// connection forever, and it is also a variable a benchmark has to control,
		// because forcing a reconnect changes what is being measured.
		std::size_t max_requests_per_connection_ = 100;

		// Connection tracking for graceful shutdown
		std::atomic<size_t> active_connections_{0};
		std::atomic<bool> shutting_down_{false};

		// Object pools for reduced allocations
		mutable BufferPool buffer_pool_{8192, 256};

		// TLS support
#ifdef COROUTE_HAS_TLS
		std::unique_ptr<net::TlsContext> tls_ctx_;
#endif
		bool tls_enabled_ = false;

		// Kept because HTTP/3 needs a second TLS context built from the same
		// certificate but a different ALPN list, and that context cannot be built until
		// run() knows the port.
		AppTlsConfig tls_config_;

		// HTTP/3 support
		//
		// Guarded because a unique_ptr to an incomplete type cannot be destroyed, and
		// without HTTP/3 built in nothing ever completes the forward declaration.
#ifdef COROUTE_HAS_HTTP3
		std::unique_ptr<http3::Http3EndpointGroup> http3_group_;
#endif
		bool http3_enabled_ = false;

		// The port QUIC is actually listening on, published for the Alt-Svc header.
		// Zero until run() has bound the socket, which is what the middleware checks:
		// advertising a port nothing is listening on sends clients somewhere that will
		// time out.
		std::atomic<uint16_t> http3_port_{0};

		// HTTP/2 support
#ifdef COROUTE_HAS_HTTP2
		bool http2_enabled_ = true;  // Enable by default when available
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
		// Both defined in the .cpp. The HTTP/3 endpoint is held by pointer to an
		// incomplete type, and a defaulted constructor needs that type complete just as
		// much as the destructor does, because it has to be able to unwind.
		App();
		~App();

		// Non-copyable, non-movable (due to atomics)
		App(const App&) = delete;
		App& operator=(const App&) = delete;
		App(App&&) = delete;
		App& operator=(App&&) = delete;

		// Configuration
		App& threads(size_t count)
		{
			thread_count_ = count;
			return *this;
		}

		// Which I/O backend the event loop runs on.
		//
		// The seam the I/O-portability experiment needs: one binary, both Linux
		// backends compiled in under -DCOROUTE_IO_BACKEND=dual, and the arm chosen per
		// process rather than per build. The default asks the host, so an ordinary
		// application never sets this and still gets io_uring where it is allowed and
		// epoll where it is not.
		//
		// Must be called before run(); the context is built there and not rebuilt.
		App& io_backend(net::IoBackend backend)
		{
			io_backend_ = backend;
			return *this;
		}

		// The backend the running context actually is, or nullptr before run() builds
		// it. What ran, not what was asked for: IoBackend::Default resolves against the
		// host, so this is the only thing that can answer for a fallback.
		[[nodiscard]] const char* effective_io_backend() const noexcept
		{
			return io_ctx_ ? io_ctx_->backend_name() : nullptr;
		}

		// The listen backlog, used by both accept paths.
		//
		// One value on purpose. The two paths used to disagree: multi-accept defaulted
		// to 1024 and the single-listener fallback to 128, so a server that quietly took
		// the fallback got an eighth of the queue. Under load that difference shows up
		// as accept behaviour and gets attributed to the accept model, which is exactly
		// the confound a comparison between those two models must not have.
		//
		// It is also a variable worth sweeping rather than a constant worth picking.
		// At several thousand concurrent connections the default is small enough to
		// matter, and a measurement that does not vary it cannot say whether it did.
		App& backlog(int entries)
		{
			backlog_ = entries;
			return *this;
		}

		[[nodiscard]] int backlog() const noexcept { return backlog_; }

		// Turns first-octet protocol classification off.
		//
		// Exists for one experiment: the cost of the demultiplexer is claimed to be
		// undetectable, and claiming that requires an arm without it. With detection off
		// the endpoint becomes a dedicated listener that already knows what is arriving,
		// which is the arrangement every production server offers today and the one this
		// is measured against.
		//
		// Which dedicated listener depends on enable_tls. Without a certificate it is a
		// cleartext HTTP/1.1 listener and every accepted connection goes straight to the
		// parser. With one it is nginx's `listen 443 ssl`: every accepted connection goes
		// straight into the handshake, and ALPN still selects h2. What is gone in both
		// cases is the octet read that decides between them, and the replaying wrapper
		// that read makes necessary. Prior-knowledge h2c is gone too, since nothing looks
		// for the preface.
		//
		// This is the before picture, not a supported configuration: a server started
		// this way serves one transport, and choosing which one is the operator's job
		// again rather than the first octet's.
		//
		// A runtime flag rather than a build option, deliberately. Build-to-build
		// variation from code layout and link order is documented at 5 to 10 percent,
		// while run-to-run variation on a controlled machine is 1 to 2 percent. A 3
		// percent difference between two separately compiled binaries is
		// indistinguishable from having linked the objects in a different order, so both
		// arms have to come out of the same binary.
		App& enable_protocol_detection(bool enable)
		{
			protocol_detection_ = enable;
			return *this;
		}

		[[nodiscard]] bool protocol_detection() const noexcept { return protocol_detection_; }

		// The limit on the classification window: from accept to knowing the protocol.
		//
		// A peer that connects and says nothing otherwise parks a coroutine for as long
		// as it likes, because Connection::set_timeout is stored by every backend and
		// enforced by none. Zero disables the limit, which is only useful under a
		// debugger.
		App& handshake_timeout(std::chrono::milliseconds limit)
		{
			handshake_timeout_ = limit;
			return *this;
		}

		[[nodiscard]] std::chrono::milliseconds handshake_timeout() const noexcept
		{
			return handshake_timeout_;
		}

		// The idle limit on an established HTTP/1.1 connection. Zero disables it.
		App& keep_alive_timeout(std::chrono::milliseconds limit)
		{
			keep_alive_timeout_ = limit;
			return *this;
		}

		[[nodiscard]] std::chrono::milliseconds keep_alive_timeout() const noexcept
		{
			return keep_alive_timeout_;
		}

		// Requests per connection before it is closed. Zero removes the limit.
		App& max_requests_per_connection(std::size_t limit)
		{
			max_requests_per_connection_ = limit;
			return *this;
		}

		[[nodiscard]] std::size_t max_requests_per_connection() const noexcept
		{
			return max_requests_per_connection_;
		}

		// Route registration (simple form)
		App& route(HttpMethod method, std::string pattern, Handler handler)
		{
			router_.add(method, std::move(pattern), std::move(handler));
			return *this;
		}

		// Convenience methods
		App& get(std::string pattern, Handler handler)
		{
			return route(HttpMethod::GET, std::move(pattern), std::move(handler));
		}

		App& post(std::string pattern, Handler handler)
		{
			return route(HttpMethod::POST, std::move(pattern), std::move(handler));
		}

		App& put(std::string pattern, Handler handler)
		{
			return route(HttpMethod::PUT, std::move(pattern), std::move(handler));
		}

		App& del(std::string pattern, Handler handler)
		{
			return route(HttpMethod::DELETE, std::move(pattern), std::move(handler));
		}

		// Route registration with automatic parameter extraction
		template <typename... Args, typename F>
			requires std::invocable<F, Args..., Request&>
		App& get(std::string pattern, F&& handler)
		{
			router_.get<Args...>(std::move(pattern), std::forward<F>(handler));
			return *this;
		}

		template <typename... Args, typename F>
			requires std::invocable<F, Args..., Request&>
		App& post(std::string pattern, F&& handler)
		{
			router_.post<Args...>(std::move(pattern), std::forward<F>(handler));
			return *this;
		}

		template <typename... Args, typename F>
			requires std::invocable<F, Args..., Request&>
		App& put(std::string pattern, F&& handler)
		{
			router_.put<Args...>(std::move(pattern), std::forward<F>(handler));
			return *this;
		}

		template <typename... Args, typename F>
			requires std::invocable<F, Args..., Request&>
		App& del(std::string pattern, F&& handler)
		{
			router_.del<Args...>(std::move(pattern), std::forward<F>(handler));
			return *this;
		}

		// Access router directly
		Router& router() noexcept { return router_; }
		const Router& router() const noexcept { return router_; }

		// WebSocket route registration
		App& ws(std::string path, WebSocketHandler handler)
		{
			ws_handlers_[std::move(path)] = std::move(handler);
			return *this;
		}

		// ========================================================================
		// Fetch API (In-Process Request Dispatch)
		// ========================================================================

		/// Set the authentication state manager.
		/// For web: use WebAuthState (no-op).
		/// For desktop/mobile: use ClientAuthState (manages cookies/tokens).
		App& set_auth_state(std::unique_ptr<AuthState> auth)
		{
			auth_state_ = std::move(auth);
			return *this;
		}

		/// Access the current auth state (may be null).
		AuthState* auth_state() noexcept { return auth_state_.get(); }
		const AuthState* auth_state() const noexcept { return auth_state_.get(); }

#ifdef COROUTE_CLIENT_MODE
		/// Set the fetch transport for client mode.
		/// Required for desktop/mobile builds to make HTTP requests.
		App& set_fetch_transport(std::unique_ptr<FetchTransport> transport, std::string base_url = "")
		{
			fetch_transport_ = std::move(transport);
			fetch_base_url_ = std::move(base_url);
			return *this;
		}

		/// Get the base URL for fetch requests.
		const std::string& fetch_base_url() const noexcept { return fetch_base_url_; }
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
		Task<Response> fetch(HttpMethod method, std::string_view route, std::string body = "")
		{
			// Build request
			Request req;
			req.set_method(method);
			req.set_path(std::string(route));
			req.set_body(std::move(body));
			req.set_http_version("HTTP/1.1");

			// Apply auth state (add cookies, tokens, etc.)
			if (auth_state_)
			{
				auth_state_->apply(req);
			}

#ifdef COROUTE_CLIENT_MODE
			// Client mode: dispatch via HTTP transport to remote server
			if (!fetch_transport_)
			{
				throw std::runtime_error("fetch() called in client mode without transport configured");
			}
			Response resp = co_await fetch_transport_->dispatch(req);
#else
			// Web/server mode: dispatch in-process through middleware chain
			auto match = router_.match(method, route);
			if (match)
			{
				req.set_route_params(std::move(match.params));
			}
			Response resp = co_await middleware_chain_.execute_or_not_found(req, match.handler);
#endif

			// Observe response for auth state updates
			if (auth_state_)
			{
				auth_state_->observe(resp);
			}

			co_return resp;
		}

		/// Convenience methods for fetch
		Task<Response> fetch_get(std::string_view route) { return fetch(HttpMethod::GET, route); }

		Task<Response> fetch_post(std::string_view route, std::string body = "")
		{
			return fetch(HttpMethod::POST, route, std::move(body));
		}

		Task<Response> fetch_put(std::string_view route, std::string body = "")
		{
			return fetch(HttpMethod::PUT, route, std::move(body));
		}

		Task<Response> fetch_delete(std::string_view route) { return fetch(HttpMethod::DELETE, route); }

		// ========================================================================
		// Fetch with Original Request (for web/server cookie forwarding)
		// ========================================================================

		/// Fetch with cookie/header forwarding from original request.
		/// Use this in web/server mode to propagate browser cookies to API calls.
		Task<Response> fetch(const Request& original, HttpMethod method, std::string_view route, std::string body = "")
		{
			// Build request
			Request req;
			req.set_method(method);
			req.set_path(std::string(route));
			req.set_body(std::move(body));
			req.set_http_version("HTTP/1.1");

			// Forward cookies from original request (web/server mode)
			auto cookie = original.header("Cookie");
			if (cookie)
			{
				req.add_header("Cookie", std::string(*cookie));
			}

			// Forward authorization header if present
			auto auth = original.header("Authorization");
			if (auth)
			{
				req.add_header("Authorization", std::string(*auth));
			}

			// Apply auth state (may override with stored tokens for client mode)
			if (auth_state_)
			{
				auth_state_->apply(req);
			}

#ifdef COROUTE_CLIENT_MODE
			if (!fetch_transport_)
			{
				throw std::runtime_error("fetch() called in client mode without transport configured");
			}
			Response resp = co_await fetch_transport_->dispatch(req);
#else
			auto match = router_.match(method, route);
			if (match)
			{
				req.set_route_params(std::move(match.params));
			}
			Response resp = co_await middleware_chain_.execute_or_not_found(req, match.handler);
#endif

			if (auth_state_)
			{
				auth_state_->observe(resp);
			}

			co_return resp;
		}

		/// Convenience methods with original request forwarding
		Task<Response> fetch_get(const Request& original, std::string_view route)
		{
			return fetch(original, HttpMethod::GET, route);
		}

		Task<Response> fetch_post(const Request& original, std::string_view route, std::string body = "")
		{
			return fetch(original, HttpMethod::POST, route, std::move(body));
		}

		// ========================================================================
		// View Route Registration & Middleware
		// ========================================================================
#ifdef COROUTE_HAS_TEMPLATES

		/// Add global view middleware (runs for all views, UI-level concerns only).
		App& use_view(ViewMiddleware mw)
		{
			global_view_middleware_.add(std::move(mw));
			return *this;
		}

		/// Register a view route with typed ViewModel.
		/// Handler signature: (Request&) -> View<VM>
		template <typename VM, typename Handler>
			requires requires(Handler h, Request& r) {
				{ h(r) } -> std::same_as<View<VM>>;
			}
		App& view(std::string_view path, Handler&& handler)
		{
			std::string path_str(path);
			auto wrapper = [this, path_str,
			                h = std::forward<Handler>(handler)](Request& req) mutable -> Task<ViewResultAny>
			{
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
			requires requires(Handler h, Request& r, ViewExecutionContext& ctx) {
				{ h(r, ctx) } -> std::same_as<View<VM>>;
			}
		App& view(std::string_view path, Handler&& handler)
		{
			std::string path_str(path);
			auto wrapper = [this, path_str,
			                h = std::forward<Handler>(handler)](Request& req) mutable -> Task<ViewResultAny>
			{
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
			requires requires(Handler h, Request& r, ViewExecutionContext& ctx) {
				{ h(r, ctx) } -> std::same_as<View<VM>>;
			}
		App& view(std::string_view path, std::vector<ViewMiddleware> per_route_mw, Handler&& handler)
		{
			std::string path_str(path);
			auto wrapper = [this, path_str, per_mw = std::move(per_route_mw),
			                h = std::forward<Handler>(handler)](Request& req) mutable -> Task<ViewResultAny>
			{
				// Run global view middleware
				ViewExecutionContext ctx{.route = path_str, .view_name = ""};
				co_await global_view_middleware_.execute(ctx);

				// Run per-route middleware
				for (const auto& mw : per_mw)
				{
					co_await mw(ctx);
				}

				// Call handler with context
				ViewResult<VM> result = co_await h(req, ctx);
				co_return ViewResultAny(std::move(result));
			};
			router_.add_view(std::string(path), std::move(wrapper));
			return *this;
		}

#endif  // COROUTE_HAS_TEMPLATES

		// Middleware registration
		// Middleware is called in order: first registered = outermost (runs first on
		// request, last on response)
		App& use(Middleware middleware)
		{
			middleware_chain_.add(std::move(middleware));
			return *this;
		}

		// Run the server (blocking)
		void run(uint16_t port);

		// Run the server (async)
		Task<void> run_async(uint16_t port);

		// Enable TLS/HTTPS
#ifdef COROUTE_HAS_TLS
		App& enable_tls(const AppTlsConfig& config);
		bool tls_enabled() const noexcept { return tls_enabled_; }

		// Serves HTTP/3 on the same port number as TCP, over UDP.
		//
		// Requires TLS: QUIC has no cleartext mode at all (RFC 9001 section 4.2 fixes
		// it to TLS 1.3), so this is refused rather than silently ignored if no
		// certificate is configured by the time run() is called.
		//
		// Also registers the Alt-Svc middleware. A browser will not try HTTP/3 on its
		// own; it connects over TCP and upgrades only if the response advertises an
		// endpoint. Without that header this port is reachable in theory and unused in
		// practice.
		App& enable_http3(bool enable = true);
		bool http3_enabled() const noexcept { return http3_enabled_; }

#ifdef COROUTE_HAS_HTTP3
		// What the QUIC endpoints did with what arrived, summed across workers.
		//
		// Exposed because forwarded_in over received is a measurement this design has to
		// produce rather than assume: it says how often connection-ID steering in the
		// kernel would have saved a userspace handoff, and therefore whether the eBPF
		// version is worth building at all.
		[[nodiscard]] http3::Http3Stats http3_stats() const noexcept;
#endif
#endif

		// HTTP/2 configuration
#ifdef COROUTE_HAS_HTTP2
		App& enable_http2(bool enable = true)
		{
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
		App& set_templates(const std::filesystem::path& dir)
		{
			template_dir_ = dir;
			if (!template_env_)
			{
				template_env_ = std::make_unique<inja::Environment>();
			}
			template_env_->set_search_included_templates_in_files(true);
			return *this;
		}

		// Enable/disable template caching
		App& set_template_caching(bool enabled)
		{
			template_caching_ = enabled;
			return *this;
		}

		// Render a template string with data
		std::string render(std::string_view template_str, const nlohmann::json& data)
		{
			ensure_template_env();
			return template_env_->render(std::string(template_str), data);
		}

		// Render a template file with data (like v1's app.render())
		std::string render(const std::string& filename, const nlohmann::json& data)
		{
			ensure_template_env();

			if (template_caching_)
			{
				std::lock_guard lock(template_mutex_);
				auto it = template_cache_.find(filename);
				if (it != template_cache_.end())
				{
					return template_env_->render(it->second, data);
				}
			}

			auto path = template_dir_ / filename;
			auto tmpl = template_env_->parse_template(path.string());
			std::string result = template_env_->render(tmpl, data);

			if (template_caching_)
			{
				std::lock_guard lock(template_mutex_);
				// Cache the template content
				std::ifstream file(path);
				if (file)
				{
					std::ostringstream ss;
					ss << file.rdbuf();
					template_cache_[filename] = ss.str();
				}
			}

			return result;
		}

		// Render template to Response
		Response render_html(const std::string& filename, const nlohmann::json& data)
		{
			return Response::html(render(filename, data));
		}

		// Add custom template callback
		void add_template_callback(const std::string& name, int num_args,
		                           const std::function<nlohmann::json(inja::Arguments&)>& callback)
		{
			ensure_template_env();
			template_env_->add_callback(name, num_args, callback);
		}

		// Clear template cache
		void clear_template_cache()
		{
			std::lock_guard lock(template_mutex_);
			template_cache_.clear();
		}

		// Access inja environment for advanced configuration
		inja::Environment& template_env()
		{
			ensure_template_env();
			return *template_env_;
		}

		// Access template directory for view renderer
		const std::filesystem::path& template_dir() const noexcept { return template_dir_; }

	private:
		void ensure_template_env()
		{
			if (!template_env_)
			{
				template_env_ = std::make_unique<inja::Environment>();
			}
		}

	public:
#endif  // coroute_HAS_TEMPLATES

		// Stop the server (immediate)
		void stop();

		// Graceful shutdown - stops accepting, waits for connections to drain
		void shutdown(ShutdownOptions options = {});

		// Check if server is shutting down
		bool is_shutting_down() const { return shutting_down_.load(std::memory_order_relaxed); }

		// Get active connection count
		size_t active_connections() const { return active_connections_.load(std::memory_order_relaxed); }

		// Get cancellation token for graceful shutdown
		CancellationToken cancellation_token() const { return cancel_source_.token(); }

	private:
		// Classify an accepted connection and hand it to the right protocol.
		//
		// This is the single entry point for every accepted socket. It reads the first
		// octets, decides TLS or cleartext, and dispatches. Every protocol the server
		// speaks over TCP reaches its handler through here, which is what allows one
		// listening descriptor to serve all of them.
		Task<void> serve_connection(std::unique_ptr<net::Connection> conn);

		// What classification concluded, and the connection to carry on with.
		//
		// Deliberately not an Http2Connection: building one here would put the
		// construction back in two places, which is where the two branches had drifted
		// apart. The caller turns the flag into a connection object, once.
		struct Detected
		{
			std::unique_ptr<net::Connection> conn;
			bool http2 = false;
		};

		// Everything from the first octet to knowing the protocol, and nothing after it.
		//
		// A separate coroutine because the deadline on this window is a local of it. The
		// frame ending is the disarm, so no exit path can forget one, and there are
		// eleven exits. That is the same lesson as the awaiter race: when the ordinary
		// use of an interface contains an easy mistake, the fix is the interface.
		Task<std::optional<Detected>> detect_protocol(std::unique_ptr<net::Connection> conn);

		// The TLS half of the above, without the part that decides it is the TLS half.
		//
		// Two callers, and the difference between them is the experiment. detect_protocol
		// arrives having read one octet and found 0x16. serve_connection arrives, when
		// detection is off and a certificate is configured, having read nothing: that is
		// the dedicated TLS listener the demultiplexer is measured against.
		//
		// The deadline is the caller's. Opening a second one here would give a peer that
		// stalls mid-handshake two windows rather than the one the design specifies.
		//
		// Taken by reference to a forward-declared type, so deadline.hpp stays out of
		// this header: it is an implementation detail of the accept path and nothing
		// that includes app.hpp needs it.
		Task<std::optional<Detected>> tls_handshake(std::unique_ptr<net::Connection> conn,
		                                            net::Deadline& deadline);

		// The route-and-middleware dispatch used by every protocol.
		//
		// HTTP/1.1 reaches the router through handle_connection, HTTP/2 through
		// Http2Connection::set_handler. Both need the same lambda, and it was written
		// out twice; HTTP/3 would have made it three times. Spelled out rather than
		// using http2::RequestHandler so this compiles with HTTP/2 disabled.
		std::function<Task<Response>(Request&)> make_request_handler();

#ifdef COROUTE_HAS_TEMPLATES
		// Sends a view whose model still has values on the way.
		//
		// The page goes out first, with a hole per pending value, and each hole is
		// filled by a later chunk. Returns false if the connection failed, in which case
		// the caller should stop rather than try to send anything else on it.
		Task<bool> stream_deferred_view(net::Connection& conn, const std::string& html,
		                                const DeferredCollector& collector, bool keep_alive);
#endif

		// Brings up the QUIC endpoint on the same port number, over UDP. Does nothing
		// unless enable_http3() was called; throws if it was and TLS is missing.
		void start_http3(uint16_t port);

		// Handle a single connection
		Task<void> handle_connection(std::unique_ptr<net::Connection> conn);

		// Parse HTTP request from connection
		Task<expected<Request, Error>> parse_request(net::Connection& conn);

		// Check if request is a WebSocket upgrade and handle it
		// Returns true if handled as WebSocket, false if should continue as HTTP
		Task<bool> try_websocket_upgrade(std::unique_ptr<net::Connection>& conn, Request& req);

		// Check if request is an HTTP/2 upgrade (h2c) and handle it
		// Returns true if handled as HTTP/2, false if should continue as HTTP/1.1
#ifdef COROUTE_HAS_HTTP2
		Task<bool> try_http2_upgrade(std::unique_ptr<net::Connection>& conn, Request& req);

		// Handle HTTP/2 connection
		Task<void> handle_http2_connection(std::shared_ptr<http2::Http2Connection> h2_conn);
#endif
	};

}  // namespace coroute
