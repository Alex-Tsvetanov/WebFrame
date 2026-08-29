#include "coroute/core/app.hpp"
#include "coroute/util/zero_copy.hpp"
#include "coroute/net/protocol_detect.hpp"
#include "coroute/net/deadline.hpp"
#include "coroute/net/idle_timeout.hpp"
#include "coroute/core/chunked.hpp"
#include "coroute/core/request_parse.hpp"
#ifdef COROUTE_HAS_HTTP3
#include "coroute/http3/endpoint.hpp"
#endif
#include <cstring>
#include <optional>
#include <iostream>
#include <sstream>
#include <thread>
#include <stdexcept>
#include <tuple>

namespace coroute
{

	// Out of line because http3::Http3EndpointGroup is only forward declared in the header.
	App::App() = default;
	App::~App() = default;

	std::function<Task<Response>(Request&)> App::make_request_handler()
	{
		return [this](Request& r) -> Task<Response>
		{
			auto match = router_.match(r.method(), r.path());
			if (match)
			{
				r.set_route_params(std::move(match.params));
			}
			co_return co_await middleware_chain_.execute_or_not_found(r, match.handler);
		};
	}

	Task<std::optional<App::Detected>> App::detect_protocol(std::unique_ptr<net::Connection> conn)
	{
		// The window this deadline covers is exactly this coroutine: from the first read
		// to the moment the protocol is known. Leaving the frame disarms it, so none of
		// the ways out below has to remember to, and there are eleven of them.
		//
		// It exists because nothing else in the stack will wake a parked classification
		// read. Connection::set_timeout is stored by all four backends and enforced by
		// none, and handle_connection already relies on it for keep-alive, so the gap is
		// wider than this window. Closing it properly is a per-backend change; this
		// covers the window the demultiplexer added.
		//
		// Cost is one heap push per accepted connection, not per read. Against a TCP
		// accept, and against a TLS handshake on the branch that has one, that is not a
		// quantity the A/B in chapter VI can resolve.
		net::Deadline deadline(*io_ctx_, handshake_timeout_, [c = conn.get()] { c->close(); });

		auto prefix = co_await net::read_prefix(std::move(conn), 1);
		if (!prefix)
		{
			co_return std::nullopt;  // peer went away before saying anything
		}

		std::unique_ptr<net::Connection> stream = std::move(prefix->conn);

		// read_prefix hands back the replaying wrapper rather than the socket it wrapped,
		// so the deadline has to follow it. Closing the old object would close nothing
		// that anyone is reading from.
		deadline.replace([c = stream.get()] { c->close(); });

		switch (net::classify(prefix->bytes))
		{
			case net::WireProtocol::Tls:
			{
#ifdef COROUTE_HAS_TLS
				if (!tls_ctx_)
				{
					// A TLS client reached a server with no certificate configured.
					// Closing is the only honest answer: replying in cleartext would
					// be unreadable to the peer.
					stream->close();
					co_return std::nullopt;
				}

				auto tls = net::TlsConnection::create(std::move(stream), *tls_ctx_, true);
				if (!tls)
				{
					co_return std::nullopt;
				}

				std::unique_ptr<net::TlsConnection> tls_conn = std::move(*tls);

				// The handshake is inside the window on purpose: a peer that opens with
				// 0x16 and then stalls is the same attack as one that says nothing.
				deadline.replace([c = tls_conn.get()] { c->close(); });

				auto handshake = co_await tls_conn->handshake();
				if (!handshake)
				{
					co_return std::nullopt;
				}

#ifdef COROUTE_HAS_HTTP2
				if (http2_enabled_)
				{
					auto proto = tls_conn->negotiated_protocol();
					if (proto && *proto == "h2")
					{
						co_return Detected{std::move(tls_conn), true};
					}
				}
#endif
				co_return Detected{std::move(tls_conn), false};
#else
				stream->close();
				co_return std::nullopt;
#endif
			}

			case net::WireProtocol::Cleartext:
			{
#ifdef COROUTE_HAS_HTTP2
				// Prior-knowledge HTTP/2: a client that already knows this endpoint
				// speaks h2 opens with the preface instead of a request line. Only
				// top up the buffer while the bytes keep matching, so a normal request
				// never pays for the check.
				if (http2_enabled_ && net::preface_match(prefix->bytes) != net::PrefaceMatch::No)
				{
					auto full = co_await net::read_prefix(std::move(stream), net::http2_client_preface.size());
					if (!full)
					{
						co_return std::nullopt;
					}
					stream = std::move(full->conn);
					deadline.replace([c = stream.get()] { c->close(); });

					if (net::preface_match(full->bytes) == net::PrefaceMatch::Yes)
					{
						co_return Detected{std::move(stream), true};
					}
				}
#endif
				// HTTP/1.1, which also covers the WebSocket and h2c upgrade paths.
				co_return Detected{std::move(stream), false};
			}

			case net::WireProtocol::Unknown:
			default:
				// Neither a TLS record nor an HTTP method token. Nothing useful can be
				// said back, so drop it rather than guess at a protocol.
				stream->close();
				co_return std::nullopt;
		}
	}

