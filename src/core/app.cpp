#include "coroute/core/app.hpp"
#include "coroute/util/zero_copy.hpp"
#include <cstring>
#include <iostream>
#include <sstream>
#include <thread>

namespace coroute {

void App::run(uint16_t port) {
  io_ctx_ = net::IoContext::create(thread_count_);

#ifdef COROUTE_HAS_TLS
  if (tls_enabled_ && tls_ctx_) {
    // TLS mode - use single listener
    listener_ = net::Listener::create(*io_ctx_);
    auto result = listener_->listen(port);
    if (!result) {
      throw std::runtime_error("Failed to listen: " +
                               result.error().to_string());
    }

    std::cout << "Server listening on port " << listener_->local_port()
              << " (HTTPS)" << std::endl;
    tls_listener_ =
        std::make_unique<net::TlsListener>(std::move(listener_), *tls_ctx_);

    // TLS accept loop - use start_detached to keep it alive
    [this]() -> Task<void> {
      while (!cancel_source_.is_cancelled()) {
        auto conn_result = co_await tls_listener_->accept();
        if (!conn_result) {
          if (cancel_source_.is_cancelled())
            break;
          std::cerr << "TLS Accept error: " << conn_result.error().to_string()
                    << std::endl;
          continue;
        }

#ifdef COROUTE_HAS_HTTP2
        // Check ALPN negotiated protocol
        if (http2_enabled_) {
          auto *tls_conn =
              dynamic_cast<net::TlsConnection *>(conn_result->get());
          if (tls_conn) {
            auto proto = tls_conn->negotiated_protocol();
            if (proto && *proto == "h2") {
              // HTTP/2 over TLS - create HTTP/2 connection
              auto h2_conn = std::make_shared<http2::Http2Connection>(
                  std::move(*conn_result));
              h2_conn->set_handler([this](Request &r) -> Task<Response> {
                auto match = router_.match(r.method(), r.path());
                if (match) {
                  r.set_route_params(std::move(match.params));
                }
                co_return co_await middleware_chain_.execute_or_not_found(
                    r, match.handler);
              });
              handle_http2_connection(h2_conn).start_detached();
              continue;
            }
          }
        }
#endif

        handle_connection(std::move(*conn_result)).start_detached();
      }
    }()
                    .start_detached();
  } else
#endif
  {
    // Try to enable multi-accept (SO_REUSEPORT) for better scalability
    // This must be done BEFORE creating a regular listener
    bool multi_accept = io_ctx_->enable_multi_accept(
        port, [this](std::unique_ptr<net::Connection> conn) {
          handle_connection(std::move(conn)).start_detached();
        });

    if (multi_accept) {
      std::cout << "Server listening on port " << port
                << " (multi-accept enabled)" << std::endl;
    } else {
      // Fall back to single-listener accept loop
      listener_ = net::Listener::create(*io_ctx_);
      auto result = listener_->listen(port);
      if (!result) {
        throw std::runtime_error("Failed to listen: " +
                                 result.error().to_string());
      }

      std::cout << "Server listening on port " << listener_->local_port()
                << std::endl;

      // Plain HTTP accept loop - use start_detached to keep it alive
      [this]() -> Task<void> {
        while (!cancel_source_.is_cancelled()) {
          auto conn_result = co_await listener_->async_accept();
          if (!conn_result) {
            if (cancel_source_.is_cancelled())
              break;
            std::cerr << "Accept error: " << conn_result.error().to_string()
                      << std::endl;
            continue;
          }

          handle_connection(std::move(*conn_result)).start_detached();
        }
      }()
                      .start_detached();
    }
  }

  // Run the event loop
  io_ctx_->run();
}

Task<void> App::run_async(uint16_t port) {
  io_ctx_ = net::IoContext::create(thread_count_);
  listener_ = net::Listener::create(*io_ctx_);

  auto result = listener_->listen(port);
  if (!result) {
    throw std::runtime_error("Failed to listen: " + result.error().to_string());
  }

  std::cout << "Server listening on port " << listener_->local_port()
            << std::endl;

  while (!cancel_source_.is_cancelled()) {
    auto conn_result = co_await listener_->async_accept();
    if (!conn_result) {
      if (cancel_source_.is_cancelled()) {
        break;
      }
      std::cerr << "Accept error: " << conn_result.error().to_string()
                << std::endl;
      continue;
    }

    // Handle connection (fire and forget - self-destroys on completion)
    handle_connection(std::move(*conn_result)).start_detached();
  }
}

void App::stop() {
  shutting_down_.store(true, std::memory_order_relaxed);
  cancel_source_.cancel();
  if (listener_) {
    listener_->close();
  }
  if (io_ctx_) {
    io_ctx_->stop();
  }
}

void App::shutdown(ShutdownOptions options) {
  std::cout << "Initiating graceful shutdown..." << std::endl;

  // Mark as shutting down
  shutting_down_.store(true, std::memory_order_relaxed);

  // Stop accepting new connections
  if (listener_) {
    listener_->close();
  }
#ifdef COROUTE_HAS_TLS
  if (tls_listener_) {
    tls_listener_->close();
  }
#endif

  // Wait for existing connections to drain
  auto start = std::chrono::steady_clock::now();
  while (active_connections_.load(std::memory_order_relaxed) > 0) {
    auto elapsed = std::chrono::steady_clock::now() - start;
    if (elapsed >= options.drain_timeout) {
      std::cout << "Drain timeout reached with "
                << active_connections_.load(std::memory_order_relaxed)
                << " connections remaining" << std::endl;
      break;
    }

    // Sleep briefly before checking again
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  // Force close if configured and connections remain
  if (options.force_close_after_timeout &&
      active_connections_.load(std::memory_order_relaxed) > 0) {
    std::cout << "Force closing remaining connections..." << std::endl;
    cancel_source_.cancel();
  }

  // Stop the event loop
  if (io_ctx_) {
    io_ctx_->stop();
  }

  std::cout << "Shutdown complete" << std::endl;
}

#ifdef COROUTE_HAS_TLS
App &App::enable_tls(const AppTlsConfig &config) {
  net::TlsConfig tls_config;
  tls_config.cert_file = config.cert_file;
  tls_config.key_file = config.key_file;
  tls_config.ca_file = config.ca_file;
  tls_config.chain_file = config.chain_file;
  tls_config.verify_client = config.verify_client;

  // Set ALPN protocols - if not specified, use defaults based on HTTP/2 support
  if (config.alpn_protocols.empty()) {
#ifdef COROUTE_HAS_HTTP2
    if (http2_enabled_) {
      tls_config.alpn_protocols = {"h2", "http/1.1"};
    } else {
      tls_config.alpn_protocols = {"http/1.1"};
    }
#else
    tls_config.alpn_protocols = {"http/1.1"};
#endif
  } else {
    tls_config.alpn_protocols = config.alpn_protocols;
  }

  auto ctx_result = net::TlsContext::create(tls_config);
  if (!ctx_result) {
    throw std::runtime_error("Failed to create TLS context: " +
                             ctx_result.error().to_string());
  }

  tls_ctx_ = std::make_unique<net::TlsContext>(std::move(*ctx_result));
  tls_enabled_ = true;

  return *this;
}
#endif

Task<void> App::handle_connection(std::unique_ptr<net::Connection> conn) {
  // Track active connection
  active_connections_.fetch_add(1, std::memory_order_relaxed);

  // RAII guard to decrement on exit
  struct ConnectionGuard {
    std::atomic<size_t> &counter;
    ~ConnectionGuard() { counter.fetch_sub(1, std::memory_order_relaxed); }
  } guard{active_connections_};

  conn->set_cancellation_token(cancel_source_.token());

  // Keep-alive configuration
  constexpr size_t MAX_REQUESTS_PER_CONNECTION = 100;
  constexpr auto KEEP_ALIVE_TIMEOUT = std::chrono::seconds(30);

  size_t request_count = 0;
  bool keep_alive = true;

  // Set connection timeout
  conn->set_timeout(KEEP_ALIVE_TIMEOUT);

  // HTTP/1.1 keep-alive loop
  while (conn->is_open() && !cancel_source_.is_cancelled() && keep_alive) {
    ++request_count;

    // Check max requests limit
    if (request_count > MAX_REQUESTS_PER_CONNECTION) {
      break;
    }

    // Parse request
    auto req_result = co_await parse_request(*conn);
    if (!req_result) {
      if (req_result.error().is_cancelled() ||
          req_result.error().io_error() == IoError::EndOfStream ||
          req_result.error().is_timeout()) {
        break; // Clean disconnect or timeout
      }

      // Send error response
      auto resp = Response::bad_request(req_result.error().to_string());
      auto data = resp.serialize();
      co_await conn->async_write_all(data.data(), data.size());
      break;
    }

    Request &req = *req_result;

    // Check for WebSocket upgrade
    if (co_await try_websocket_upgrade(conn, req)) {
      // WebSocket upgrade handled - connection is now owned by WS handler
      // conn is now null, so we must return without calling conn->close()
      co_return;
    }

#ifdef COROUTE_HAS_HTTP2
    // Check for HTTP/2 upgrade (h2c)
    if (co_await try_http2_upgrade(conn, req)) {
      // HTTP/2 upgrade handled - connection is now owned by HTTP/2 handler
      // conn is now null, so we must return without calling conn->close()
      co_return;
    }
#endif

    // Determine keep-alive based on request
    keep_alive = req.keep_alive();

#ifdef COROUTE_HAS_TEMPLATES
    // Check for view routes first (GET-only)
    if (req.method() == HttpMethod::GET) {
      auto view_match = router_.match_view(req.path());
      if (view_match.handler) {
        // Set route parameters
        req.set_route_params(std::move(view_match.params));

        // Execute view handler
        Response resp;
        try {
          ViewResultAny view_result = co_await (*view_match.handler)(req);

          // Render the view using the web template
          nlohmann::json data = view_result.to_json();
          std::string template_name = view_result.templates.web;
          if (template_name.find('.') == std::string::npos) {
            template_name += ".html";
          }
          resp = render_html(template_name, data);
        } catch (const std::exception &e) {
          resp = Response::internal_error(e.what());
        } catch (...) {
          resp = Response::internal_error("Unknown error");
        }

        // Add Connection header based on keep-alive status
        bool should_close =
            !keep_alive || request_count >= MAX_REQUESTS_PER_CONNECTION;
        if (should_close) {
          resp.set_header("Connection", "close");
          keep_alive = false;
        } else {
          resp.set_header("Connection", "keep-alive");
          resp.set_header(
              "Keep-Alive",
              "timeout=30, max=" +
                  std::to_string(MAX_REQUESTS_PER_CONNECTION - request_count));
        }

        // Send response
        auto data = resp.serialize();
        auto write_result =
            co_await conn->async_write_all(data.data(), data.size());
        if (!write_result) {
          break;
        }

        continue; // Go to next request in keep-alive loop
      }
    }
#endif

    // Route the request
    auto match = router_.match(req.method(), req.path());

    // Set route parameters if matched
    if (match) {
      req.set_route_params(std::move(match.params));
    }

    // Execute handler with pre-compiled middleware chain
    Response resp;
    try {
      resp =
          co_await middleware_chain_.execute_or_not_found(req, match.handler);
    } catch (const std::exception &e) {
      resp = Response::internal_error(e.what());
    } catch (...) {
      resp = Response::internal_error("Unknown error");
    }

    // Add Connection header based on keep-alive status
    bool should_close =
        !keep_alive || request_count >= MAX_REQUESTS_PER_CONNECTION;
    if (should_close) {
      resp.set_header("Connection", "close");
      keep_alive = false;
    } else {
      resp.set_header("Connection", "keep-alive");
      resp.set_header(
          "Keep-Alive",
          "timeout=30, max=" +
              std::to_string(MAX_REQUESTS_PER_CONNECTION - request_count));
    }

    // Send response
    if (resp.has_file() && req.method() != HttpMethod::HEAD) {
      // Zero-copy file response: send headers then file
      auto headers_data = resp.serialize_headers();
      auto write_result = co_await conn->async_write_all(headers_data.data(),
                                                         headers_data.size());
      if (!write_result) {
        break;
      }

      // Send file body via zero-copy
      const auto &file_info = resp.file_info();
      auto transmit_result = co_await send_file_zero_copy(
          *conn, file_info.path, file_info.offset, file_info.length);

      // Fallback: if transmit fails, we can't recover easily
      // In production, you'd want better error handling
      if (!transmit_result) {
        break;
      }
    } else {
      // Normal response with body in memory
      auto data = resp.serialize();
      auto write_result =
          co_await conn->async_write_all(data.data(), data.size());
      if (!write_result) {
        break;
      }
    }
  }

  conn->close();
}

// URL decode helper
static std::string url_decode(std::string_view str) {
  std::string result;
  result.reserve(str.size());

  for (size_t i = 0; i < str.size(); ++i) {
    if (str[i] == '%' && i + 2 < str.size()) {
      // Decode %XX
      char hex[3] = {str[i + 1], str[i + 2], '\0'};
      char *end;
      long val = std::strtol(hex, &end, 16);
      if (end == hex + 2) {
        result += static_cast<char>(val);
        i += 2;
        continue;
      }
    } else if (str[i] == '+') {
      result += ' ';
      continue;
    }
    result += str[i];
  }

  return result;
}

Task<expected<Request, Error>> App::parse_request(net::Connection &conn) {
  // HTTP request parser with improved efficiency and validation

  constexpr size_t MAX_HEADER_SIZE = 8192;
  constexpr size_t MAX_BODY_SIZE = 10 * 1024 * 1024; // 10MB default max
  constexpr size_t READ_CHUNK_SIZE = 1024;

  // Use pooled buffer for reduced allocations
  auto buffer_ptr = buffer_pool_.acquire(MAX_HEADER_SIZE);
  auto &buffer = *buffer_ptr;
  buffer.resize(MAX_HEADER_SIZE);

  // RAII guard to return buffer to pool
  struct BufferGuard {
    BufferPool &pool;
    std::unique_ptr<std::vector<char>> buf;
    ~BufferGuard() {
      if (buf)
        pool.release(std::move(buf));
    }
  } buffer_guard{buffer_pool_, std::move(buffer_ptr)};
  size_t total_read = 0;
  size_t header_end_pos = std::string::npos;

  // Read headers in chunks for better efficiency
  while (total_read < MAX_HEADER_SIZE) {
    size_t to_read = std::min(READ_CHUNK_SIZE, MAX_HEADER_SIZE - total_read);
    auto result = co_await conn.async_read(buffer.data() + total_read, to_read);
    if (!result) {
      co_return unexpected(result.error());
    }

    size_t bytes_read = *result;
    if (bytes_read == 0) {
      co_return unexpected(
          Error::io(IoError::EndOfStream, "Connection closed"));
    }

    // Search for end of headers in newly read data
    size_t search_start = (total_read >= 3) ? total_read - 3 : 0;
    total_read += bytes_read;

    for (size_t i = search_start; i + 3 < total_read; ++i) {
      if (buffer[i] == '\r' && buffer[i + 1] == '\n' && buffer[i + 2] == '\r' &&
          buffer[i + 3] == '\n') {
        header_end_pos = i + 4;
        break;
      }
    }

    if (header_end_pos != std::string::npos) {
      break;
    }
  }

  if (header_end_pos == std::string::npos) {
    co_return unexpected(
        Error::http(HttpError::PayloadTooLarge, "Headers too large"));
  }

  // Parse the request
  std::string_view data(buffer.data(), total_read);
  Request req;

  // Find request line
  auto line_end = data.find("\r\n");
  if (line_end == std::string_view::npos) {
    co_return unexpected(
        Error::http(HttpError::BadRequest, "Invalid request line"));
  }

  std::string_view request_line = data.substr(0, line_end);

  // Parse method
  auto method_end = request_line.find(' ');
  if (method_end == std::string_view::npos) {
    co_return unexpected(
        Error::http(HttpError::BadRequest, "Invalid request line"));
  }
  req.set_method(request_line.substr(0, method_end));

  // Parse path
  auto path_start = method_end + 1;
  auto path_end = request_line.find(' ', path_start);
  if (path_end == std::string_view::npos) {
    co_return unexpected(
        Error::http(HttpError::BadRequest, "Invalid request line"));
  }

  std::string_view path_with_query =
      request_line.substr(path_start, path_end - path_start);

  // Split path and query string
  auto query_start = path_with_query.find('?');
  if (query_start != std::string_view::npos) {
    // URL decode the path
    req.set_path(url_decode(path_with_query.substr(0, query_start)));
    req.set_query_string(std::string(path_with_query.substr(query_start + 1)));

    // Parse query parameters with URL decoding
    std::string_view qs = path_with_query.substr(query_start + 1);
    while (!qs.empty()) {
      auto amp = qs.find('&');
      std::string_view param =
          (amp != std::string_view::npos) ? qs.substr(0, amp) : qs;

      auto eq = param.find('=');
      if (eq != std::string_view::npos) {
        req.add_query_param(url_decode(param.substr(0, eq)),
                            url_decode(param.substr(eq + 1)));
      } else if (!param.empty()) {
        // Parameter without value
        req.add_query_param(url_decode(param), "");
      }

      if (amp == std::string_view::npos)
        break;
      qs = qs.substr(amp + 1);
    }
  } else {
    req.set_path(url_decode(path_with_query));
  }

  // Parse HTTP version
  auto version_start = path_end + 1;
  req.set_http_version(std::string(request_line.substr(version_start)));

  // Parse headers
  size_t pos = line_end + 2;
  while (pos < data.size()) {
    auto header_end = data.find("\r\n", pos);
    if (header_end == std::string_view::npos || header_end == pos) {
      break; // End of headers
    }

    std::string_view header_line = data.substr(pos, header_end - pos);
    auto colon = header_line.find(':');
    if (colon != std::string_view::npos) {
      std::string_view key = header_line.substr(0, colon);
      std::string_view value = header_line.substr(colon + 1);

      // Trim leading whitespace from value
      while (!value.empty() && value[0] == ' ') {
        value = value.substr(1);
      }

      req.add_header(std::string(key), std::string(value));
    }

    pos = header_end + 2;
  }

  // Read body if Content-Length is present
  auto content_length = req.content_length();
  if (content_length && *content_length > 0) {
    // Validate body size
    if (*content_length > MAX_BODY_SIZE) {
      co_return unexpected(Error::http(HttpError::PayloadTooLarge,
                                       "Request body too large (max " +
                                           std::to_string(MAX_BODY_SIZE) +
                                           " bytes)"));
    }

    std::string body(*content_length, '\0');
    size_t body_read = 0;

    // Check if we already read some body data while reading headers
    size_t body_in_buffer = total_read - header_end_pos;
    if (body_in_buffer > 0) {
      size_t to_copy = std::min(body_in_buffer, *content_length);
      std::memcpy(body.data(), buffer.data() + header_end_pos, to_copy);
      body_read = to_copy;
    }

    // Read remaining body
    while (body_read < *content_length) {
      auto result = co_await conn.async_read(body.data() + body_read,
                                             *content_length - body_read);
      if (!result) {
        co_return unexpected(result.error());
      }
      if (*result == 0) {
        co_return unexpected(
            Error::http(HttpError::BadRequest, "Incomplete request body"));
      }
      body_read += *result;
    }

    req.set_body(std::move(body));

    // Parse form body as query parameters if content type is form-urlencoded
    auto ct = req.content_type();
    if (ct && ct->find("application/x-www-form-urlencoded") !=
                  std::string_view::npos) {
      std::string_view form_body = req.body();
      while (!form_body.empty()) {
        auto amp = form_body.find('&');
        std::string_view param = (amp != std::string_view::npos)
                                     ? form_body.substr(0, amp)
                                     : form_body;

        auto eq = param.find('=');
        if (eq != std::string_view::npos) {
          req.add_query_param(url_decode(param.substr(0, eq)),
                              url_decode(param.substr(eq + 1)));
        } else if (!param.empty()) {
          // Parameter without value
          req.add_query_param(url_decode(param), "");
        }

        if (amp == std::string_view::npos)
          break;
        form_body = form_body.substr(amp + 1);
      }
    }
  }

  co_return req;
}

Task<bool> App::try_websocket_upgrade(std::unique_ptr<net::Connection> &conn,
                                      Request &req) {
  // Check if this is a WebSocket upgrade request
  if (!net::is_websocket_upgrade(req)) {
    co_return false;
  }

  // Check if we have a handler for this path
  auto it = ws_handlers_.find(std::string(req.path()));
  if (it == ws_handlers_.end()) {
    co_return false;
  }

  // Upgrade the connection
  auto ws_conn = co_await net::upgrade_to_websocket(std::move(conn), req);
  if (!ws_conn) {
    // Upgrade failed - connection is now invalid
    co_return true; // Return true to indicate we handled it (even if failed)
  }

  // Call the WebSocket handler
  try {
    co_await it->second(std::move(*ws_conn));
  } catch (const std::exception &e) {
    std::cerr << "WebSocket handler error: " << e.what() << std::endl;
  } catch (...) {
    std::cerr << "WebSocket handler error: unknown" << std::endl;
  }

  co_return true;
}

#ifdef COROUTE_HAS_HTTP2
Task<bool> App::try_http2_upgrade(std::unique_ptr<net::Connection> &conn,
                                  Request &req) {
  if (!http2_enabled_) {
    co_return false;
  }

  // Check if this is an h2c upgrade request
  if (!http2::is_h2c_upgrade_request(req)) {
    co_return false;
  }

  // Upgrade the connection to HTTP/2
  auto h2_conn = co_await http2::upgrade_to_http2(std::move(conn), req);
  if (!h2_conn) {
    co_return true; // Upgrade failed, but we consumed the connection
  }

  // Set up the request handler
  (*h2_conn)->set_handler([this](Request &r) -> Task<Response> {
    auto match = router_.match(r.method(), r.path());
    if (match) {
      r.set_route_params(std::move(match.params));
    }
    co_return co_await middleware_chain_.execute_or_not_found(r, match.handler);
  });

  // Handle the HTTP/2 connection
  handle_http2_connection(*h2_conn).start_detached();

  co_return true;
}

Task<void>
App::handle_http2_connection(std::shared_ptr<http2::Http2Connection> h2_conn) {
  active_connections_.fetch_add(1, std::memory_order_relaxed);

  try {
    co_await h2_conn->run();
  } catch (const std::exception &e) {
    std::cerr << "HTTP/2 connection error: " << e.what() << std::endl;
  } catch (...) {
    std::cerr << "HTTP/2 connection error: unknown" << std::endl;
  }

  active_connections_.fetch_sub(1, std::memory_order_relaxed);
}
#endif

} // namespace coroute
