#include "coroute/net/io_context.hpp"

#if defined(COROUTE_PLATFORM_LINUX)

#include <liburing.h>
#include <sys/socket.h>
#include <sys/eventfd.h>
#include <sys/sendfile.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>

#include <thread>
#include <vector>
#include <queue>
#include <mutex>
#include <atomic>
#include <memory>
#include <cstring>

namespace coroute::net {

// ============================================================================
// io_uring Operation Types
// ============================================================================

enum class UringOpType {
    Accept,
    Read,
    Write,
    Connect,
    Timeout,
    Cancel
};

struct UringOperation {
    UringOpType type;
    std::coroutine_handle<> continuation;
    Error error;
    int result = 0;
    size_t ring_index = 0;  // Which ring this operation belongs to
    
    // For accept
    int accept_fd = -1;
    sockaddr_in client_addr{};
    socklen_t client_addr_len = sizeof(sockaddr_in);
    
    UringOperation(UringOpType t) : type(t) {}
};

// Custom awaiter that captures the continuation handle before suspending
struct UringAwaiter {
    UringOperation& op;
    
    bool await_ready() const noexcept { return false; }
    
    void await_suspend(std::coroutine_handle<> h) noexcept {
        op.continuation = h;
    }
    
    void await_resume() const noexcept {}
};

// ============================================================================
// Per-Thread Ring - Each worker has its own io_uring instance and listener
// ============================================================================

struct WorkerRing {
    io_uring ring;
    int eventfd = -1;
    int listen_fd = -1;  // SO_REUSEPORT listener for this ring
    std::atomic<bool> initialized{false};
    
    WorkerRing() = default;
    
    ~WorkerRing() {
        if (listen_fd >= 0) {
            ::close(listen_fd);
        }
        if (initialized) {
            if (eventfd >= 0) {
                ::close(eventfd);
            }
            io_uring_queue_exit(&ring);
        }
    }
    
    // Non-copyable, non-movable
    WorkerRing(const WorkerRing&) = delete;
    WorkerRing& operator=(const WorkerRing&) = delete;
    
    bool init() {
        // Use larger queue size for better throughput
        io_uring_params params{};
        // SQPOLL can hurt performance for this workload, use regular mode
        int ret = io_uring_queue_init_params(8192, &ring, &params);
        if (ret < 0) {
            return false;
        }
        
        eventfd = ::eventfd(0, EFD_NONBLOCK);
        if (eventfd < 0) {
            io_uring_queue_exit(&ring);
            return false;
        }
        
        initialized = true;
        return true;
    }
    
    // Create SO_REUSEPORT listener for this ring
    bool create_listener(uint16_t port, int backlog) {
        listen_fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
        if (listen_fd < 0) return false;
        
        int opt = 1;
        setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        setsockopt(listen_fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));
        
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(port);
        
        if (bind(listen_fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
            ::close(listen_fd);
            listen_fd = -1;
            return false;
        }
        
        if (::listen(listen_fd, backlog) < 0) {
            ::close(listen_fd);
            listen_fd = -1;
            return false;
        }
        
        return true;
    }
    
    void wake() {
        if (eventfd >= 0) {
            uint64_t val = 1;
            ::write(eventfd, &val, sizeof(val));
        }
    }
};

// ============================================================================
// io_uring Context Implementation - Per-Thread Rings
// ============================================================================

// Forward declaration
class UringConnection;

// Connection handler callback
using ConnectionHandler = std::function<void(std::unique_ptr<Connection>)>;

class UringContext : public IoContext {
    std::vector<std::unique_ptr<WorkerRing>> rings_;
    std::vector<std::thread> workers_;
    std::atomic<bool> stopped_{false};
    std::atomic<size_t> next_ring_{0};
    size_t thread_count_;
    
    // SO_REUSEPORT multi-accept
    ConnectionHandler connection_handler_;
    uint16_t listen_port_ = 0;
    bool multi_accept_enabled_ = false;
    
    // Posted callbacks
    std::mutex callback_mutex_;
    std::queue<std::function<void()>> callbacks_;

public:
    explicit UringContext(size_t thread_count)
        : thread_count_(thread_count > 0 ? thread_count : 1)
    {
        // Create per-thread rings
        rings_.reserve(thread_count_);
        for (size_t i = 0; i < thread_count_; ++i) {
            auto ring = std::make_unique<WorkerRing>();
            if (!ring->init()) {
                throw std::runtime_error("Failed to initialize io_uring ring " + std::to_string(i));
            }
            rings_.push_back(std::move(ring));
        }
    }
    