	Task<void> App::serve_connection(std::unique_ptr<net::Connection> conn)
	{
		// Deliberately does not touch active_connections_. Both handle_connection and
		// handle_http2_connection account for the connection themselves, so counting it
		// here as well would double-count it for the length of the classification.
		if (!protocol_detection_)
		{
			// The arm with the demultiplexer switched off. Straight to HTTP/1.1 without
			// reading a byte to decide, which is what the cost of classification is
			// measured against. See App::enable_protocol_detection.
			co_await handle_connection(std::move(conn));
			co_return;
		}

		auto detected = co_await detect_protocol(std::move(conn));
		if (!detected)
		{
			co_return;
		}

#ifdef COROUTE_HAS_HTTP2
		if (detected->http2)
		{
			// One place rather than two. The TLS-with-h2 branch and the prior-knowledge
			// branch used to build this separately, which is how they came to differ.
			auto h2 = std::make_shared<http2::Http2Connection>(std::move(detected->conn));
			h2->set_handler(make_request_handler());
			co_await handle_http2_connection(std::move(h2));
			co_return;
		}
#endif
		co_await handle_connection(std::move(detected->conn));
	}

#ifdef COROUTE_HAS_TEMPLATES
	Task<bool> App::stream_deferred_view(net::Connection& conn, const std::string& html,
	                                     const DeferredCollector& collector, bool keep_alive)
	{
		// Chunked rather than a length: the length is not known, because the values that
		// go in the holes have not arrived. That is the whole reason to stream.
		ChunkedResponse chunked(&conn);
		chunked.status(200).header("Content-Type", "text/html; charset=utf-8");
		chunked.header("Connection", keep_alive ? "keep-alive" : "close");

		// Headers go out with the first write rather than by a separate call, which is
		// what keeps the shell and its headers in one flush.
		//
		// The page first, complete except for the holes. This is where the time is
		// saved: a reader sees the layout, the navigation and everything already known
		// without waiting on the slowest query on the page.
		if (!co_await chunked.write(with_deferred_runtime(html)))
		{
			co_return false;
		}

		// In slot order rather than completion order.
		//
		// Every deferred started when it was constructed, so they are all running
		// already and the total wait is the slowest one either way. What in-order costs
		// is that a value which arrives early waits behind a slower one before reaching
		// the page. The first flush above, which is the large win, is unaffected.
		//
		// ponytail: in-order flush, upgrade to a completion queue if measurement shows
		// per-slot arrival time matters.
		for (std::size_t slot = 0; slot < collector.pending().size(); ++slot)
		{
			const auto& state = collector.pending()[slot];
			co_await await_deferred(state);

			if (!co_await chunked.write(deferred_resolve_script(slot, state->to_json())))
			{
				co_return false;
			}
		}

		if (!co_await chunked.finish())
		{
			co_return false;
		}
		co_return true;
	}
#endif

