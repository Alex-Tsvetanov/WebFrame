#include "request_logger.hpp"
#include <iostream>
#include <chrono>
#include <iomanip>

namespace project::middleware {

coroute::Middleware request_logger() {
    return [](coroute::Request& req, coroute::Next next) -> coroute::Task<coroute::Response> {
        auto start = std::chrono::steady_clock::now();
        
        // Call next middleware/handler
        auto response = co_await next(req);
        
        auto end = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        // Get current time for logging
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        
        // Log the request
        std::cout << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S")
                  << " | " << std::setw(3) << response.status()
                  << " | " << std::setw(6) << duration.count() << "ms"
                  << " | " << coroute::method_to_string(req.method())
                  << " " << req.path()
                  << "\n";
        
        co_return response;
    };
}

} // namespace project::middleware