    ~UringContext() override {
        stop();
        
        for (auto& worker : workers_) {
            if (worker.joinable()) {
                worker.join();
            }
        }
    }

    size_t ring_count() const noexcept { return rings_.size(); }
    
    WorkerRing* worker_ring(size_t index) noexcept {
        return rings_[index % rings_.size()].get();
    }

    // Get the ring for a specific index
    io_uring* ring(size_t index) noexcept { 
        return &rings_[index % rings_.size()]->ring; 
    }
    
    // Get the next ring index (round-robin)
    size_t next_ring_index() noexcept {
        return next_ring_.fetch_add(1, std::memory_order_relaxed) % rings_.size();
    }
    
    // Enable SO_REUSEPORT multi-accept: each worker accepts on its own listener
    bool enable_multi_accept(uint16_t port, ConnectionHandler handler, int backlog = 1024) override {
        // Create SO_REUSEPORT listeners on all rings
        for (auto& ring : rings_) {
            if (!ring->create_listener(port, backlog)) {
                // Clean up on failure
                for (auto& r : rings_) {
                    if (r->listen_fd >= 0) {
                        ::close(r->listen_fd);
                        r->listen_fd = -1;
                    }
                }
                return false;
            }
        }
        
        connection_handler_ = std::move(handler);
        listen_port_ = port;
        multi_accept_enabled_ = true;
        return true;
    }
    
    bool is_multi_accept_enabled() const noexcept override { return multi_accept_enabled_; }
    uint16_t listen_port() const noexcept { return listen_port_; }
    
    // Submit SQE to a specific ring
    template<typename PrepFunc>
    bool submit_sqe(size_t ring_index, UringOperation* op, PrepFunc prep_func) {
        auto* worker_ring = rings_[ring_index % rings_.size()].get();
        io_uring_sqe* sqe = io_uring_get_sqe(&worker_ring->ring);
        if (!sqe) {
            return false;
        }
        prep_func(sqe);
        io_uring_sqe_set_data(sqe, op);
        op->ring_index = ring_index;
        io_uring_submit(&worker_ring->ring);
        return true;
    }
    
    // Submit to ring 0 (default)
    template<typename PrepFunc>
    bool submit_sqe(UringOperation* op, PrepFunc prep_func) {
        return submit_sqe(0, op, prep_func);
    }

    void run() override {
        stopped_ = false;
        
        // Start one worker thread per ring
        for (size_t i = 0; i < thread_count_; ++i) {
            workers_.emplace_back([this, i] { worker_loop(i); });
        }
        
        // Wait for all workers
        for (auto& worker : workers_) {
            if (worker.joinable()) {
                worker.join();
            }
        }
    }
    
    void run_one() override {
        poll_and_resume(0);
    }
    
    void stop() override {
        stopped_ = true;
        
        // Wake up all worker threads
        for (auto& ring : rings_) {
            ring->wake();
        }
    }
    
    bool stopped() const noexcept override {
        return stopped_;
    }

    void post(std::function<void()> callback) override {
        {
            std::lock_guard lock(callback_mutex_);
            callbacks_.push(std::move(callback));
        }
        
        // Wake up ring 0 to process callbacks
        if (!rings_.empty()) {
            rings_[0]->wake();
        }
    }

    void schedule(std::chrono::milliseconds delay, std::function<void()> callback) override {
        std::thread([this, delay, cb = std::move(callback)]() mutable {
            std::this_thread::sleep_for(delay);
            post(std::move(cb));
        }).detach();
    }

private:
    // Accept loop coroutine for multi-accept mode
    Task<void> accept_loop(size_t ring_index);
    
    void worker_loop(size_t ring_index) {
        // Start accept loop if multi-accept is enabled
        if (multi_accept_enabled_ && rings_[ring_index]->listen_fd >= 0) {
            accept_loop(ring_index).start_detached();
        }
        
        while (!stopped_) {
            poll_and_resume(ring_index);
            
            // Only ring 0 processes callbacks
            if (ring_index == 0) {
                process_callbacks();
            }
        }
    }