	void App::start_http3(uint16_t port)
	{
		if (!http3_enabled_)
		{
			return;
		}

#ifdef COROUTE_HAS_HTTP3
		// QUIC gets its own TLS context, built from the same certificate. It cannot
		// share the TCP one: that advertises h2 and http/1.1, and this must advertise
		// only h3 and force TLS 1.3, which QUIC requires (RFC 9001 section 4.2).
		net::TlsConfig quic_config;
		quic_config.cert_file = tls_config_.cert_file;
		quic_config.key_file = tls_config_.key_file;
		quic_config.ca_file = tls_config_.ca_file;
		quic_config.chain_file = tls_config_.chain_file;
		quic_config.verify_client = tls_config_.verify_client;

		if (quic_config.cert_file.empty() || quic_config.key_file.empty())
		{
			// Refused rather than skipped. QUIC has no cleartext mode, so there is no
			// degraded thing to fall back to, and a server that quietly ignored this
			// would advertise Alt-Svc for a port serving nothing.
			throw std::runtime_error("HTTP/3 requires TLS: call enable_tls() before run()");
		}

		// One endpoint per worker where the backend can direct work at a named thread,
		// otherwise one. The group decides which, because the answer depends on the I/O
		// backend rather than on anything the application said.
		http3_group_ = std::make_unique<http3::Http3EndpointGroup>(*io_ctx_, quic_config,
		                                                         make_request_handler());

		// Same port number as TCP, different transport. Sharing the number is what lets
		// one Alt-Svc header point at "the same place, over UDP".
		if (auto bound = http3_group_->bind(port); !bound)
		{
			throw std::runtime_error("HTTP/3 bind failed: " + bound.error().to_string());
		}

		// Published only now, so the Alt-Svc middleware cannot advertise a port before
		// anything is listening on it.
		http3_port_.store(http3_group_->local_port(), std::memory_order_relaxed);
		std::cout << "HTTP/3 listening on UDP " << http3_group_->local_port() << " across "
				  << http3_group_->worker_count() << " worker(s)" << '\n'
				  << std::flush;

		http3_group_->start();
#else
		(void)port;
#endif
	}

#ifdef COROUTE_HAS_HTTP3
	http3::Http3Stats App::http3_stats() const noexcept
	{
		return http3_group_ ? http3_group_->stats() : http3::Http3Stats{};
	}
#endif

	void App::run(uint16_t port)
	{
		io_ctx_ = net::IoContext::create(thread_count_);

		// One listening descriptor, whatever mix of protocols is configured. TLS and
		// cleartext used to be mutually exclusive branches here, so serving both meant
		// two App instances: two thread pools, two event loops and the routes
		// registered twice. Classification happens per connection instead.
		auto on_connection = [this](std::unique_ptr<net::Connection> conn)
		{ serve_connection(std::move(conn)).start_detached(); };

		start_http3(port);

		if (io_ctx_->enable_multi_accept(port, on_connection, backlog_))
		{
			std::cout << "Server listening on port " << port << " (multi-accept)" << '\n' << std::flush;
		}
		else
		{
			listener_ = net::Listener::create(*io_ctx_);
			// Same backlog as the multi-accept path above. These used to differ, 1024
			// against 128, so falling back quietly cost seven eighths of the queue.
			auto result = listener_->listen(port, backlog_);
			if (!result)
			{
				throw std::runtime_error("Failed to listen: " + result.error().to_string());
			}

			std::cout << "Server listening on port " << listener_->local_port() << '\n' << std::flush;

			[this, on_connection]() -> Task<void>
			{
				while (!cancel_source_.is_cancelled())
				{
					auto conn = co_await listener_->async_accept();
					if (!conn)
					{
						if (cancel_source_.is_cancelled()) break;
						std::cerr << "Accept error: " << conn.error().to_string() << '\n';
						continue;
					}
					on_connection(std::move(*conn));
				}
			}()
										   .start_detached();
		}

		// Run the event loop
		io_ctx_->run();
	}

