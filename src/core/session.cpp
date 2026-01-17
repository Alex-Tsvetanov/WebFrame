#include "coroute/core/session.hpp"

#include <algorithm>
#include <iomanip>
#include <sstream>

namespace coroute {

// ============================================================================
// Session Implementation
// ============================================================================

Session::Session(std::string id, bool is_new)
    : id_(std::move(id))
    , created_(std::chrono::system_clock::now())
    , last_accessed_(created_)
    , is_new_(is_new)
{}

void Session::remove(const std::string& key) {
    if (data_.erase(key) > 0) {
        modified_ = true;
    }
}

bool Session::has(const std::string& key) const {
    return data_.count(key) > 0;
}

void Session::clear() {
    if (!data_.empty()) {
        data_.clear();
        modified_ = true;
    }
}

std::vector<std::string> Session::keys() const {
    std::vector<std::string> result;
    result.reserve(data_.size());
    for (const auto& [key, _] : data_) {
        result.push_back(key);
    }
    return result;
}

void Session::touch() {
    last_accessed_ = std::chrono::system_clock::now();
}

// ============================================================================
// MemorySessionStore Implementation
// ============================================================================

std::shared_ptr<Session> MemorySessionStore::load(const std::string& id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = sessions_.find(id);
    if (it == sessions_.end()) {
        return nullptr;
    }
    
    // Check if expired
    if (it->second.expires < std::chrono::system_clock::now()) {
        sessions_.erase(it);
        return nullptr;
    }
    
    it->second.session->touch();
    return it->second.session;
}

void MemorySessionStore::save(const Session& session) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto& stored = sessions_[session.id()];
    if (!stored.session) {
        stored.session = std::make_shared<Session>(session.id());
    }
    
    // Copy data (simplified - in real impl would serialize)
    *stored.session = session;
    stored.session->mark_saved();
    stored.expires = std::chrono::system_clock::now() + default_max_age_;
}

void MemorySessionStore::destroy(const std::string& id) {
    std::lock_guard<std::mutex> lock(mutex_);
    sessions_.erase(id);
}

void MemorySessionStore::cleanup(std::chrono::seconds max_age) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto now = std::chrono::system_clock::now();
    auto cutoff = now - max_age;
    
    for (auto it = sessions_.begin(); it != sessions_.end(); ) {
        if (it->second.session->last_accessed_at() < cutoff) {
            it = sessions_.erase(it);
        } else {
            ++it;
        }
    }
}

std::string MemorySessionStore::generate_id() {
    static thread_local std::random_device rd;
    static thread_local std::mt19937_64 gen(rd());
    static thread_local std::uniform_int_distribution<uint64_t> dis;
    
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    oss << std::setw(16) << dis(gen);
    oss << std::setw(16) << dis(gen);
    return oss.str();
}

size_t MemorySessionStore::size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return sessions_.size();
}

// ============================================================================
// SessionMiddleware Implementation
// ============================================================================

// Key for storing session in request context
static const std::string SESSION_KEY = "__coroute_session";

SessionMiddleware::SessionMiddleware(std::shared_ptr<SessionStore> store, SessionOptions options)
    : store_(std::move(store))
    , options_(std::move(options))
{}

Cookie SessionMiddleware::create_session_cookie(const std::string& session_id) {
    Cookie c;
    c.name = options_.cookie_name;
    c.value = session_id;
    c.path = options_.cookie_path;
    if (!options_.cookie_domain.empty()) {
        c.domain = options_.cookie_domain;
    }
    c.max_age = options_.max_age;
    c.secure = options_.secure;
    c.http_only = options_.http_only;
    c.same_site = options_.same_site;
    return c;
}

Task<Response> SessionMiddleware::operator()(Request& req, Next next) {
    // Get session ID from cookie
    auto jar = cookies(req);
    auto session_id = jar.get(options_.cookie_name);
    
    std::shared_ptr<Session> sess;
    bool need_set_cookie = false;
    
    if (session_id) {
        // Try to load existing session
        sess = store_->load(std::string(*session_id));
    }
    
    if (!sess) {
        // Create new session
        auto new_id = store_->generate_id();
        sess = std::make_shared<Session>(new_id, true);
        need_set_cookie = true;
    }
    
    // Store session in request context
    req.set_context(SESSION_KEY, sess);
    
    // Call next handler
    Response resp = co_await next(req);
    
    // Auto-save if modified
    if (options_.auto_save && sess->is_modified()) {
        store_->save(*sess);
    }
    
    // Set cookie if new session or rolling
    if (need_set_cookie || (options_.rolling && !sess->is_new())) {
        set_cookie(resp, create_session_cookie(sess->id()));
    }
    
    co_return resp;
}

std::shared_ptr<Session> SessionMiddleware::get_session(Request& req) {
    auto ctx = req.get_context<std::shared_ptr<Session>>(SESSION_KEY);
    return ctx.value_or(nullptr);
}

void SessionMiddleware::destroy_session(Request& req, Response& resp) {
    auto sess = get_session(req);
    if (sess) {
        store_->destroy(sess->id());
        delete_cookie(resp, options_.cookie_name, options_.cookie_path, options_.cookie_domain);
    }
}

// ============================================================================
// Middleware Factory
// ============================================================================

Middleware sessions(SessionOptions options) {
    auto store = std::make_shared<MemorySessionStore>(options.max_age);
    return sessions(std::move(store), std::move(options));
}

Middleware sessions(std::shared_ptr<SessionStore> store, SessionOptions options) {
    auto middleware = std::make_shared<SessionMiddleware>(std::move(store), std::move(options));
    
    return [middleware](Request& req, Next next) -> Task<Response> {
        co_return co_await (*middleware)(req, std::move(next));
    };
}

} // namespace coroute