    void poll_and_resume(size_t ring_index) {
        auto* worker_ring = rings_[ring_index].get();
        io_uring_cqe* cqe;
        
        // Use short timeout - balance between latency and CPU usage
        __kernel_timespec ts;
        ts.tv_sec = 0;
        ts.tv_nsec = 1000; // 1µs timeout for low latency
        
        int ret = io_uring_wait_cqe_timeout(&worker_ring->ring, &cqe, &ts);
        if (ret == -ETIME || ret < 0) {
            return;
        }
        
        // Process all available completions in batch
        unsigned head;
        unsigned processed = 0;
        io_uring_for_each_cqe(&worker_ring->ring, head, cqe) {
            auto* op = static_cast<UringOperation*>(io_uring_cqe_get_data(cqe));
            if (op) {
                op->result = cqe->res;
                if (cqe->res < 0) {
                    op->error = Error::system(std::error_code(-cqe->res, std::system_category()));
                }
                
                // Resume coroutine inline
                if (op->continuation) {
                    op->continuation.resume();
                }
            }
            processed++;
            if (processed >= 512) break;  // Limit batch size
        }
        io_uring_cq_advance(&worker_ring->ring, processed);
    }

    void process_callbacks() {
        for (int i = 0; i < 32; ++i) {
            std::function<void()> callback;
            {
                std::lock_guard lock(callback_mutex_);
                if (callbacks_.empty()) return;
                callback = std::move(callbacks_.front());
                callbacks_.pop();
            }
            if (callback) {
                callback();
            }
        }
    }
};

// ============================================================================
// io_uring Listener Implementation
// ============================================================================

class UringListener : public Listener {
    UringContext& ctx_;
    int listen_fd_ = -1;
    uint16_t port_ = 0;

public:
    explicit UringListener(UringContext& ctx) : ctx_(ctx) {}
    
    ~UringListener() override {
        close();
    }

