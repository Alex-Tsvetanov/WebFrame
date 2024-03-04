#pragma once

#include "../../core.hpp"
#include "../exceptions/socket_not_created.hpp"
#include "../exceptions/unable_to_bind_to_socket.hpp"
#include "../exceptions/unable_to_retrieve_address.hpp"
#include <optional>

namespace {
    using route_variables_container = std::vector<std::string>;
    using regex_id_t = size_t;

    struct executor_data {
		std::string regex;
        std::regex matcher;
        route_variables_container vars;
        webframe::utils::responser executor;
        executor_data() {}
        executor_data(std::string a, route_variables_container b, webframe::utils::responser c) : regex(a), matcher(std::regex(a)), vars(b), executor(c)
        {}
    };
    
    std::chrono::duration<double, std::milli> timer(
        const std::chrono::time_point<
            std::chrono::system_clock,
            std::chrono::duration<long long int, std::ratio<1, 1000000000>>>
            start) {
        return std::chrono::duration<double, std::milli>(
            std::chrono::system_clock::now() - start);
    }
}

namespace webframe::core {
	class application {
	private:
		std::vector<executor_data> executors;
		RegexMatcher<executor_data> routes;

#ifdef USE_INJA
		std::string template_dir;
		inja::Environment env;
#endif

		utils::synchronized_file performancer;
		utils::info_synchronized_file logger;
		utils::warning_synchronized_file warn;
		utils::error_synchronized_file errors;

	public:
		application();

		static constexpr bool initHttpCodes([[maybe_unused]] const unsigned int code = 0);
		static constexpr bool init();

		template <typename F>
		application& handle(std::string code, F _res);

		template <typename Ret, typename... Ts>
		application& handle(std::string code, std::function<Ret(Ts...)> const& res);

		inline application& set_performancer(std::ostream* _performancer);

		inline application& set_logger(std::ostream* _logger);

		inline application& set_warner(std::ostream* _logger);

		inline application& set_error_logger(std::ostream* _logger);

		application& set_static(const std::string& path,
		                         const std::string& alias);

#ifdef USE_INJA
		application& set_templates(const std::string& path);
		response render(std::string path, inja::json params = {});
#endif

		response get_file(const std::string& path);

		template <typename Ret, typename... Ts>
		inline application& route(const std::string& path,
		                    std::function<Ret(Ts...)> const& res);

		template <typename F>
		inline application& route(const std::string& path, F _res);

	private:
		application& route(const std::string& path, utils::responser res);

	public:
		application& extend_with(const router& set_of_routes,
		                          const std::string& prefix = "");

		response respond(const std::string& path,
		                 const std::string& http = "1.1");

		inline response respond(const request& req,
		                        const std::string& http = "1.1");

		inline response respond(const char* p,
		                        const std::string& http = "1.1");

	private:
		int responder(SOCKET socket);
		void handler(SOCKET client, const std::function<void()>& callback);
		bool initialized_sockets();
		SOCKET get_listener(const char* PORT);
		utils::generator<SOCKET> gen_clients(SOCKET listener, const std::string PORT, std::function<void()> on_end);
	public:
		application& run(const char* PORT, const size_t cores, std::optional<size_t> requests = std::nullopt);

		static void wait_start(const char* PORT);

		static void wait_end(const char* PORT);

		static void request_stop(const char* PORT);

	protected:
		static webframe::utils::server_status port_status;

	public:
		friend class router;
	};
	webframe::utils::server_status application::port_status = webframe::utils::server_status();

}  // namespace webframe::core

#include "application.cpp"
