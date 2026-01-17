#include "pages.hpp"
#include "services/user_service.hpp"
#include "coroute/core/cookie.hpp"

namespace project::handlers::pages {

void register_routes(coroute::App& app, [[maybe_unused]] services::UserService& user_service) {
    
    // Home page / Dashboard
    app.get("/", [&app](coroute::Request& req) -> coroute::Task<coroute::Response> {
#ifdef COROUTE_HAS_TEMPLATES
        nlohmann::json data;
        
        // Check if user is logged in via cookie
        auto cookies = coroute::CookieJar::from_request(req);
        auto username_cookie = cookies.get("username");
        data["authenticated"] = username_cookie.has_value();
        if (username_cookie) {
            data["username"] = std::string(*username_cookie);
        }
        
        data["title"] = "Task Dashboard";
        
        co_return app.render_html("pages/index.html", data);
#else
        (void)req;
        co_return coroute::Response::html("<h1>Task Dashboard</h1><p>Templates not enabled</p>");
#endif
    });
    
    // Login page
    app.get("/login", [&app](coroute::Request& req) -> coroute::Task<coroute::Response> {
#ifdef COROUTE_HAS_TEMPLATES
        // Redirect if already logged in
        auto cookies = coroute::CookieJar::from_request(req);
        if (cookies.has("user_id")) {
            co_return coroute::Response::redirect("/");
        }
        
        nlohmann::json data;
        data["title"] = "Login";
        data["error"] = "";
        
        co_return app.render_html("pages/login.html", data);
#else
        (void)req;
        co_return coroute::Response::html(R"(
            <h1>Login</h1>
            <form method="POST" action="/api/login">
                <input name="username" placeholder="Username"><br>
                <input name="password" type="password" placeholder="Password"><br>
                <button type="submit">Login</button>
            </form>
            <p>Demo: username=demo, password=demo123</p>
        )");
#endif
    });
    
    // Logout (redirect after clearing session)
    app.get("/logout", [](coroute::Request&) -> coroute::Task<coroute::Response> {
        auto response = coroute::Response::redirect("/login");
        // Clear cookies by setting them to expire
        response.set_header("Set-Cookie", "user_id=; Path=/; Max-Age=0");
        response.add_header("Set-Cookie", "username=; Path=/; Max-Age=0");
        co_return response;
    });
}

} // namespace project::handlers::pages