	Task<void> App::run_async(uint16_t port)
	{
		io_ctx_ = net::IoContext::create(thread_count_);
		listener_ = net::Listener::create(*io_ctx_);

		auto result = listener_->listen(port);
		if (!result)
		{
			throw std::runtime_error("Failed to listen: " + result.error().to_string());
		}

		std::cout << "Server listening on port " << listener_->local_port() << '\n' << std::flush;

		while (!cancel_source_.is_cancelled())
		{
			auto conn_result = co_await listener_->async_accept();
			if (!conn_result)
			{
				if (cancel_source_.is_cancelled())
				{
					break;
				}
				std::cerr << "Accept error: " << conn_result.error().to_string() << '\n';
				continue;
			}

			// Handle connection (fire and forget - self-destroys on completion)
			handle_connection(std::move(*conn_result)).start_detached();
		}
	}

	void App::stop()
	{
		shutting_down_.store(true, std::memory_order_relaxed);
		cancel_source_.cancel();
		if (listener_)
		{
			listener_->close();
		}
		if (io_ctx_)
		{
			io_ctx_->stop();
		}
	}

	void App::shutdown(ShutdownOptions options)
	{
		std::cout << "Initiating graceful shutdown..." << '\n' << std::flush;

		// Mark as shutting down
		shutting_down_.store(true, std::memory_order_relaxed);

		// Stop accepting new connections
		if (listener_)
		{
			listener_->close();
		}

		// Wait for existing connections to drain
		auto start = std::chrono::steady_clock::now();
		while (active_connections_.load(std::memory_order_relaxed) > 0)
		{
			auto elapsed = std::chrono::steady_clock::now() - start;
			if (elapsed >= options.drain_timeout)
			{
				std::cout << "Drain timeout reached with " << active_connections_.load(std::memory_order_relaxed)
						  << " connections remaining" << '\n';
				break;
			}

			// Sleep briefly before checking again
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}

		// Force close if configured and connections remain
		if (options.force_close_after_timeout && active_connections_.load(std::memory_order_relaxed) > 0)
		{
			std::cout << "Force closing remaining connections..." << '\n' << std::flush;
			cancel_source_.cancel();
		}

		// Stop the event loop
		if (io_ctx_)
		{
			io_ctx_->stop();
		}

		std::cout << "Shutdown complete" << '\n' << std::flush;
	}

#ifdef COROUTE_HAS_TLS
	App& App::enable_http3(bool enable)
	{
#ifndef COROUTE_HAS_HTTP3
		if (enable)
		{
			// Asked for and not built in. Saying so beats starting a server that
			// silently answers nothing on UDP.
			throw std::runtime_error("HTTP/3 was requested but this build has it disabled "
			                         "(configure with -DCOROUTE_ENABLE_HTTP3=ON)");
		}
#endif
		if (enable && !http3_enabled_)
		{
			// One middleware covers every protocol, rather than editing each response
			// finalisation path. Registered here so it exists whatever order the
			// application sets things up in.
			use(
				[this](Request& request, Next next) -> Task<Response>
				{
					Response response = co_await next(request);

					// Only once the socket is actually bound. Advertising a port before
					// then points clients at nothing, and they pay a timeout for it.
					const uint16_t port = http3_port_.load(std::memory_order_relaxed);
					if (port != 0 && !response.header("alt-svc"))
					{
						// ma is how long the client may remember this. A day is the
						// usual choice: long enough to be worth caching, short enough
						// that withdrawing HTTP/3 takes effect within a day.
						response.set_header("Alt-Svc", "h3=\":" + std::to_string(port) + "\"; ma=86400");
					}
					co_return response;
				});
		}
		http3_enabled_ = enable;
		return *this;
	}