    expected<void, Error> listen(uint16_t port, int backlog) override {
        listen_fd_ = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
        if (listen_fd_ < 0) {
            return unexpected(Error::system(std::error_code(errno, std::system_category())));
        }
        
        // Set SO_REUSEADDR
        int opt = 1;
        setsockopt(listen_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        
        // Bind
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(port);
        
        if (bind(listen_fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
            ::close(listen_fd_);
            listen_fd_ = -1;
            return unexpected(Error::system(std::error_code(errno, std::system_category())));
        }
        
        // Listen
        if (::listen(listen_fd_, backlog) < 0) {
            ::close(listen_fd_);
            listen_fd_ = -1;
            return unexpected(Error::system(std::error_code(errno, std::system_category())));
        }
        
        // Get actual port
        sockaddr_in bound_addr{};
        socklen_t addr_len = sizeof(bound_addr);
        getsockname(listen_fd_, reinterpret_cast<sockaddr*>(&bound_addr), &addr_len);
        port_ = ntohs(bound_addr.sin_port);
        
        return {};
    }
    
    Task<AcceptResult> async_accept() override;
    
    UringContext& context() noexcept { return ctx_; }
    int fd() const noexcept { return listen_fd_; }
    
    void close() override {
        if (listen_fd_ >= 0) {
            ::close(listen_fd_);
            listen_fd_ = -1;
        }
    }
    
    bool is_listening() const noexcept override {
        return listen_fd_ >= 0;
    }

    uint16_t local_port() const noexcept override {
        return port_;
    }
};

// ============================================================================
// io_uring Connection Implementation
// ============================================================================

class UringConnection : public Connection {
    UringContext& ctx_;
    int fd_;
    size_t ring_index_;  // Which ring this connection is assigned to
    std::chrono::milliseconds timeout_{30000};
    CancellationToken cancel_token_;
    std::string remote_addr_;
    uint16_t remote_port_ = 0;

public:
    UringConnection(UringContext& ctx, int fd, const sockaddr_in& addr, size_t ring_index = 0)
        : ctx_(ctx)
        , fd_(fd)
        , ring_index_(ring_index)
    {
        char ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &addr.sin_addr, ip, sizeof(ip));
        remote_addr_ = ip;
        remote_port_ = ntohs(addr.sin_port);
    }
    
    size_t ring_index() const noexcept { return ring_index_; }
    
    ~UringConnection() override {
        close();
    }

    Task<ReadResult> async_read(void* buffer, size_t len) override;
    Task<ReadResult> async_read_until(void* buffer, size_t len, char delimiter) override;
    Task<WriteResult> async_write(const void* buffer, size_t len) override;
    Task<WriteResult> async_write_all(const void* buffer, size_t len) override;
    Task<TransmitResult> async_transmit_file(FileHandle file, size_t offset, size_t length) override;

    void close() override {
        if (fd_ >= 0) {
            ::close(fd_);
            fd_ = -1;
        }
    }
    
    bool is_open() const noexcept override {
        return fd_ >= 0;
    }

    void set_timeout(std::chrono::milliseconds timeout) override {
        timeout_ = timeout;
    }

    std::string remote_address() const override {
        return remote_addr_;
    }
    
    uint16_t remote_port() const noexcept override {
        return remote_port_;
    }

    void set_cancellation_token(CancellationToken token) override {
        cancel_token_ = std::move(token);
    }

    int fd() const noexcept { return fd_; }
};

// ============================================================================
// Async Operation Implementations
// ============================================================================

Task<AcceptResult> UringListener::async_accept() {
    if (!is_listening()) {
        co_return unexpected(Error::io(IoError::InvalidArgument, "Not listening"));
    }
    
    UringOperation op{UringOpType::Accept};
    int fd = listen_fd_;
    
    // Submit accept to ring 0
    bool submitted = ctx_.submit_sqe(0, &op, [fd, &op](io_uring_sqe* sqe) {
        io_uring_prep_accept(sqe, fd, 
                             reinterpret_cast<sockaddr*>(&op.client_addr), 
                             &op.client_addr_len, 0);
    });
    
    if (!submitted) {
        co_return unexpected(Error::io(IoError::Unknown, "Failed to get SQE"));
    }
    
    // Suspend and wait for completion
    co_await UringAwaiter{op};
    
    if (op.error) {
        co_return unexpected(op.error);
    }
    
    if (op.result < 0) {
        co_return unexpected(Error::system(std::error_code(-op.result, std::system_category())));
    }
    
    // Connection on ring 0
    co_return AcceptResult(std::make_unique<UringConnection>(ctx_, op.result, op.client_addr, 0));
}

Task<ReadResult> UringConnection::async_read(void* buffer, size_t len) {
    if (!is_open()) {
        co_return unexpected(Error::io(IoError::ConnectionReset, "Connection closed"));
    }
    
    if (cancel_token_.is_cancelled()) {
        co_return unexpected(Error::cancelled());
    }
    
    UringOperation op{UringOpType::Read};
    int fd = fd_;
    
    bool submitted = ctx_.submit_sqe(ring_index_, &op, [fd, buffer, len](io_uring_sqe* sqe) {
        io_uring_prep_recv(sqe, fd, buffer, len, 0);
    });
    
    if (!submitted) {
        co_return unexpected(Error::io(IoError::Unknown, "Failed to get SQE"));
    }
    
    co_await UringAwaiter{op};
    
    if (op.error) {
        co_return unexpected(op.error);
    }
    
    if (op.result == 0) {
        co_return unexpected(Error::io(IoError::EndOfStream, "Connection closed by peer"));
    }
    
    if (op.result < 0) {
        co_return unexpected(Error::system(std::error_code(-op.result, std::system_category())));
    }
    
    co_return static_cast<size_t>(op.result);
}

Task<ReadResult> UringConnection::async_read_until(void* buffer, size_t len, char delimiter) {
    char* buf = static_cast<char*>(buffer);
    size_t total = 0;
    
    while (total < len) {
        auto result = co_await async_read(buf + total, 1);
        if (!result) {
            co_return unexpected(result.error());
        }
        
        total += *result;
        if (buf[total - 1] == delimiter) {
            break;
        }
    }
    
    co_return total;
}

Task<WriteResult> UringConnection::async_write(const void* buffer, size_t len) {
    if (!is_open()) {
        co_return unexpected(Error::io(IoError::ConnectionReset, "Connection closed"));
    }
    
    if (cancel_token_.is_cancelled()) {
        co_return unexpected(Error::cancelled());
    }
    
    UringOperation op{UringOpType::Write};
    int fd = fd_;
    
    bool submitted = ctx_.submit_sqe(ring_index_, &op, [fd, buffer, len](io_uring_sqe* sqe) {
        io_uring_prep_send(sqe, fd, buffer, len, 0);
    });
    
    if (!submitted) {
        co_return unexpected(Error::io(IoError::Unknown, "Failed to get SQE"));
    }
    
    co_await UringAwaiter{op};
    
    if (op.error) {
        co_return unexpected(op.error);
    }
    
    if (op.result < 0) {
        co_return unexpected(Error::system(std::error_code(-op.result, std::system_category())));
    }
    
    co_return static_cast<size_t>(op.result);
}

Task<WriteResult> UringConnection::async_write_all(const void* buffer, size_t len) {
    const char* buf = static_cast<const char*>(buffer);
    size_t total = 0;
    
    while (total < len) {
        auto result = co_await async_write(buf + total, len - total);
        if (!result) {
            co_return unexpected(result.error());
        }
        total += *result;
    }
    
    co_return total;
}

Task<TransmitResult> UringConnection::async_transmit_file(FileHandle file, size_t offset, size_t length) {
    if (!is_open()) {
        co_return unexpected(Error::io(IoError::ConnectionReset, "Connection closed"));
    }
    
    if (cancel_token_.is_cancelled()) {
        co_return unexpected(Error::cancelled());
    }
    
    // Use sendfile for zero-copy file transfer
    // Note: sendfile is blocking, but for large files we could use io_uring's splice
    // For now, use sendfile in a simple loop
    size_t total_sent = 0;
    off_t off = static_cast<off_t>(offset);
    
    while (total_sent < length) {
        ssize_t sent = sendfile(fd_, file, &off, length - total_sent);
        if (sent < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // Socket buffer full, need to wait for writability
                UringOperation wait_op{UringOpType::Write};
                int fd = fd_;
                bool submitted = ctx_.submit_sqe(ring_index_, &wait_op, [fd](io_uring_sqe* sqe) {
                    // Use a zero-length send to wait for socket writability
                    io_uring_prep_send(sqe, fd, nullptr, 0, MSG_NOSIGNAL);
                });
                if (!submitted) {
                    co_return unexpected(Error::io(IoError::Unknown, "Failed to get SQE"));
                }
                co_await UringAwaiter{wait_op};
                continue;
            }
            co_return unexpected(Error::system(std::error_code(errno, std::system_category())));
        }
        if (sent == 0) {
            break; // EOF on source file
        }
        total_sent += static_cast<size_t>(sent);
    }
    
