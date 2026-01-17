#include "auth.hpp"
#include "coroute/core/cookie.hpp"

namespace project::middleware {

coroute::Middleware require_auth() {
    return [](coroute::Request& req, coroute::Next next) -> coroute::Task<coroute::Response> {
        // Check if user is authenticated via cookie
        auto cookies = coroute::CookieJar::from_request(req);
        auto user_id_cookie = cookies.get("user_id");
        
        if (!user_id_cookie) {
            // Not authenticated - return 401
            nlohmann::json error;
            error["error"]["code"] = 401;
            error["error"]["message"] = "Authentication required";
            coroute::Response resp;
            resp.set_status(401);
            resp.set_body(error.dump());
            resp.set_header("Content-Type", "application/json");
            co_return resp;
        }
        
        // User is authenticated, continue
        co_return co_await next(req);
    };
}

coroute::Middleware load_user() {
    return [](coroute::Request& req, coroute::Next next) -> coroute::Task<coroute::Response> {
        // Load user info from cookies into request context
        auto cookies = coroute::CookieJar::from_request(req);
        auto user_id_cookie = cookies.get("user_id");
        auto username_cookie = cookies.get("username");
        
        if (user_id_cookie) {
            try {
                int64_t user_id = std::stoll(std::string(*user_id_cookie));
                req.set_context("user_id", user_id);
                req.set_context("username", username_cookie ? std::string(*username_cookie) : "");
                req.set_context("authenticated", true);
            } catch (...) {
                req.set_context("authenticated", false);
            }
        } else {
            req.set_context("authenticated", false);
        }
        
        co_return co_await next(req);
    };
}

} // namespace project::middleware