	App& App::enable_tls(const AppTlsConfig& config)
	{
		tls_config_ = config;

		net::TlsConfig tls_config;
		tls_config.cert_file = config.cert_file;
		tls_config.key_file = config.key_file;
		tls_config.ca_file = config.ca_file;
		tls_config.chain_file = config.chain_file;
		tls_config.verify_client = config.verify_client;

		// Set ALPN protocols - if not specified, use defaults based on HTTP/2 support
		if (config.alpn_protocols.empty())
		{
#ifdef COROUTE_HAS_HTTP2
			if (http2_enabled_)
			{
				tls_config.alpn_protocols = {"h2", "http/1.1"};
			}
			else
			{
				tls_config.alpn_protocols = {"http/1.1"};
			}
#else
			tls_config.alpn_protocols = {"http/1.1"};
#endif
		}
		else
		{
			// h3 is filtered out rather than passed through. This list is offered over
			// TCP, and HTTP/3 does not run over TCP: a client that selected it here
			// would negotiate a protocol neither end can speak on this connection.
			// HTTP/3 is discovered through Alt-Svc instead.
			for (const auto& protocol : config.alpn_protocols)
			{
				if (protocol != "h3")
				{
					tls_config.alpn_protocols.push_back(protocol);
				}
			}
		}

		auto ctx_result = net::TlsContext::create(tls_config);
		if (!ctx_result)
		{
			throw std::runtime_error("Failed to create TLS context: " + ctx_result.error().to_string());
		}

		tls_ctx_ = std::make_unique<net::TlsContext>(std::move(*ctx_result));
		tls_enabled_ = true;

		return *this;
	}
#endif

