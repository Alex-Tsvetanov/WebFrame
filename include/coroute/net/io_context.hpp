#pragma once

#include <cstddef>
#include <memory>
#include <functional>
#include <chrono>

#include "coroute/util/expected.hpp"
#include "coroute/core/error.hpp"
#include "coroute/coro/task.hpp"
#include "coroute/coro/cancellation.hpp"

namespace coroute::net {

// Forward declarations
class Socket;
class Connection;

// ============================================================================
// IoContext - Abstract I/O event loop
// ============================================================================

// Connection handler callback for multi-accept
using ConnectionHandler = std::function<void(std::unique_ptr<Connection>)>;

class IoContext {
public:
    virtual ~IoContext() = default;

    // Run the event loop (blocking)
    virtual void run() = 0;
    
    // Run one iteration of the event loop
    virtual void run_one() = 0;
    
    // Stop the event loop
    virtual void stop() = 0;
    
    // Check if stopped
    virtual bool stopped() const noexcept = 0;

    // Post a callback to be executed in the event loop
    virtual void post(std::function<void()> callback) = 0;

    // Schedule a callback after a delay
    virtual void schedule(std::chrono::milliseconds delay, std::function<void()> callback) = 0;

    // Enable multi-accept mode (SO_REUSEPORT on Linux)
    // Each worker thread accepts on its own listener for better scalability
    // Returns false if not supported or failed
    virtual bool enable_multi_accept(uint16_t port, ConnectionHandler handler, int backlog = 1024) {
        (void)port; (void)handler; (void)backlog;
        return false;  // Default: not supported
    }
    
    // Check if multi-accept is enabled
    virtual bool is_multi_accept_enabled() const noexcept { return false; }

    // Factory method - creates platform-appropriate context
    static std::unique_ptr<IoContext> create(size_t thread_count = 1);
};

// ============================================================================
// Async Operations - Awaitables for coroutines
// ============================================================================

// Result types for async operations
using AcceptResult = expected<std::unique_ptr<Connection>, Error>;
using ReadResult = expected<size_t, Error>;
using WriteResult = expected<size_t, Error>;
using ConnectResult = expected<void, Error>;
using TransmitResult = expected<size_t, Error>;

// Platform-specific file handle type
#ifdef _WIN32
using FileHandle = void*;  // HANDLE
#else
using FileHandle = int;    // fd
#endif

// ============================================================================
// Listener - Accepts incoming connections
// ============================================================================

class Listener {
public:
    virtual ~Listener() = default;

    // Start listening on the specified port
    virtual expected<void, Error> listen(uint16_t port, int backlog = 128) = 0;
    
    // Accept a connection (coroutine)
    virtual Task<AcceptResult> async_accept() = 0;
    
    // Close the listener
    virtual void close() = 0;
    
    // Check if listening
    virtual bool is_listening() const noexcept = 0;

    // Get local port
    virtual uint16_t local_port() const noexcept = 0;

    // Factory method
    static std::unique_ptr<Listener> create(IoContext& ctx);
};

// ============================================================================
// Connection - Async read/write on a socket
// ============================================================================

class Connection {
public:
    virtual ~Connection() = default;

    // Async read into buffer
    virtual Task<ReadResult> async_read(void* buffer, size_t len) = 0;
    
    // Async read until delimiter or buffer full
    virtual Task<ReadResult> async_read_until(void* buffer, size_t len, char delimiter) = 0;
    
    // Async write from buffer
    virtual Task<WriteResult> async_write(const void* buffer, size_t len) = 0;
    
    // Async write all data (loops until complete)
    virtual Task<WriteResult> async_write_all(const void* buffer, size_t len) = 0;
    
    // Zero-copy file transfer (TransmitFile on Windows, sendfile on Linux)
    // Returns bytes transmitted
    virtual Task<TransmitResult> async_transmit_file(FileHandle file, 
                                                      size_t offset, 
                                                      size_t length) = 0;

    // Close the connection
    virtual void close() = 0;
    
    // Check if connected
    virtual bool is_open() const noexcept = 0;

    // Set read/write timeout
    virtual void set_timeout(std::chrono::milliseconds timeout) = 0;

    // Get remote address (as string for now)
    virtual std::string remote_address() const = 0;
    
    // Get remote port
    virtual uint16_t remote_port() const noexcept = 0;

    // Set cancellation token for this connection
    virtual void set_cancellation_token(CancellationToken token) = 0;
};

} // namespace coroute::net
