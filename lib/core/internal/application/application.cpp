#include "application.hpp"

namespace webframe::core {
	application::application() : routes(&executors) {
#ifdef USE_INJA
		template_dir = ".";
#endif
		performancer = utils::synchronized_file(&std::clog);
		logger = utils::info_synchronized_file(&std::cout);
		warn = utils::warning_synchronized_file(&std::clog);
		errors = utils::error_synchronized_file(&std::cerr);

		this->handle("404", [&](const std::string& path) { return "Error 404: " + path + " was not found."; })
		    .handle("500",
		            [&](const std::string& reason) { return "Error 500: Internal server error: " + reason + "."; });
	}
	constexpr bool application::initHttpCodes([[maybe_unused]] const unsigned int code) {
		/*std::visit(
		            [](auto code) {
		                    constexpr auto [[maybe_unused]] _1 =
		http_codes::get_reason_by_code(_compile_time::codes[code]); constexpr auto [[maybe_unused]] _2 =
		http_codes::get_reason_by_code(_compile_time::strCodes[code]);
		            },
		            _compile_time::var_index<_compile_time::codes.size()>(code));

		return (code + 1 >= _compile_time::codes.size()) ? true : initHttpCodes(code + 1);
		*/

		return true;
	}
	constexpr bool application::init() {
		static_assert(mime_types::get_mime_type(".zip").size() > 0, "mime_types were not initialized.");

		// static_assert(method_to_string(method::GET) == "GET"sv);
		// static_assert(method_to_string(method::HEAD) == "HEAD"sv);
		// static_assert(method_to_string(method::POST) == "POST"sv);
		// static_assert(method_to_string(method::PUT) == "PUT"sv);
		// static_assert(method_to_string(method::DDELETE) == "DELETE"sv);
		// static_assert(method_to_string(method::CONNECT) == "CONNECT"sv);
		// static_assert(method_to_string(method::OPTIONS) == "OPTIONS"sv);
		// static_assert(method_to_string(method::TRACE) == "TRACE"sv);
		// static_assert(method_to_string(method::PATCH) == "PATCH"sv);
		// static_assert(string_to_method("GET") == method::GET);
		// static_assert(string_to_method("HEAD") == method::HEAD);
		// static_assert(string_to_method("POST") == method::POST);
		// static_assert(string_to_method("PUT") == method::PUT);
		// static_assert(string_to_method("DELETE") == method::DDELETE);
		// static_assert(string_to_method("CONNECT") == method::CONNECT);
		// static_assert(string_to_method("OPTIONS") == method::OPTIONS);
		// static_assert(string_to_method("TRACE") == method::TRACE);
		// static_assert(string_to_method("PATCH") == method::PATCH);

		static_assert(initHttpCodes(), "The initiation of HTTP code and their reasons failed");

		return true;
	}

	template <typename F>
	application& application::handle(std::string code, F _res) {
		const auto res = wrap(_res);
		this->executors.push_back(executor_data(code, {}, utils::responser(res)));
		this->routes.add_regex(code, executors.size() - 1);
		return *this;
	}

	template <typename Ret, typename... Ts>
	application& application::handle(std::string code, std::function<Ret(Ts...)> const& res) {
		this->executors.push_back(executor_data(code, {}, utils::responser(res)));
		this->routes.add_regex(code, executors.size() - 1);
		return *this;
	}

	application& application::set_performancer(std::ostream* _performancer) {
		performancer = utils::synchronized_file(_performancer);
		return *this;
	}

	application& application::set_logger(std::ostream* _logger) {
		logger = utils::info_synchronized_file(_logger);
		return *this;
	}

	application& application::set_warner(std::ostream* _logger) {
		warn = utils::warning_synchronized_file(_logger);
		return *this;
	}

	application& application::set_error_logger(std::ostream* _logger) {
		errors = utils::error_synchronized_file(_logger);
		return *this;
	}

	application& application::set_static(const std::string& path, const std::string& alias) {
		// std::filesystem::path p = std::filesystem::relative(path);
		this->route(alias + "/{path}",
		            [path, this](const std::string& file) { return this->get_file(path + "/" + file); });
		return *this;
	}

