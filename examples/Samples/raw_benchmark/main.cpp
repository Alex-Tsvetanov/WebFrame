/**
 * Raw io_uring benchmark - bypasses Coroute framework
 * Tests pure io_uring + coroutine performance
 */

#include <coroute/net/io_context.hpp>
#include <coroute/coro/task.hpp>
#include <iostream>
#include <cstring>
#include <csignal>
#include <thread>

using namespace coroute;
using namespace coroute::net;

// Pre-computed HTTP response (no allocations)
static constexpr const char HTTP_RESPONSE[] = 
    "HTTP/1.1 200 OK\r\n"
    "Content-Type: text/plain\r\n"
    "Content-Length: 13\r\n"
    "Connection: keep-alive\r\n"
    "\r\n"
    "Hello, World!";

static constexpr size_t HTTP_RESPONSE_LEN = sizeof(HTTP_RESPONSE) - 1;

// Minimal request buffer
static constexpr size_t BUFFER_SIZE = 1024;

std::atomic<bool> g_running{true};

void signal_handler(int) {
    g_running = false;
}

// Minimal connection handler - no HTTP parsing, just echo response
Task<void> handle_connection_raw(std::unique_ptr<Connection> conn) {
    char buffer[BUFFER_SIZE];
    
    while (g_running) {
        // Read request (we don't parse it, just consume)
        auto read_result = co_await conn->async_read(buffer, BUFFER_SIZE);
        if (!read_result || *read_result == 0) {
            break;
        }
        
        // Send pre-computed response
        auto write_result = co_await conn->async_write(HTTP_RESPONSE, HTTP_RESPONSE_LEN);
        if (!write_result) {
            break;
        }
    }
    
    conn->close();
}

int main() {
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);
    
    // Use 8 threads to stay within io_uring limits
    size_t threads = 8;
    auto io_ctx = IoContext::create(threads);
    std::cout << "Using " << threads << " worker threads" << std::endl;
    
    // Use multi-accept for best performance
    bool ok = io_ctx->enable_multi_accept(8080, [](std::unique_ptr<Connection> conn) {
        handle_connection_raw(std::move(conn)).start_detached();
    });
    
    if (!ok) {
        std::cerr << "Failed to enable multi-accept" << std::endl;
        return 1;
    }
    
    std::cout << "Raw benchmark server on port 8080 (multi-accept)" << std::endl;
    std::cout << "Press Ctrl+C to stop" << std::endl;
    
    io_ctx->run();
    
    return 0;
}
