#pragma once

#include "config.hpp"
#include "coroute/core/app.hpp"

// Forward declarations
namespace flutter_project::services
{
	class TaskService;
	class UserService;
}  // namespace flutter_project::services

namespace flutter_project::handlers::websocket
{
	class TaskHub;
}

namespace flutter_project
{

	class Server
	{
	public:
		explicit Server(Config config);
		~Server();

		// Start the server (blocking, server mode - binds TCP port)
		void run();

		// Start in client/FFI mode (blocking, no TCP listener)
		void run_client();

		// Stop the server
		void stop();

		// Access the underlying App (for testing)
		coroute::App& app() { return app_; }

	private:
		void setup_middleware();
		void setup_routes();
		void setup_error_handlers();

		Config config_;
		coroute::App app_;

		// Services (owned by server, shared with handlers)
		// Order matters for initialization!
		std::unique_ptr<handlers::websocket::TaskHub> task_hub_;
		std::unique_ptr<services::UserService> user_service_;
		std::unique_ptr<services::TaskService> task_service_;
	};

}  // namespace flutter_project
