#pragma once 
#include "core.hpp"
namespace webframe::core
{
	class application
	{
	private:
		_application* app;
	public:
		application()
		{
			app = new _application();
		}

		~application()
		{
			delete app;
		}

		static constexpr bool initHttpCodes([[maybe_unused]] const unsigned int code = 0) {
			return _application::initHttpCodes();
		}

		static constexpr bool init()
		{
			return _application::init();
		}

		template <typename F>
		inline application& handle(std::string code, F _res)
		{
			app->handle(code, _res);
			return *this;
		}

		template <typename Ret, typename... Ts>
		inline application& handle(std::string code, std::function<Ret(Ts...)> const& res)
		{
			app->handle(code, res);
			return *this;
		}

		inline application& set_performancer(std::ostream* _performancer)
		{
			app->set_performancer(_performancer);
			return *this;
		}

		inline application& set_logger(std::ostream* _logger)
		{
			app->set_logger(_logger);
			return *this;
		}

		inline application& set_warner(std::ostream* _logger)
		{
			app->set_warner(_logger);
			return *this;
		}

		inline application& set_error_logger(std::ostream* _logger)
		{
			app->set_error_logger(_logger);
			return *this;
		}

		inline application& set_static(const std::string& path, const std::string& alias)
		{
			app->set_static(path, alias);
			return *this;
		}

#ifdef USE_INJA
		inline application& set_templates(const std::string& path)
		{
			app->set_templates(path);
			return *this;
		}
#endif
		inline response get_file(const std::string& path)
		{
			return app->get_file(path);
		}

#ifdef USE_INJA
		inline response render(std::string path, inja::json params = {})
		{
			return app->render(path, params);
		}
#endif

		template <typename Ret, typename... Ts>
		inline application& route(const std::string& path, std::function<Ret(Ts...)> const& res)
		{
			app->route(path, res);
			return *this;
		}

		template <typename F>
		inline application& route(const std::string& path, F _res)
		{
			app->route(path, _res);
			return *this;
		}
	public:
		inline application& extend_with(const router& set_of_routes, const std::string& prefix = "")
		{
			app->extend_with(set_of_routes, prefix);
			return *this;
		}

		inline response respond(const std::string& path, const std::string& http = "1.1")
		{
			return app->respond(path, http);
		}

		inline response respond(const request& req, const std::string& http = "1.1")
		{
			return app->respond(req, http);
		}

		inline response respond(const char* p, const std::string& http = "1.1")
		{
			return app->respond(p, http);
		}

		inline application& run(const char* PORT, const unsigned int cores, bool limited = false, int requests = -1)
		{
			app->run(PORT, cores, limited, requests);
			return *this;
		}

		static void wait_start(const char* PORT)
		{
			webframe::core::_application::port_status.get_start(PORT).lock();
		}

		static void wait_end(const char* PORT)
		{
			webframe::core::_application::port_status.get_end(PORT).lock();
		}

		static void request_stop(const char* PORT)
		{
			webframe::core::_application::port_status.alert_end(PORT);
		}

		static void reset(const char* PORT)
		{
			webframe::core::_application::port_status.reset(PORT);
		}
		friend class _application;
	};
}