	response application::get_file(const std::string& path) {
		std::string ext = std::filesystem::path(path).extension().string();
		const std::string mime = mime_types::get_mime_type(ext.c_str()).data();
		std::ifstream ifs(path, std::ios::in | std::ios::binary);
		if (!ifs.is_open()) {
			if (!this->routes.match("404").has_value())
				throw std::invalid_argument("404 page not found in the routes.");
			path_vars p;
			p += path_vars::var(path, "string");
			return this->routes.match("404").value().executor.call("1.1", "", p);
		} else {
			std::ostringstream oss;
			oss << ifs.rdbuf();
			std::string content(oss.str());
			std::map<std::string, std::string> m = {{"Content-type", mime},
			                                        {"Accept-Ranges", "bytes"},
			                                        {"Cache-Control", "public, max-age=10"},
			                                        {"Connection", "keep-alive"},
			                                        {"Keep-Alive", "timeout=5"}};
			return response(status_line("1.1", "200"), m, content);
		}
	}

	application& application::route(const std::string& path, utils::responser res) {
		auto x = convert_path_to_regex(path);
		this->executors.push_back(executor_data(x.first, x.second, res));
		this->routes.add_regex(x.first, executors.size() - 1);
		return *this;
	}

	template <typename Ret, typename... Ts>
	application& application::route(const std::string& path, std::function<Ret(Ts...)> const& res) {
		return this->route(path, utils::responser(res));
	}

	template <typename F>
	application& application::route(const std::string& path, F res) {
		return this->route(path, wrap(res));
	}

	application& application::extend_with(const router& set_of_routes, const std::string& prefix) {
		for (const auto& route : set_of_routes.routes) {
			this->route(prefix + route.first, route.second);
		}
		return *this;
	}

	response application::respond(const std::string& path, const std::string& http) {
		auto matches = routes.match(path);
		if (!matches.has_value()) {
			const auto route404 = routes.match("404");
			if (!route404.has_value()) throw std::invalid_argument("404 page not found in the routes.");
			path_vars p;
			p += path_vars::var(path, "string");
			return route404.value().executor.call(http, "", p);
		}
		auto match = matches.value();
		std::smatch pieces_match;
		std::regex pieces_regex(match.matcher);
		if (std::regex_match(path, pieces_match, pieces_regex)) {
			std::ssub_match sub_match;
			path_vars params;
			for (size_t i = 1; i < pieces_match.size(); ++i) {
				sub_match = pieces_match[i];
				params += {sub_match.str(), match.vars[i - 1]};
			}
			return match.executor.call(http, "", params);
		} else {
			throw std::invalid_argument(std::string("URLMatcher matched regex that does not match.") + path +
			                            " doesn't match " + match.regex);
		}
	}

	response application::respond(const request& req, const std::string& http) { return respond(req.uri, http); }

	response application::respond(const char* p, const std::string& http) { return respond(std::string(p), http); }

#ifdef USE_INJA
	application& application::set_templates(const std::string& path) {
		this->template_dir = path;
		return *this;
	}

	response application::render(std::string path, inja::json params) {
		path = this->template_dir + "/" + path;
		try {
			return response(this->env.render_file(path, params));
		} catch (...) {
			// file not found
			return this->routes.match("404").value().executor.call("1.1", "", {{path, "string"}});
		}
	}
#endif

	void application::wait_start(const char* PORT) { application::port_status.wait_to_start(PORT); }

	void application::wait_end(const char* PORT) { application::port_status.wait_to_stop(PORT); }

	void application::request_stop(const char* PORT) { application::port_status.signal_to_stop(PORT); }

	int application::responder(SOCKET socket) {
		request r;
		try {
			const std::size_t capacity = 1024;
			char data[capacity];
			int total_recv = 0;
			int n = 0;
			do {
				n = RECV(socket, data, capacity, 0);
				if (n < 0) {
					break;
				}

				total_recv += n;

				auto state = r.loadMore(data, n);

				if (state == LoadingState::LOADED)  // Headers are loaded, the buffer
				                                    // is not full and the last char
				                                    // is \0 -> body is filled
					break;
			} while (n > 0);

			this->logger << "(responder) Read state: " << (int)r.getState() << " " << total_recv << "\n";

			if (r.getState() != LoadingState::LOADED)
				throw std::string(
				    "Request was not loaded completely and data with "
				    "size=" +
				    std::to_string(total_recv) + " was sent.");

			const auto t1 = std::chrono::system_clock::now();
			response res;
			res = this->respond(r, r.http);
			const std::string& response = res.to_string();
			const size_t responseSize = response.size();
			int status;
			status = SEND(socket, response.c_str(), responseSize, 0);
			if (status == SOCKET_ERROR) {
				return -1;
			}
			this->performancer << r.uri << ": " << timer(t1).count() << "miliseconds\n";
		} catch (std::exception const& e) {
			this->errors << "(responder) Responding Exception: " << e.what() << "\n";
			if (!this->routes.match("500").has_value()) throw std::invalid_argument("500 pge not found.");
			response res =
			    this->routes.match("500").value().executor.call(r.http, "", {{std::string(e.what()), "string"}});
			const std::string& response = res.to_string();
			const size_t responseSize = response.size();
			SEND(socket, response.c_str(), responseSize, 0);
			return -1;
		} catch (...) {
			return -2;
		}
		return 0;
	}

