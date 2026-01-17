#pragma once

#include <atomic>
#include <memory>
#include <functional>
#include <vector>
#include <mutex>

namespace coroute {

// Forward declarations
class CancellationToken;
class CancellationSource;

// ============================================================================
// CancellationState - Shared state between source and tokens
// ============================================================================

namespace detail {

class CancellationState {
    std::atomic<bool> cancelled_{false};
    std::mutex mutex_;
    std::vector<std::function<void()>> callbacks_;

public:
    bool is_cancelled() const noexcept {
        return cancelled_.load(std::memory_order_acquire);
    }

    bool cancel() noexcept {
        bool expected = false;
        if (cancelled_.compare_exchange_strong(expected, true, std::memory_order_acq_rel)) {
            // Successfully cancelled, invoke callbacks
            std::vector<std::function<void()>> cbs;
            {
                std::lock_guard lock(mutex_);
                cbs = std::move(callbacks_);
            }
            for (auto& cb : cbs) {
                if (cb) cb();
            }
            return true;
        }
        return false; // Already cancelled
    }

    // Register a callback to be invoked on cancellation
    // Returns true if registered, false if already cancelled (callback invoked immediately)
    bool register_callback(std::function<void()> cb) {
        if (is_cancelled()) {
            if (cb) cb();
            return false;
        }
        
        std::lock_guard lock(mutex_);
        if (cancelled_.load(std::memory_order_acquire)) {
            if (cb) cb();
            return false;
        }
        callbacks_.push_back(std::move(cb));
        return true;
    }
};

} // namespace detail

// ============================================================================
// CancellationToken - Read-only view of cancellation state
// ============================================================================

class CancellationToken {
    std::shared_ptr<detail::CancellationState> state_;

    friend class CancellationSource;
    
    explicit CancellationToken(std::shared_ptr<detail::CancellationState> state)
        : state_(std::move(state)) {}

public:
    CancellationToken() = default;
    CancellationToken(const CancellationToken&) = default;
    CancellationToken(CancellationToken&&) = default;
    CancellationToken& operator=(const CancellationToken&) = default;
    CancellationToken& operator=(CancellationToken&&) = default;

    // Check if cancellation has been requested
    bool is_cancelled() const noexcept {
        return state_ && state_->is_cancelled();
    }

    // Implicit conversion to bool (true if NOT cancelled, for easy checks)
    explicit operator bool() const noexcept {
        return !is_cancelled();
    }

    // Check if this token is valid (has associated state)
    bool valid() const noexcept {
        return state_ != nullptr;
    }

    // Register a callback to be invoked when cancelled
    // Callback is invoked immediately if already cancelled
    void on_cancel(std::function<void()> callback) const {
        if (state_) {
            state_->register_callback(std::move(callback));
        }
    }

    // Create a token that is never cancelled (for operations that don't support cancellation)
    static CancellationToken none() {
        return CancellationToken{};
    }
};

// ============================================================================
// CancellationSource - Controls cancellation
// ============================================================================

class CancellationSource {
    std::shared_ptr<detail::CancellationState> state_;

public:
    CancellationSource()
        : state_(std::make_shared<detail::CancellationState>()) {}

    CancellationSource(const CancellationSource&) = default;
    CancellationSource(CancellationSource&&) = default;
    CancellationSource& operator=(const CancellationSource&) = default;
    CancellationSource& operator=(CancellationSource&&) = default;

    // Get a token that can be passed to operations
    CancellationToken token() const {
        return CancellationToken{state_};
    }

    // Request cancellation
    bool cancel() noexcept {
        return state_ && state_->cancel();
    }

    // Check if cancellation has been requested
    bool is_cancelled() const noexcept {
        return state_ && state_->is_cancelled();
    }
};

// ============================================================================
// RAII guard for automatic cancellation
// ============================================================================

class CancellationGuard {
    CancellationSource* source_;

public:
    explicit CancellationGuard(CancellationSource& source) : source_(&source) {}
    
    CancellationGuard(const CancellationGuard&) = delete;
    CancellationGuard& operator=(const CancellationGuard&) = delete;
    
    CancellationGuard(CancellationGuard&& other) noexcept : source_(other.source_) {
        other.source_ = nullptr;
    }
    
    CancellationGuard& operator=(CancellationGuard&& other) noexcept {
        if (this != &other) {
            if (source_) source_->cancel();
            source_ = other.source_;
            other.source_ = nullptr;
        }
        return *this;
    }
    
    ~CancellationGuard() {
        if (source_) source_->cancel();
    }
    
    void release() noexcept {
        source_ = nullptr;
    }
};

} // namespace coroute
