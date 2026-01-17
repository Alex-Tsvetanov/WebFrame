#pragma once

#include <string>
#include <string_view>
#include <optional>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <chrono>
#include <functional>
#include <any>
#include <random>

#include "coroute/core/request.hpp"
#include "coroute/core/response.hpp"
#include "coroute/core/cookie.hpp"
#include "coroute/coro/task.hpp"

namespace coroute {

// Forward declare middleware types
using Next = std::function<Task<Response>(Request&)>;
using Middleware = std::function<Task<Response>(Request&, Next)>;

// ============================================================================
// Session Data
// ============================================================================

class Session {
    std::string id_;
    std::unordered_map<std::string, std::any> data_;
    std::chrono::system_clock::time_point created_;
    std::chrono::system_clock::time_point last_accessed_;
    bool modified_ = false;
    bool is_new_ = false;
    
public:
    Session() = default;
    explicit Session(std::string id, bool is_new = false);
    
    // Session ID
    const std::string& id() const { return id_; }
    
    // Check if session is new (just created)
    bool is_new() const { return is_new_; }
    
    // Check if session was modified
    bool is_modified() const { return modified_; }
    
    // Timestamps
    std::chrono::system_clock::time_point created_at() const { return created_; }
    std::chrono::system_clock::time_point last_accessed_at() const { return last_accessed_; }
    
    // Get value
    template<typename T>
    std::optional<T> get(const std::string& key) const {
        auto it = data_.find(key);
        if (it == data_.end()) return std::nullopt;
        try {
            return std::any_cast<T>(it->second);
        } catch (const std::bad_any_cast&) {
            return std::nullopt;
        }
    }
    
    // Get value with default
    template<typename T>
    T get_or(const std::string& key, T default_value) const {
        return get<T>(key).value_or(std::move(default_value));
    }
    
    // Set value
    template<typename T>
    void set(const std::string& key, T value) {
        data_[key] = std::move(value);
        modified_ = true;
    }
    
    // Remove value
    void remove(const std::string& key);
    
    // Check if key exists
    bool has(const std::string& key) const;
    
    // Clear all data
    void clear();
    
    // Get all keys
    std::vector<std::string> keys() const;
    
    // Touch (update last accessed time)
    void touch();
    
    // Mark as not modified (after save)
    void mark_saved() { modified_ = false; }
};

// ============================================================================
// Session Store Interface
// ============================================================================

class SessionStore {
public:
    virtual ~SessionStore() = default;
    
    // Load session by ID (returns nullptr if not found)
    virtual std::shared_ptr<Session> load(const std::string& id) = 0;
    
    // Save session
    virtual void save(const Session& session) = 0;
    
    // Delete session
    virtual void destroy(const std::string& id) = 0;
    
    // Clean up expired sessions
    virtual void cleanup(std::chrono::seconds max_age) = 0;
    
    // Generate new session ID
    virtual std::string generate_id() = 0;
};

// ============================================================================
// In-Memory Session Store
// ============================================================================

class MemorySessionStore : public SessionStore {
    struct StoredSession {
        std::shared_ptr<Session> session;
        std::chrono::system_clock::time_point expires;
    };
    
    std::unordered_map<std::string, StoredSession> sessions_;
    mutable std::mutex mutex_;
    std::chrono::seconds default_max_age_{3600};  // 1 hour
    
public:
    MemorySessionStore() = default;
    explicit MemorySessionStore(std::chrono::seconds max_age) : default_max_age_(max_age) {}
    
    std::shared_ptr<Session> load(const std::string& id) override;
    void save(const Session& session) override;
    void destroy(const std::string& id) override;
    void cleanup(std::chrono::seconds max_age) override;
    std::string generate_id() override;
    
    // Get session count
    size_t size() const;
};

// ============================================================================
// Session Options
// ============================================================================

struct SessionOptions {
    // Cookie name for session ID
    std::string cookie_name = "session_id";
    
    // Cookie path
    std::string cookie_path = "/";
    
    // Cookie domain (empty = current domain)
    std::string cookie_domain;
    
    // Session max age
    std::chrono::seconds max_age{3600};  // 1 hour
    
    // Cookie flags
    bool secure = true;
    bool http_only = true;
    SameSite same_site = SameSite::Lax;
    
    // Auto-save modified sessions
    bool auto_save = true;
    
    // Rolling sessions (extend expiry on each request)
    bool rolling = false;
};

// ============================================================================
// Session Middleware
// ============================================================================

class SessionMiddleware {
    std::shared_ptr<SessionStore> store_;
    SessionOptions options_;
    
public:
    SessionMiddleware(std::shared_ptr<SessionStore> store, SessionOptions options = {});
    
    Task<Response> operator()(Request& req, Next next);
    
    // Get session from request (must be called after middleware runs)
    static std::shared_ptr<Session> get_session(Request& req);
    
    // Destroy session
    void destroy_session(Request& req, Response& resp);

private:
    Cookie create_session_cookie(const std::string& session_id);
};

// ============================================================================
// Middleware Factory
// ============================================================================

// Create session middleware with in-memory store
Middleware sessions(SessionOptions options = {});

// Create session middleware with custom store
Middleware sessions(std::shared_ptr<SessionStore> store, SessionOptions options = {});

// ============================================================================
// Request Extension
// ============================================================================

// Get session from request (convenience function)
inline std::shared_ptr<Session> session(Request& req) {
    return SessionMiddleware::get_session(req);
}

} // namespace coroute