	Task<void> App::handle_connection(std::unique_ptr<net::Connection> conn)
	{
		// Track active connection
		active_connections_.fetch_add(1, std::memory_order_relaxed);

		// RAII guard to decrement on exit
		struct ConnectionGuard
		{
			std::atomic<size_t>& counter;
			~ConnectionGuard() { counter.fetch_sub(1, std::memory_order_relaxed); }
		} guard{active_connections_};

		conn->set_cancellation_token(cancel_source_.token());

		// Keep-alive configuration. The idle limit is a member rather than a constant
		// because it is now enforced, so it is something a run has to be able to set.
		constexpr size_t MAX_REQUESTS_PER_CONNECTION = 100;
		const auto KEEP_ALIVE_TIMEOUT = keep_alive_timeout_;

		size_t request_count = 0;
		bool keep_alive = true;

		// Wrapped rather than merely configured. Every backend stores the value passed to
		// set_timeout and none of them acts on it, so without this the is_timeout() branch
		// below is unreachable, and a client that opens a connection and then goes quiet
		// holds a coroutine until it disconnects. The wrapper still passes the value down,
		// so a backend that grows a real implementation agrees with it rather than
		// competing with it.
		conn = std::make_unique<net::IdleTimeout>(std::move(conn), *io_ctx_, KEEP_ALIVE_TIMEOUT);
		conn->set_timeout(KEEP_ALIVE_TIMEOUT);

		// HTTP/1.1 keep-alive loop
		while (conn->is_open() && !cancel_source_.is_cancelled() && keep_alive)
		{
			++request_count;

			// Check max requests limit
			if (request_count > MAX_REQUESTS_PER_CONNECTION)
			{
				break;
			}

			// Parse request
			auto req_result = co_await parse_request(*conn);
			if (!req_result)
			{
				if (req_result.error().is_cancelled() || req_result.error().io_error() == IoError::EndOfStream ||
				    req_result.error().is_timeout())
				{
					break;  // Clean disconnect or timeout
				}

				// The status the error carries, rather than 400 for everything.
				//
				// parse_request already reports PayloadTooLarge for an oversized body or
				// header block, and NotImplemented for a transfer coding it does not
				// support. Both reached the client as 400, so a client could not tell "your
				// request was malformed" from "this server will not do that", which are
				// different things to act on and, for the second, the answer RFC 9112
				// section 6.1 requires.
				auto resp = Response::bad_request(req_result.error().to_string());
				if (req_result.error().is_http())
				{
					// set_status carries the reason phrase with it, so the status line stays
					// consistent rather than saying 501 Bad Request.
					resp.set_status(static_cast<int>(req_result.error().http_error()));
				}

				// Deliberately ignored: the connection is closed on the next line
				// regardless of whether the client is still there to read this.
				auto data = resp.serialize();
				std::ignore = co_await conn->async_write_all(data.data(), data.size());
				break;
			}

			Request& req = *req_result;

			// Check for WebSocket upgrade
			if (co_await try_websocket_upgrade(conn, req))
			{
				// WebSocket upgrade handled - connection is now owned by WS handler
				// conn is now null, so we must return without calling conn->close()
				co_return;
			}

#ifdef COROUTE_HAS_HTTP2
			// Check for HTTP/2 upgrade (h2c)
			if (co_await try_http2_upgrade(conn, req))
			{
				// HTTP/2 upgrade handled - connection is now owned by HTTP/2 handler
				// conn is now null, so we must return without calling conn->close()
				co_return;
			}
#endif

			// Determine keep-alive based on request
			keep_alive = req.keep_alive();

#ifdef COROUTE_HAS_TEMPLATES
			// Check for view routes first (GET-only)
			if (req.method() == HttpMethod::GET)
			{
				auto view_match = router_.match_view(req.path());
				if (view_match.handler)
				{
					// Set route parameters
					req.set_route_params(std::move(view_match.params));

					// Execute view handler
					Response resp;
					try
					{
						ViewResultAny view_result = co_await (*view_match.handler)(req);

						// Values still on the way are collected as the model is
						// serialised, so a field cannot emit a placeholder without being
						// registered for streaming.
						DeferredCollector collector;
						nlohmann::json data;
						{
							DeferredCollector::Scope scope(collector);
							data = view_result.to_json();
						}

						std::string template_name = view_result.templates.web;
						if (!template_name.contains('.'))
						{
							template_name += ".html";
						}

						if (!collector.empty())
						{
							// Streamed, and the response is finished here rather than by
							// the code below: it has already been written.
							const bool ok = co_await stream_deferred_view(
								*conn, render(template_name, data), collector,
								keep_alive && request_count < MAX_REQUESTS_PER_CONNECTION);
							if (!ok)
							{
								break;
							}
							continue;
						}

						resp = render_html(template_name, data);
					}
					catch (const std::exception& e)
					{
						resp = Response::internal_error(e.what());
					}
					catch (...)
					{
						resp = Response::internal_error("Unknown error");
					}

					// Add Connection header based on keep-alive status
					bool should_close = !keep_alive || request_count >= MAX_REQUESTS_PER_CONNECTION;
					if (should_close)
					{
						resp.set_header("Connection", "close");
						keep_alive = false;
					}
					else
					{
						resp.set_header("Connection", "keep-alive");
						resp.set_header("Keep-Alive", "timeout=30, max=" +
						                                  std::to_string(MAX_REQUESTS_PER_CONNECTION - request_count));
					}

					// Send response
					auto data = resp.serialize();
					auto write_result = co_await conn->async_write_all(data.data(), data.size());
					if (!write_result)
					{
						break;
					}

					continue;  // Go to next request in keep-alive loop
				}
			}
#endif

			// Route the request
			auto match = router_.match(req.method(), req.path());

			// Set route parameters if matched
			if (match)
			{
				req.set_route_params(std::move(match.params));
			}

			// Execute handler with pre-compiled middleware chain
			Response resp;
			try
			{
				resp = co_await middleware_chain_.execute_or_not_found(req, match.handler);
			}
			catch (const std::exception& e)
			{
				resp = Response::internal_error(e.what());
			}
			catch (...)
			{
				resp = Response::internal_error("Unknown error");
			}

			// Add Connection header based on keep-alive status
			bool should_close = !keep_alive || request_count >= MAX_REQUESTS_PER_CONNECTION;
			if (should_close)
			{
				resp.set_header("Connection", "close");
				keep_alive = false;
			}
			else
			{
				resp.set_header("Connection", "keep-alive");
				resp.set_header("Keep-Alive",
				                "timeout=30, max=" + std::to_string(MAX_REQUESTS_PER_CONNECTION - request_count));
			}

			// Send response
			if (resp.has_file() && req.method() != HttpMethod::HEAD)
			{
				// Zero-copy file response: send headers then file
				auto headers_data = resp.serialize_headers();
				auto write_result = co_await conn->async_write_all(headers_data.data(), headers_data.size());
				if (!write_result)
				{
					break;
				}

				// Send file body via zero-copy
				const auto& file_info = resp.file_info();
				auto transmit_result =
					co_await send_file_zero_copy(*conn, file_info.path, file_info.offset, file_info.length);

				// Fallback: if transmit fails, we can't recover easily
				// In production, you'd want better error handling
				if (!transmit_result)
				{
					break;
				}
			}
			else
			{
				// Normal response with body in memory
				auto data = resp.serialize();
				auto write_result = co_await conn->async_write_all(data.data(), data.size());
				if (!write_result)
				{
					break;
				}
			}
		}

		conn->close();
	}


