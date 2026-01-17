#include "user_service.hpp"
#include <chrono>
#include <functional>

namespace project::services {

UserService::UserService() {
    // Create a demo user
    models::RegisterRequest demo{
        .username = "demo",
        .email = "demo@example.com",
        .password = "demo123"
    };
    register_user(demo);
}

std::string UserService::hash_password(const std::string& password) {
    // WARNING: This is NOT secure! Use bcrypt/argon2 in production!
    // This is just for demonstration purposes
    std::hash<std::string> hasher;
    return std::to_string(hasher(password + "salt"));
}

bool UserService::verify_password(const std::string& password, const std::string& hash) {
    return hash_password(password) == hash;
}

std::optional<models::User> UserService::authenticate(const models::LoginRequest& req) {
    std::lock_guard lock(mutex_);
    
    auto it = username_index_.find(req.username);
    if (it == username_index_.end()) {
        return std::nullopt;
    }
    
    auto& user = users_[it->second];
    if (!verify_password(req.password, user.password_hash)) {
        return std::nullopt;
    }
    
    return user;
}

models::User UserService::register_user(const models::RegisterRequest& req) {
    std::lock_guard lock(mutex_);
    
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::seconds>(
        now.time_since_epoch()).count();
    
    models::User user{
        .id = next_id_++,
        .username = req.username,
        .email = req.email,
        .password_hash = hash_password(req.password),
        .created_at = timestamp
    };
    
    users_[user.id] = user;
    username_index_[user.username] = user.id;
    
    return user;
}

std::optional<models::User> UserService::find_by_id(int64_t id) {
    std::lock_guard lock(mutex_);
    
    auto it = users_.find(id);
    if (it == users_.end()) {
        return std::nullopt;
    }
    return it->second;
}

std::optional<models::User> UserService::find_by_username(const std::string& username) {
    std::lock_guard lock(mutex_);
    
    auto it = username_index_.find(username);
    if (it == username_index_.end()) {
        return std::nullopt;
    }
    return users_[it->second];
}

std::vector<models::User> UserService::list() {
    std::lock_guard lock(mutex_);
    
    std::vector<models::User> result;
    result.reserve(users_.size());
    for (const auto& [id, user] : users_) {
        result.push_back(user);
    }
    return result;
}

} // namespace project::services