    co_return total_sent;
}

// ============================================================================
// Accept Loop Implementation (for SO_REUSEPORT multi-accept)
// ============================================================================

// Helper to set TCP optimizations on accepted sockets
static void set_tcp_opts(int fd) {
    int opt = 1;
    setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt));
#ifdef TCP_QUICKACK
    setsockopt(fd, IPPROTO_TCP, TCP_QUICKACK, &opt, sizeof(opt));
#endif
    // Increase send buffer for better throughput
    int sndbuf = 256 * 1024;
    setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &sndbuf, sizeof(sndbuf));
}

Task<void> UringContext::accept_loop(size_t ring_index) {
    auto* worker_ring = rings_[ring_index].get();
    
    while (!stopped_ && worker_ring->listen_fd >= 0) {
        UringOperation op{UringOpType::Accept};
        int fd = worker_ring->listen_fd;
        
        bool submitted = submit_sqe(ring_index, &op, [fd, &op](io_uring_sqe* sqe) {
            io_uring_prep_accept(sqe, fd, 
                                 reinterpret_cast<sockaddr*>(&op.client_addr), 
                                 &op.client_addr_len, SOCK_NONBLOCK);
        });
        
        if (!submitted) {
            continue;
        }
        
        co_await UringAwaiter{op};
        
        if (stopped_) break;
        
        if (op.result < 0) {
            // Accept error - continue unless stopped
            continue;
        }
        
        // Set TCP optimizations
        set_tcp_opts(op.result);
        
        // Create connection on this ring
        auto conn = std::make_unique<UringConnection>(*this, op.result, op.client_addr, ring_index);
        
        // Call the connection handler
        if (connection_handler_) {
            connection_handler_(std::move(conn));
        } else {
            conn->close();
        }
    }
}

// ============================================================================
// Factory Functions
// ============================================================================

std::unique_ptr<IoContext> IoContext::create(size_t thread_count) {
    return std::make_unique<UringContext>(thread_count);
}

std::unique_ptr<Listener> Listener::create(IoContext& ctx) {
    return std::make_unique<UringListener>(static_cast<UringContext&>(ctx));
}

} // namespace coroute::net

#endif // coroute_PLATFORM_LINUX
