#include "app/server.hpp"
#include "app/config.hpp"

#include <iostream>
#include <csignal>
#include <atomic>

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
static std::atomic<flutter_project::Server*> g_server = nullptr;

static void signal_handler(int /*signal*/)
{
	auto* srv = g_server.load();
	if (srv)
	{
		std::cout << "\nShutting down gracefully...\n";
		srv->stop();
		g_server.store(nullptr);
	}
}

int main()
{
	try
	{
		// Load configuration from environment
		auto config = flutter_project::Config::from_env();

		// Create and run server
		flutter_project::Server server(config);
		g_server.store(&server);

		// Handle graceful shutdown
		(void)std::signal(SIGINT, signal_handler);
		(void)std::signal(SIGTERM, signal_handler);

#ifdef COROUTE_CLIENT_MODE
		// In Flutter/FFI mode the app must NOT bind a TCP port — the macOS
		// sandbox disallows it and the bridge dispatches requests in-process.
		server.run_client();
#else
		server.run();
#endif

		return 0;
	}
	catch (const std::exception& e)
	{
		std::cerr << "Fatal error: " << e.what() << "\n";
		return 1;
	}
}
