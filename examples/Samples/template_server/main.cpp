// Template Server Example
// Demonstrates inja template engine integration with App-based configuration
// Similar to v1's app.render() pattern

#include "coroute/core/app.hpp"
#include <iostream>
#include <filesystem>

using namespace coroute;

// Get the directory containing this source file at compile time
static std::filesystem::path source_dir() {
    return std::filesystem::path(__FILE__).parent_path();
}

int main() {
    App app;
    
#ifdef COROUTE_HAS_TEMPLATES
    // Configure template engine - use source file directory + "templates"
    app.set_templates(source_dir() / "templates");
    
    // Add custom template function
    app.add_template_callback("upper", 1, [](inja::Arguments& args) -> nlohmann::json {
        std::string s = args.at(0)->get<std::string>();
        for (char& c : s) c = static_cast<char>(std::toupper(c));
        return s;
    });
    
    // Route with inline template - app is captured by reference
    app.get("/", [&app](Request&) -> Task<Response> {
        nlohmann::json data;
        data["title"] = "Coroute Templates";
        data["message"] = "Hello from inja!";
        data["items"] = {"Apple", "Banana", "Cherry"};
        
        co_return app.render_html("listing.html", data);
    });
    
    // Route with user data - app captured for template rendering
    app.get("/user/{name}", [&app](Request& req) -> Task<Response> {
        // Route params are positional - {name} is index 0
        std::string name = req.param<std::string>(0).value_or("Guest");
        
        nlohmann::json data;
        data["name"] = name;
        data["greeting"] = "Welcome";
        data["logged_in"] = true;
        
        co_return app.render_html("greeting.html", data);
    });
    
    // JSON API endpoint (for comparison)
    app.get("/api/users", [](Request&) -> Task<Response> {
        nlohmann::json users;
        users.push_back({{"id", 1}, {"name", "Alice"}});
        users.push_back({{"id", 2}, {"name", "Bob"}});
        users.push_back({{"id", 3}, {"name", "Charlie"}});
        co_return Response::json(users.dump());
    });
    
    std::cout << "Template server running on http://localhost:8080\n";
    std::cout << "Try:\n";
    std::cout << "  http://localhost:8080/\n";
    std::cout << "  http://localhost:8080/user/YourName\n";
    std::cout << "  http://localhost:8080/api/users\n";
#else
    app.get("/", [](Request&) -> Task<Response> {
        co_return Response::ok("Templates not enabled. Build with coroute_HAS_TEMPLATES");
    });
    std::cout << "Templates not enabled!\n";
#endif
    
    app.run(8080);
    return 0;
}
