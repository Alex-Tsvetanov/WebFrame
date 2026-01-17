#pragma once

#include <vector>
#include <queue>
#include <mutex>
#include <memory>
#include <atomic>
#include <functional>

#include "coroute/net/io_context.hpp"

namespace coroute::net {

// ============================================================================
// Connection Pool Configuration
// ============================================================================

struct ConnectionPoolConfig {
    size_t initial_size = 16;       // Pre-allocate this many connections
    size_t max_size = 256;          // Maximum pooled connections
    size_t pre_accept_count = 4;    // Number of pending accepts to maintain
    bool enable_reuse = true;       // Enable connection reuse
};

// ============================================================================
// Connection Pool
// ============================================================================

class ConnectionPool {
public:
    using ConnectionFactory = std::function<std::unique_ptr<Connection>()>;
    using ConnectionResetter = std::function<void(Connection&)>;
    
private:
    std::vector<std::unique_ptr<Connection>> pool_;
    mutable std::mutex mutex_;
    ConnectionPoolConfig config_;
    ConnectionResetter resetter_;
    
    std::atomic<size_t> active_count_{0};
    std::atomic<size_t> total_acquired_{0};
    std::atomic<size_t> total_reused_{0};
    
public:
    explicit ConnectionPool(ConnectionPoolConfig config = {})
        : config_(std::move(config))
    {
        pool_.reserve(config_.max_size);
    }
    
    // Set custom reset function (called when connection is returned)
    void set_resetter(ConnectionResetter resetter) {
        resetter_ = std::move(resetter);
    }
    
    // Acquire a connection from the pool (or nullptr if empty)
    std::unique_ptr<Connection> acquire() {
        std::lock_guard<std::mutex> lock(mutex_);
        total_acquired_.fetch_add(1, std::memory_order_relaxed);
        
        if (pool_.empty()) {
            return nullptr;
        }
        
        total_reused_.fetch_add(1, std::memory_order_relaxed);
        auto conn = std::move(pool_.back());
        pool_.pop_back();
        active_count_.fetch_add(1, std::memory_order_relaxed);
        return conn;
    }
    
    // Release a connection back to the pool
    void release(std::unique_ptr<Connection> conn) {
        if (!conn || !config_.enable_reuse) {
            return;
        }
        
        active_count_.fetch_sub(1, std::memory_order_relaxed);
        
        // Reset connection state if resetter is set
        if (resetter_) {
            resetter_(*conn);
        }
        
        std::lock_guard<std::mutex> lock(mutex_);
        if (pool_.size() < config_.max_size) {
            pool_.push_back(std::move(conn));
        }
        // If pool is full, connection is destroyed
    }
    
    // Pre-populate the pool with connections
    void populate(ConnectionFactory factory, size_t count) {
        std::lock_guard<std::mutex> lock(mutex_);
        for (size_t i = 0; i < count && pool_.size() < config_.max_size; ++i) {
            if (auto conn = factory()) {
                pool_.push_back(std::move(conn));
            }
        }
    }
    
    // Statistics
    size_t pool_size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return pool_.size();
    }
    
    size_t active_count() const { 
        return active_count_.load(std::memory_order_relaxed); 
    }
    
    size_t total_acquired() const { 
        return total_acquired_.load(std::memory_order_relaxed); 
    }
    
    size_t total_reused() const { 
        return total_reused_.load(std::memory_order_relaxed); 
    }
    
    double reuse_rate() const {
        size_t acquired = total_acquired_.load(std::memory_order_relaxed);
        if (acquired == 0) return 0.0;
        return static_cast<double>(total_reused_.load(std::memory_order_relaxed)) / acquired;
    }
    
    // Clear the pool
    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        pool_.clear();
    }
    
    const ConnectionPoolConfig& config() const { return config_; }
};

// ============================================================================
// Pooled Connection Handle (RAII)
// ============================================================================

class PooledConnection {
    std::unique_ptr<Connection> conn_;
    ConnectionPool* pool_;
    
public:
    PooledConnection() : conn_(nullptr), pool_(nullptr) {}
    
    PooledConnection(std::unique_ptr<Connection> conn, ConnectionPool* pool)
        : conn_(std::move(conn)), pool_(pool) {}
    
    ~PooledConnection() {
        if (conn_ && pool_) {
            pool_->release(std::move(conn_));
        }
    }
    
    // Move only
    PooledConnection(const PooledConnection&) = delete;
    PooledConnection& operator=(const PooledConnection&) = delete;
    
    PooledConnection(PooledConnection&& other) noexcept
        : conn_(std::move(other.conn_)), pool_(other.pool_) {
        other.pool_ = nullptr;
    }
    
    PooledConnection& operator=(PooledConnection&& other) noexcept {
        if (this != &other) {
            if (conn_ && pool_) {
                pool_->release(std::move(conn_));
            }
            conn_ = std::move(other.conn_);
            pool_ = other.pool_;
            other.pool_ = nullptr;
        }
        return *this;
    }
    
    // Access
    Connection* get() { return conn_.get(); }
    const Connection* get() const { return conn_.get(); }
    Connection& operator*() { return *conn_; }
    const Connection& operator*() const { return *conn_; }
    Connection* operator->() { return conn_.get(); }
    const Connection* operator->() const { return conn_.get(); }
    
    explicit operator bool() const { return conn_ != nullptr; }
    
    // Release ownership (won't return to pool)
    std::unique_ptr<Connection> release() {
        pool_ = nullptr;
        return std::move(conn_);
    }
    
    // Detach from pool but keep connection
    void detach() {
        pool_ = nullptr;
    }
};

// ============================================================================
// Accept Socket Pool (for pre-allocated accept sockets)
// ============================================================================

#ifdef _WIN32

// Pre-allocated socket for AcceptEx
struct PreAllocatedSocket {
    void* socket;  // SOCKET
    char accept_buffer[2 * (sizeof(void*) * 4 + 16)];  // Address buffer
    bool pending = false;
};

class AcceptSocketPool {
    std::vector<PreAllocatedSocket> sockets_;
    std::queue<size_t> available_;
    mutable std::mutex mutex_;
    size_t max_pending_;
    
public:
    explicit AcceptSocketPool(size_t max_pending = 8)
        : max_pending_(max_pending)
    {
        sockets_.reserve(max_pending);
    }
    
    // Get index of available socket, or -1 if none
    int acquire() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (available_.empty()) {
            return -1;
        }
        int idx = static_cast<int>(available_.front());
        available_.pop();
        sockets_[idx].pending = true;
        return idx;
    }
    
    // Return socket to pool
    void release(size_t idx) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (idx < sockets_.size()) {
            sockets_[idx].pending = false;
            available_.push(idx);
        }
    }
    
    // Add a new pre-allocated socket
    size_t add(void* socket) {
        std::lock_guard<std::mutex> lock(mutex_);
        size_t idx = sockets_.size();
        sockets_.push_back({socket, {}, false});
        available_.push(idx);
        return idx;
    }
    
    PreAllocatedSocket* get(size_t idx) {
        if (idx >= sockets_.size()) return nullptr;
        return &sockets_[idx];
    }
    
    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return sockets_.size();
    }
    
    size_t available_count() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return available_.size();
    }
};

#endif // _WIN32

} // namespace coroute::net
