#include "users.hpp"
#include "services/user_service.hpp"
#include "models/user.hpp"
#include "coroute/core/cookie.hpp"

namespace project::handlers::api::users {

// Helper to create JSON error response
static coroute::Response json_error(int code, const std::string& message) {
    nlohmann::json err;
    err["error"]["code"] = code;
    err["error"]["message"] = message;
    
    coroute::Response resp;
    resp.set_status(code);
    resp.set_body(err.dump());
    resp.set_header("Content-Type", "application/json");
    return resp;
}

void register_routes(coroute::App& app, services::UserService& user_service) {
    
    // POST /api/login - Authenticate user
    app.post("/api/login", [&user_service](coroute::Request& req) -> coroute::Task<coroute::Response> {
        try {
            auto body_str = req.body();
            if (body_str.empty()) {
                co_return json_error(400, "Request body is empty");
            }
            
            auto body = nlohmann::json::parse(body_str);
            
            // Validate request
            if (auto error = models::LoginRequest::validate(body)) {
                co_return json_error(400, *error);
            }
            
            auto login_req = models::LoginRequest::from_json(body);
            auto user = user_service.authenticate(login_req);
            
            if (!user) {
                co_return json_error(401, "Invalid username or password");
            }
            
            // Set cookies for session
            auto response = coroute::Response::json(user->to_json().dump());
            response.add_header("Set-Cookie", "user_id=" + std::to_string(user->id) + "; Path=/; HttpOnly");
            response.add_header("Set-Cookie", "username=" + user->username + "; Path=/");
            co_return response;
            
        } catch (const nlohmann::json::exception& e) {
            co_return json_error(400, std::string("Invalid JSON: ") + e.what());
        } catch (const std::exception& e) {
            co_return json_error(500, std::string("Internal error: ") + e.what());
        }
    });
    
    // POST /api/logout - Clear session
    app.post("/api/logout", [](coroute::Request&) -> coroute::Task<coroute::Response> {
        nlohmann::json resp_body;
        resp_body["message"] = "Logged out successfully";
        
        auto response = coroute::Response::json(resp_body.dump());
        response.add_header("Set-Cookie", "user_id=; Path=/; Max-Age=0");
        response.add_header("Set-Cookie", "username=; Path=/; Max-Age=0");
        co_return response;
    });
    
    // GET /api/me - Get current user info
    app.get("/api/me", [&user_service](coroute::Request& req) -> coroute::Task<coroute::Response> {
        auto cookies = coroute::CookieJar::from_request(req);
        auto user_id_cookie = cookies.get("user_id");
        
        if (!user_id_cookie) {
            co_return json_error(401, "Not authenticated");
        }
        
        int64_t user_id = 0;
        try {
            user_id = std::stoll(std::string(*user_id_cookie));
        } catch (...) {
            co_return json_error(401, "Invalid session");
        }
        
        auto user = user_service.find_by_id(user_id);
        
        if (!user) {
            co_return json_error(404, "User not found");
        }
        
        co_return coroute::Response::json(user->to_json().dump());
    });
    
    // GET /api/users - List all users (for task assignment dropdown)
    app.get("/api/users", [&user_service](coroute::Request&) -> coroute::Task<coroute::Response> {
        auto users = user_service.list();
        
        nlohmann::json arr = nlohmann::json::array();
        for (const auto& user : users) {
            arr.push_back(user.to_json());
        }
        
        co_return coroute::Response::json(arr.dump());
    });
}

} // namespace project::handlers::api::users
