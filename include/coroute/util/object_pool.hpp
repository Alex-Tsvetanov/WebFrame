#pragma once

#include <vector>
#include <memory>
#include <mutex>
#include <functional>
#include <cstddef>

namespace coroute {

// ============================================================================
// Generic Object Pool
// ============================================================================

template<typename T>
class ObjectPool {
public:
    using ResetFunc = std::function<void(T&)>;
    
private:
    std::vector<std::unique_ptr<T>> pool_;
    mutable std::mutex mutex_;
    size_t max_size_;
    ResetFunc reset_func_;
    
public:
    explicit ObjectPool(size_t max_size = 1024, ResetFunc reset_func = nullptr)
        : max_size_(max_size)
        , reset_func_(std::move(reset_func))
    {
        pool_.reserve(std::min(max_size, size_t(64)));
    }
    
    // Pre-allocate objects
    void reserve(size_t count) {
        std::lock_guard<std::mutex> lock(mutex_);
        while (pool_.size() < count && pool_.size() < max_size_) {
            pool_.push_back(std::make_unique<T>());
        }
    }
    
    // Acquire an object from the pool (or create new)
    std::unique_ptr<T> acquire() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (pool_.empty()) {
            return std::make_unique<T>();
        }
        auto obj = std::move(pool_.back());
        pool_.pop_back();
        return obj;
    }
    
    // Release an object back to the pool
    void release(std::unique_ptr<T> obj) {
        if (!obj) return;
        
        // Reset object state
        if (reset_func_) {
            reset_func_(*obj);
        }
        
        std::lock_guard<std::mutex> lock(mutex_);
        if (pool_.size() < max_size_) {
            pool_.push_back(std::move(obj));
        }
        // If pool is full, object is destroyed
    }
    
    // Get current pool size
    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return pool_.size();
    }
    
    // Get max pool size
    size_t max_size() const { return max_size_; }
    
    // Clear the pool
    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        pool_.clear();
    }
};

// ============================================================================
// Pooled Object Handle (RAII wrapper)
// ============================================================================

template<typename T>
class PooledObject {
    std::unique_ptr<T> obj_;
    ObjectPool<T>* pool_;
    
public:
    PooledObject() : obj_(nullptr), pool_(nullptr) {}
    
    PooledObject(std::unique_ptr<T> obj, ObjectPool<T>* pool)
        : obj_(std::move(obj)), pool_(pool) {}
    
    ~PooledObject() {
        if (obj_ && pool_) {
            pool_->release(std::move(obj_));
        }
    }
    
    // Move only
    PooledObject(const PooledObject&) = delete;
    PooledObject& operator=(const PooledObject&) = delete;
    
    PooledObject(PooledObject&& other) noexcept
        : obj_(std::move(other.obj_)), pool_(other.pool_) {
        other.pool_ = nullptr;
    }
    
    PooledObject& operator=(PooledObject&& other) noexcept {
        if (this != &other) {
            if (obj_ && pool_) {
                pool_->release(std::move(obj_));
            }
            obj_ = std::move(other.obj_);
            pool_ = other.pool_;
            other.pool_ = nullptr;
        }
        return *this;
    }
    
    // Access
    T* get() { return obj_.get(); }
    const T* get() const { return obj_.get(); }
    T& operator*() { return *obj_; }
    const T& operator*() const { return *obj_; }
    T* operator->() { return obj_.get(); }
    const T* operator->() const { return obj_.get(); }
    
    explicit operator bool() const { return obj_ != nullptr; }
    
    // Release ownership (won't return to pool)
    std::unique_ptr<T> release() {
        pool_ = nullptr;
        return std::move(obj_);
    }
};

// ============================================================================
// Thread-Local Object Pool (lock-free for single thread)
// ============================================================================

template<typename T>
class ThreadLocalPool {
    static constexpr size_t DEFAULT_MAX_SIZE = 64;
    
    struct LocalPool {
        std::vector<std::unique_ptr<T>> pool;
        size_t max_size = DEFAULT_MAX_SIZE;
        
        LocalPool() { pool.reserve(16); }
    };
    
    static thread_local LocalPool local_;
    
public:
    static void set_max_size(size_t max_size) {
        local_.max_size = max_size;
    }
    
    static std::unique_ptr<T> acquire() {
        if (local_.pool.empty()) {
            return std::make_unique<T>();
        }
        auto obj = std::move(local_.pool.back());
        local_.pool.pop_back();
        return obj;
    }
    
    static void release(std::unique_ptr<T> obj) {
        if (!obj) return;
        if (local_.pool.size() < local_.max_size) {
            local_.pool.push_back(std::move(obj));
        }
    }
    
    static size_t size() { return local_.pool.size(); }
};

template<typename T>
thread_local typename ThreadLocalPool<T>::LocalPool ThreadLocalPool<T>::local_;

// ============================================================================
// Buffer Pool (specialized for byte buffers)
// ============================================================================

class BufferPool {
public:
    using Buffer = std::vector<char>;
    
private:
    ObjectPool<Buffer> pool_;
    size_t default_buffer_size_;
    
public:
    explicit BufferPool(size_t default_size = 4096, size_t max_buffers = 256)
        : pool_(max_buffers, [](Buffer& b) { b.clear(); })
        , default_buffer_size_(default_size)
    {}
    
    std::unique_ptr<Buffer> acquire() {
        auto buf = pool_.acquire();
        if (buf->capacity() < default_buffer_size_) {
            buf->reserve(default_buffer_size_);
        }
        return buf;
    }
    
    std::unique_ptr<Buffer> acquire(size_t min_size) {
        auto buf = pool_.acquire();
        if (buf->capacity() < min_size) {
            buf->reserve(min_size);
        }
        return buf;
    }
    
    void release(std::unique_ptr<Buffer> buf) {
        pool_.release(std::move(buf));
    }
    
    size_t size() const { return pool_.size(); }
};

} // namespace coroute