	void application::handler(SOCKET client, const std::function<void()>& callback) {
		int status = this->responder(client);
		this->logger << "(handler) Responded status: " << status << "\n";
		CLOSE(client);
		this->logger << "(handler) Closing client: " << client << "\n";
		if (status != -2) {
			this->logger << "(handler) Calling callback"
			             << "\n";
			callback();
			this->logger << "(handler) Callback done"
			             << "\n";
		}
	}
	bool application::initialized_sockets() {
#ifdef _WIN32
		// Initialize Winsock.
		WSADATA wsaData;
		this->logger << "(main) Startup called\n";
		int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
		this->logger << "(main) Startup finished " << iResult << "\n";
		if (iResult != NO_ERROR) {
			this->errors << "(main) WSAStartup failed with error: " << iResult << "\n";
			return false;
		}
#endif
		return true;
	}

	SOCKET application::get_listener(const char* PORT) {
		int status;
		struct addrinfo hints, *res;
		SOCKET listener;

		// Before using hint you have to make sure that the data
		// structure is empty
		memset(&hints, 0, sizeof hints);
		// Set the attribute for hint
		hints.ai_family = AF_INET;        // We don't care V4 AF_INET or 6 AF_INET6
		hints.ai_socktype = SOCK_STREAM;  // TCP Socket SOCK_DGRAM
		hints.ai_flags = AI_PASSIVE;
		hints.ai_protocol = 0; /* Any protocol */
		hints.ai_canonname = NULL;
		hints.ai_addr = NULL;
		hints.ai_next = NULL;

		// Fill the res data structure and make sure that the
		// results make sense.
		status = getaddrinfo(NULL, PORT, &hints, &res);
		if (status != 0) {
			this->errors << "(main) getaddrinfo error: " << gai_strerror(status) << "\n";
			application::port_status.change_status(PORT, webframe::utils::server_status::port::Status::STARTED);
			application::port_status.change_status(PORT, webframe::utils::server_status::port::Status::STOPPED);
			throw exceptions::unable_to_retrieve_address();
		}

		// Create Socket and check if error occured afterwards
		listener = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
		if (listener == INVALID_SOCKET) {
			this->errors << "(main) socket error: " << gai_strerror(status) << "\n";
			application::port_status.change_status(PORT, webframe::utils::server_status::port::Status::STARTED);
			application::port_status.change_status(PORT, webframe::utils::server_status::port::Status::STOPPED);
			throw exceptions::socket_not_created();
		}

		// Bind the socket to the address of my local machine and
		// port number
		status = bind(listener, res->ai_addr, sizeof(*res->ai_addr) /*res->ai_addrlen*/);
		if (status < 0) {
			this->errors << "(main) bind error: " << status << " " << gai_strerror(status) << "\n";
			application::port_status.change_status(PORT, webframe::utils::server_status::port::Status::STARTED);
			application::port_status.change_status(PORT, webframe::utils::server_status::port::Status::STOPPED);
			throw exceptions::unable_to_bind_socket();
		}

		status = listen(listener, 128);
		if (status < 0) {
			this->errors << "(main) listen error: " << gai_strerror(status) << "\n";
			application::port_status.change_status(PORT, webframe::utils::server_status::port::Status::STARTED);
			application::port_status.change_status(PORT, webframe::utils::server_status::port::Status::STOPPED);
			throw exceptions::unable_to_bind_socket();
		}

		// status = nonblock_config(listener);
		status = nonblock_config(listener);
		if (status < 0) {
			this->errors << "(main) nonblocking config error: " << gai_strerror(status) << "\n";
			application::port_status.change_status(PORT, webframe::utils::server_status::port::Status::STARTED);
			application::port_status.change_status(PORT, webframe::utils::server_status::port::Status::STOPPED);
			throw exceptions::socket_not_created();
		}

		// Free the res linked list after we are done with it
		freeaddrinfo(res);
		application::port_status.change_status(PORT, webframe::utils::server_status::port::Status::STARTED);
		return listener;
	}

