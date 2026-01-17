#include "user.hpp"

namespace project::models {

nlohmann::json User::to_json() const {
    return {
        {"id", id},
        {"username", username},
        {"email", email},
        {"created_at", created_at}
        // Note: password_hash is intentionally excluded
    };
}

User User::from_json(const nlohmann::json& j) {
    User user;
    user.id = j.value("id", int64_t{0});
    user.username = j.value("username", "");
    user.email = j.value("email", "");
    user.created_at = j.value("created_at", int64_t{0});
    return user;
}

std::optional<std::string> User::validate(const nlohmann::json& j) {
    if (!j.contains("username") || !j["username"].is_string()) {
        return "username is required";
    }
    if (j["username"].get<std::string>().empty()) {
        return "username cannot be empty";
    }
    return std::nullopt;
}

// LoginRequest
LoginRequest LoginRequest::from_json(const nlohmann::json& j) {
    return {
        .username = j.value("username", ""),
        .password = j.value("password", "")
    };
}

std::optional<std::string> LoginRequest::validate(const nlohmann::json& j) {
    if (!j.contains("username") || !j["username"].is_string() || j["username"].get<std::string>().empty()) {
        return "username is required";
    }
    if (!j.contains("password") || !j["password"].is_string() || j["password"].get<std::string>().empty()) {
        return "password is required";
    }
    return std::nullopt;
}

// RegisterRequest
RegisterRequest RegisterRequest::from_json(const nlohmann::json& j) {
    return {
        .username = j.value("username", ""),
        .email = j.value("email", ""),
        .password = j.value("password", "")
    };
}

std::optional<std::string> RegisterRequest::validate(const nlohmann::json& j) {
    if (!j.contains("username") || !j["username"].is_string() || j["username"].get<std::string>().empty()) {
        return "username is required";
    }
    if (!j.contains("email") || !j["email"].is_string() || j["email"].get<std::string>().empty()) {
        return "email is required";
    }
    if (!j.contains("password") || !j["password"].is_string() || j["password"].get<std::string>().length() < 6) {
        return "password must be at least 6 characters";
    }
    return std::nullopt;
}

} // namespace project::models
