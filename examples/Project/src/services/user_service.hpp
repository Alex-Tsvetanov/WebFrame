#pragma once

#include "models/user.hpp"
#include <vector>
#include <optional>
#include <unordered_map>
#include <mutex>

namespace project::services {

class UserService {
public:
    UserService();
    
    // Authentication
    std::optional<models::User> authenticate(const models::LoginRequest& req);
    models::User register_user(const models::RegisterRequest& req);
    
    // CRUD
    std::optional<models::User> find_by_id(int64_t id);
    std::optional<models::User> find_by_username(const std::string& username);
    std::vector<models::User> list();
    
private:
    // Simple in-memory storage (replace with database in production)
    std::unordered_map<int64_t, models::User> users_;
    std::unordered_map<std::string, int64_t> username_index_;
    int64_t next_id_ = 1;
    mutable std::mutex mutex_;
    
    // Simple password hashing (use bcrypt/argon2 in production!)
    static std::string hash_password(const std::string& password);
    static bool verify_password(const std::string& password, const std::string& hash);
};

} // namespace project::services