	utils::generator<SOCKET> application::gen_clients(SOCKET listener, const std::string PORT,
	                                                  std::function<void()> on_end) {
		std::mutex m;
		while (application::port_status.get_status(PORT.c_str()) !=
		       webframe::utils::server_status::port::Status::SIGNALED) {
			std::lock_guard<std::mutex> lk(m);
			// Accept a request
			SOCKET client = ACCEPT(listener, NULL, NULL);
			if (client == INVALID_SOCKET) {
				continue;
			}
			this->logger << "(thread " << std::this_thread::get_id()
			             << ") Client invalid: " << (client == INVALID_SOCKET) << "\n";
			this->logger << "(thread " << std::this_thread::get_id() << ") Client found: " << client << "\n";

			// Check if the socket is valid
			{
				struct timeval selTimeout;
				selTimeout.tv_sec = 1;
				selTimeout.tv_usec = 0;
				fd_set readSet;
				FD_ZERO(&readSet);
				FD_SET(client + 1, &readSet);
				FD_SET(client, &readSet);

				int status = SELECT(client + 1, &readSet, nullptr, nullptr, &selTimeout);
				this->logger << "(thread " << std::this_thread::get_id() << ") SELECT status is " << status << "\n";
				if (status < 0) {
					this->errors << "(thread " << std::this_thread::get_id() << ") INVALID SOCKET: " << client
					             << " was skipped (" << status << ")\n";
					continue;
				}

				this->logger << "(thread " << std::this_thread::get_id() << ") Client " << client
				             << " vs invalid socket " << INVALID_SOCKET << "\n";
				this->logger << "(thread " << std::this_thread::get_id() << ") Requestor " << client
				             << " is still valid\n";

				co_yield client;
			}
		}
		on_end();
	}

	application& application::run(const char* PORT, const size_t cores, std::optional<size_t> requests) {
		application::port_status.initiate(PORT, requests);
		if (!initialized_sockets()) {
			application::port_status.change_status(PORT, webframe::utils::server_status::port::Status::STARTED);
			application::port_status.change_status(PORT, webframe::utils::server_status::port::Status::STOPPED);
			return *this;
		}
		this->logger << "(main) Startup done\n";
		SOCKET listener = get_listener(PORT);
		this->logger << "(main) Listener setup " << listener << "\n";
		auto get_clients = gen_clients(listener, std::string(PORT), [port = std::string(PORT)]() -> void {
			application::port_status.change_status(port.c_str(), webframe::utils::server_status::port::Status::STOPPED);
#ifdef _WIN32
			WSACleanup();
#endif
		});
		const size_t threads = my_min(cores, requests.value_or(cores));

		thread_pool pool = thread_pool(
		    threads, std::make_shared<utils::generator<SOCKET>>(std::move(get_clients)),
		    [this](std::shared_ptr<utils::generator<SOCKET>> get_client,
		           std::shared_ptr<std::optional<size_t>> requests) {
			    this->logger << "(thread " << std::this_thread::get_id() << ") Thread started\n";
			    do {
				    SOCKET client = get_client->getNextValue();
				    this->logger << "(thread " << std::this_thread::get_id() << ") Client found: " << client << "\n";
				    this->logger << "(thread " << std::this_thread::get_id() << ") Requestor " << client
				                 << " is getting handled\n";

				    this->handler(client, [this, requests]() {
					    if (!requests->has_value()) return;
					    requests->value()--;
					    this->logger << "(thread " << std::this_thread::get_id()
					                 << ") (callback) Requests: " << requests->value() << "\n";
				    });
			    } while (true);
		    },
		    application::port_status.requests_left(PORT));
		this->logger << "(main) Thread pool generated\n";
		application::port_status.change_status(PORT, webframe::utils::server_status::port::Status::STARTED);
		pool.start();
		this->logger << "(main) Thread pool started\n";
		return *this;
	}

}  // namespace webframe::core