	Task<expected<Request, Error>> App::parse_request(net::Connection& conn)
	{
		// HTTP request parser with improved efficiency and validation

		constexpr size_t MAX_HEADER_SIZE = 8192;
		constexpr size_t MAX_BODY_SIZE = 10 * 1024 * 1024;  // 10MB default max
		constexpr size_t READ_CHUNK_SIZE = 1024;

		// Use pooled buffer for reduced allocations
		auto buffer_ptr = buffer_pool_.acquire(MAX_HEADER_SIZE);
		auto& buffer = *buffer_ptr;
		buffer.resize(MAX_HEADER_SIZE);

		// RAII guard to return buffer to pool
		struct BufferGuard
		{
			BufferPool& pool;
			std::unique_ptr<std::vector<char>> buf;
			~BufferGuard()
			{
				if (buf) pool.release(std::move(buf));
			}
		} buffer_guard{buffer_pool_, std::move(buffer_ptr)};
		size_t total_read = 0;
		size_t header_end_pos = std::string::npos;

		// Read headers in chunks for better efficiency
		while (total_read < MAX_HEADER_SIZE)
		{
			size_t to_read = std::min(READ_CHUNK_SIZE, MAX_HEADER_SIZE - total_read);
			auto result = co_await conn.async_read(buffer.data() + total_read, to_read);
			if (!result)
			{
				co_return unexpected(result.error());
			}

			size_t bytes_read = *result;
			if (bytes_read == 0)
			{
				co_return unexpected(Error::io(IoError::EndOfStream, "Connection closed"));
			}

			// Search for end of headers in newly read data
			size_t search_start = (total_read >= 3) ? total_read - 3 : 0;
			total_read += bytes_read;

			for (size_t i = search_start; i + 3 < total_read; ++i)
			{
				if (buffer[i] == '\r' && buffer[i + 1] == '\n' && buffer[i + 2] == '\r' && buffer[i + 3] == '\n')
				{
					header_end_pos = i + 4;
					break;
				}
			}

			if (header_end_pos != std::string::npos)
			{
				break;
			}
		}

		if (header_end_pos == std::string::npos)
		{
			co_return unexpected(Error::http(HttpError::PayloadTooLarge, "Headers too large"));
		}

		// Parse the request
		// The head parses without the connection, so it lives in request_parse.cpp and
		// is reachable from a test and a fuzz harness. Only the body needs the socket.
		auto head = parse_request_head(std::string_view(buffer.data(), total_read));
		if (!head)
		{
			co_return unexpected(head.error());
		}
		Request req = std::move(*head);

		// Read body if Content-Length is present
		auto content_length = req.content_length();
		if (content_length && *content_length > 0)
		{
			// Validate body size
			if (*content_length > MAX_BODY_SIZE)
			{
				co_return unexpected(
					Error::http(HttpError::PayloadTooLarge,
				                "Request body too large (max " + std::to_string(MAX_BODY_SIZE) + " bytes)"));
			}

			std::string body(*content_length, '\0');
			size_t body_read = 0;

			// Check if we already read some body data while reading headers
			size_t body_in_buffer = total_read - header_end_pos;
			if (body_in_buffer > 0)
			{
				size_t to_copy = std::min(body_in_buffer, *content_length);
				std::memcpy(body.data(), buffer.data() + header_end_pos, to_copy);
				body_read = to_copy;
			}

			// Read remaining body
			while (body_read < *content_length)
			{
				auto result = co_await conn.async_read(body.data() + body_read, *content_length - body_read);
				if (!result)
				{
					co_return unexpected(result.error());
				}
				if (*result == 0)
				{
					co_return unexpected(Error::http(HttpError::BadRequest, "Incomplete request body"));
				}
				body_read += *result;
			}

			req.set_body(std::move(body));

			// Parse form body as query parameters if content type is form-urlencoded
			auto ct = req.content_type();
			if (ct && ct->contains("application/x-www-form-urlencoded"))
			{
				std::string_view form_body = req.body();
				while (!form_body.empty())
				{
					auto amp = form_body.find('&');
					std::string_view param = (amp != std::string_view::npos) ? form_body.substr(0, amp) : form_body;

					auto eq = param.find('=');
					if (eq != std::string_view::npos)
					{
						req.add_query_param(url_decode(param.substr(0, eq), true), url_decode(param.substr(eq + 1), true));
					}
					else if (!param.empty())
					{
						// Parameter without value
						req.add_query_param(url_decode(param, true), "");
					}

					if (amp == std::string_view::npos) break;
					form_body = form_body.substr(amp + 1);
				}
			}
		}

		co_return req;
	}

