// Task Dashboard - Production Example
// A real-time collaborative task management application

#include "app/config.hpp"
#include "app/server.hpp"

#include <iostream>
#include <csignal>

namespace {
    project::Server* g_server = nullptr;
    
    void signal_handler(int signal) {
        if (signal == SIGINT || signal == SIGTERM) {
            std::cout << "\nShutting down...\n";
            if (g_server) {
                g_server->stop();
            }
        }
    }
}

int main() {
    try {
        // Load configuration from environment
        auto config = project::Config::from_env();
        
        // Create and run server
        project::Server server(config);
        g_server = &server;
        
        // Handle graceful shutdown
        std::signal(SIGINT, signal_handler);
        std::signal(SIGTERM, signal_handler);
        
        server.run();
        
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << "\n";
        return 1;
    }
}
