#include "server.hpp"

#include "middleware/request_logger.hpp"
#include "handlers/pages.hpp"
#include "handlers/api/tasks.hpp"
#include "handlers/api/users.hpp"
#include "handlers/websocket/task_hub.hpp"
#include "services/task_service.hpp"
#include "services/user_service.hpp"

#include "coroute/core/static_files.hpp"
#include "coroute/core/compression.hpp"

#include <iostream>

namespace flutter_project
{

	Server::Server(Config config)
		: config_(std::move(config)),
		  task_hub_(std::make_unique<handlers::websocket::TaskHub>()),
		  user_service_(std::make_unique<services::UserService>()),
		  task_service_(std::make_unique<services::TaskService>(*task_hub_))
	{
		// Configure templates (web server mode)
#ifdef COROUTE_HAS_TEMPLATES
		app_.set_templates(config_.template_dir);
#endif

		setup_middleware();
		setup_error_handlers();
		setup_routes();
	}

	Server::~Server() = default;

	void Server::setup_middleware()
	{
		// Request logging (outermost - runs first)
		app_.use(middleware::request_logger());

		// Response compression
		app_.use(coroute::compression());
	}

	void Server::setup_error_handlers()
	{
		// Default 404 response is used; custom error handlers can be added here
	}

	void Server::setup_routes()
	{
		// Static files (CSS, JS, images) - web mode only
		coroute::StaticFileOptions static_opts;
		static_opts.root = config_.static_dir;
		static_opts.url_prefix = "/static";
		static_opts.directory_listing = false;
		static_opts.max_age_seconds = 3600;
		app_.use(coroute::static_files(static_opts));

		// View routes (page rendering - web: Inja HTML, mobile/desktop: Flutter screens)
		handlers::pages::register_routes(app_, *user_service_, *task_service_);

		// API routes (business logic - shared across all platforms)
		handlers::api::users::register_routes(app_, *user_service_);
		handlers::api::tasks::register_routes(app_, *task_service_);

		// WebSocket (real-time updates - web server mode)
		handlers::websocket::register_routes(app_, *task_hub_);
	}

	void Server::run()
	{
		std::cout << "=================================\n";
		std::cout << "  Task Dashboard (Cross-Platform)\n";
		std::cout << "=================================\n";
		std::cout << "Listening on: http://" << config_.host << ":" << config_.port << "\n";
		std::cout << "Static files: " << config_.static_dir << "\n";
		std::cout << "Templates:    " << config_.template_dir << "\n";
		std::cout << "\nPress Ctrl+C to stop.\n";
		std::cout << "=================================\n\n";

		app_.run(config_.port);
	}

	void Server::run_client()
	{
		std::cout << "[FlutterProject] Running in FFI/client mode — remote server: " << config_.api_base_url << "\n";
		app_.run(coroute::RunOptions{.api_domain = config_.api_base_url});
	}

	void Server::stop() { app_.stop(); }

}  // namespace flutter_project