	Task<bool> App::try_websocket_upgrade(std::unique_ptr<net::Connection>& conn, Request& req)
	{
		// Check if this is a WebSocket upgrade request
		if (!net::is_websocket_upgrade(req))
		{
			co_return false;
		}

		// Check if we have a handler for this path
		auto it = ws_handlers_.find(std::string(req.path()));
		if (it == ws_handlers_.end())
		{
			co_return false;
		}

		// Upgrade the connection
		auto ws_conn = co_await net::upgrade_to_websocket(std::move(conn), req);
		if (!ws_conn)
		{
			// Upgrade failed - connection is now invalid
			co_return true;  // Return true to indicate we handled it (even if failed)
		}

		// Call the WebSocket handler
		try
		{
			co_await it->second(std::move(*ws_conn));
		}
		catch (const std::exception& e)
		{
			std::cerr << "WebSocket handler error: " << e.what() << '\n';
		}
		catch (...)
		{
			std::cerr << "WebSocket handler error: unknown" << '\n';
		}

		co_return true;
	}

#ifdef COROUTE_HAS_HTTP2
	Task<bool> App::try_http2_upgrade(std::unique_ptr<net::Connection>& conn, Request& req)
	{
		if (!http2_enabled_)
		{
			co_return false;
		}

		// Check if this is an h2c upgrade request
		if (!http2::is_h2c_upgrade_request(req))
		{
			co_return false;
		}

		// Upgrade the connection to HTTP/2
		auto h2_conn = co_await http2::upgrade_to_http2(std::move(conn), req);
		if (!h2_conn)
		{
			co_return true;  // Upgrade failed, but we consumed the connection
		}

		// Set up the request handler
		(*h2_conn)->set_handler(
			[this](Request& r) -> Task<Response>
			{
				auto match = router_.match(r.method(), r.path());
				if (match)
				{
					r.set_route_params(std::move(match.params));
				}
				co_return co_await middleware_chain_.execute_or_not_found(r, match.handler);
			});

		// Handle the HTTP/2 connection
		handle_http2_connection(*h2_conn).start_detached();

		co_return true;
	}

	Task<void> App::handle_http2_connection(std::shared_ptr<http2::Http2Connection> h2_conn)
	{
		active_connections_.fetch_add(1, std::memory_order_relaxed);

		try
		{
			co_await h2_conn->run();
		}
		catch (const std::exception& e)
		{
			std::cerr << "HTTP/2 connection error: " << e.what() << '\n';
		}
		catch (...)
		{
			std::cerr << "HTTP/2 connection error: unknown" << '\n';
		}

		active_connections_.fetch_sub(1, std::memory_order_relaxed);
	}
#endif

}  // namespace coroute
