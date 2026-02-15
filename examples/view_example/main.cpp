// View Example - Demonstrates the new Coroute architecture
//
// This example showcases:
// 1. Business logic in API routes
// 2. Views orchestrating data via app.fetch()
// 3. Authentication propagation (for desktop/mobile clients)
// 4. Minimal view middleware (UI-level concerns only)

#include "coroute/coroute.hpp"
#include <filesystem>
#include <iostream>

using namespace coroute;

// ============================================================================
// ViewModels
// ============================================================================

#include "viewmodels/listing_vm.hpp"
#include "viewmodels/user_vm.hpp"
#include "viewmodels/login_vm.hpp"

// ============================================================================
// HTTP MIDDLEWARE (Transport Layer)
// - Runs for ALL requests (API and Views)
// - Handles cookies, server_sessions, headers, compression
// - IS web-specific
// ============================================================================

Task<Response> logging_middleware(Request& req, Next next)
{
	std::cout << "[HTTP MW] " << method_to_string(req.method()) << " " << req.path() << std::endl;
	auto resp = co_await next(req);
	std::cout << "[HTTP MW] Response: " << resp.status() << std::endl;
	co_return resp;
}

// ============================================================================
// API ROUTES (Business Logic)
// - All business logic lives here
// - Views fetch data from these endpoints
// - Auth is checked here, not in views
// ============================================================================

// Simulated user database
struct UserData
{
	std::string name;
	std::string email;
	bool is_admin;
};

std::unordered_map<std::string, UserData> user_db = {
	{"alice", {"Alice", "alice@example.com", true}},
	{  "bob",    {"Bob", "bob@example.com", false}},
};

// Simulated session storage
std::unordered_map<std::string, std::string> server_sessions;  // token -> username

// ============================================================================
// VIEW MIDDLEWARE (UI-Level Concerns Only)
// - Runs only for VIEW routes
// - CANNOT produce Response or access HTTP details
// - For access gating, ViewModel enrichment, etc.
// ============================================================================

Task<void> log_view_access(ViewExecutionContext& ctx)
{
	std::cout << "[VIEW MW] Rendering view for route: " << ctx.route << std::endl;
	co_return;
}

// Get the directory containing this source file at compile time
static std::filesystem::path source_dir() { return std::filesystem::path(__FILE__).parent_path(); }

int main()
{
	try
	{
		std::cout << "Starting view_example..." << std::endl;
		App app;

		// Configure template engine
		std::cout << "Setting templates dir..." << std::endl;
		app.set_templates(source_dir() / "templates/web");

		// ========================================================================
		// HTTP Middleware Registration (runs for ALL routes)
		// ========================================================================
		app.use(logging_middleware);

		// ========================================================================
		// Global View Middleware (UI-level, runs for all views)
		// ========================================================================
		app.use_view(log_view_access);

		// ========================================================================
		// API Routes (Business Logic)
		// ========================================================================

		// Login endpoint - returns auth token
		app.post("/api/login",
		         [](Request& req) -> Task<Response>
		         {
					 // In real app, would parse JSON body
					 for (auto& header : req.query_params())
					 {
						 std::cout << header.first << ": " << header.second << std::endl;
					 }
					 std::string username = req.query_opt<std::string>("user").value_or("guest");
					 std::cout << "Login attempt for user: " << username << std::endl;
					 for (auto& user : user_db)
					 {
						 std::cout << "DB User: " << user.first << std::endl;
					 }

					 if (user_db.count(username))
					 {
						 // Create session
						 std::string token = "token_" + username;  // In real app, use secure random
						 server_sessions[token] = username;
						 std::cout << "Session created: " << token << " -> " << username << std::endl;

						 // Return success with Set-Cookie
						 Response resp = Response::json(R"({"status":"ok"})");
						 resp.add_header("Set-Cookie", "auth=" + token + "; HttpOnly; Path=/");
						 co_return resp;
					 }

					 std::cout << "Login failed: Unknown user" << std::endl;
					 co_return Response::json(R"({"status":"error","message":"Unknown user"})");
				 });

		// Get current user (requires auth)
		app.get("/api/user",
		        [](Request& req) -> Task<Response>
		        {
					// Check auth via cookie (case-insensitive)
					std::string cookie_val;
					std::cout << "[API] Request Headers:" << std::endl;
					for (const auto& h : req.headers())
					{
						std::cout << "  - " << h.first << ": " << h.second << std::endl;
						// Simple case-insensitive check
						std::string k = h.first;
						std::transform(k.begin(), k.end(), k.begin(), ::tolower);
						if (k == "cookie")
						{
							cookie_val = h.second;
						}
					}

					if (!cookie_val.empty())
					{
						std::cout << "[API] Found Cookie header: '" << cookie_val << "'" << std::endl;
						// Parse auth cookie (simplified)
						auto pos = cookie_val.find("auth=");
						if (pos != std::string::npos)
						{
							std::string token = std::string(cookie_val.substr(pos + 5));
							auto semi = token.find(';');
							if (semi != std::string::npos) token = token.substr(0, semi);

							// Trim whitespace just in case
							token.erase(token.find_last_not_of(" \n\r\t") + 1);

							std::cout << "[API] Parsed token: '" << token << "'" << std::endl;
							std::cout << "[API] Active sessions: " << server_sessions.size() << std::endl;
							for (const auto& s : server_sessions)
							{
								std::cout << "  - " << s.first << " -> " << s.second << std::endl;
							}

							if (server_sessions.count(token))
							{
								std::string username = server_sessions[token];
								std::cout << "[API] Found session for user: " << username << std::endl;
								if (user_db.count(username))
								{
									auto& user = user_db[username];
									nlohmann::json j = {
										{    "name",     user.name},
                                        {   "email",    user.email},
                                        {"is_admin", user.is_admin}
                                    };
									co_return Response::json(j.dump());
								}
							}
						}
					}

					co_return Response(401,
			                           {
										   {"Content-Type", "application/json"}
                    },
			                           R"({"error":"not_authenticated"})");
				});

		// List items (public API)
		app.get("/api/items",
		        [](Request&) -> Task<Response>
		        {
					nlohmann::json items = {"Apple", "Banana", "Cherry", "Date"};
					co_return Response::json(items.dump());
				});

		// ========================================================================
		// View Routes (Orchestrate data via fetch)
		// ========================================================================

		// Login View
		app.view<LoginVm>("/login",
		                  [](Request&) -> View<LoginVm>
		                  {
							  co_return ViewResult<LoginVm>{.templates = ViewTemplates("login.html", "LoginScreen"),
			                                                .model = LoginVm{}};
						  });

		// Home view - fetches items from API
		app.view<ListingVm>("/",
		                    [&app](Request&) -> View<ListingVm>
		                    {
								// Fetch data from API route (business logic lives there)
								Response resp = co_await app.fetch_get("/api/items");

								std::vector<std::string> items;
								if (resp.status() == 200)
								{
									nlohmann::json j = nlohmann::json::parse(resp.body());
									items = j.get<std::vector<std::string>>();
								}

								ListingVm vm{.title = "Items from API", .items = std::move(items)};
								// Web uses "listing.html", Mobile uses "ListingScreen"
								co_return ViewResult<ListingVm>{
									.templates = ViewTemplates("listing.html", "ListingScreen"),
									.model = std::move(vm)};
							});

		// Profile view - shows current user info
		app.view<UserVm>("/profile",
		                 [&app](Request& req, ViewExecutionContext&) -> View<UserVm>
		                 {
							 // Fetch current user from API
			                 // Pass original request to forward browser cookies
							 Response resp = co_await app.fetch_get(req, "/api/user");

							 UserVm vm;
							 if (resp.status() == 200)
							 {
								 nlohmann::json j = nlohmann::json::parse(resp.body());
								 vm.name = j["name"].get<std::string>();
								 vm.greeting = "Welcome back";
								 vm.logged_in = true;
							 }
							 else if (resp.status() == 401)
							 {
								 vm.name = "Guest";
								 vm.greeting = "Please log in";
								 vm.logged_in = false;
							 }
							 else
							 {
								 // Handle other errors or empty body
								 vm.name = "Guest (Error)";
								 vm.greeting = "Error loading profile";
								 vm.logged_in = false;
							 }

							 // Web uses "user.html", Mobile uses "UserScreen"
							 co_return ViewResult<UserVm>{.templates = ViewTemplates("user.html", "UserScreen"),
			                                              .model = std::move(vm)};
						 });

		// User detail view with route parameter
		app.view<UserVm>("/user/{name}",
		                 [](Request& req, ViewExecutionContext& /*ctx*/) -> View<UserVm>
		                 {
							 expected<std::string, Error> name_param = req.param<std::string>(0);

							 UserVm vm;
							 if (!name_param)
							 {
								 vm.name = "Unknown";
								 vm.greeting = "Hello";
								 vm.logged_in = false;
								 co_return ViewResult<UserVm>{.templates = ViewTemplates("user.html", "UserScreen"),
				                                              .model = std::move(vm)};
							 }
							 else
							 {
								 vm.name = name_param.value();
								 vm.greeting = "Hello";
								 vm.logged_in = false;

								 co_return ViewResult<UserVm>{.templates = ViewTemplates("user.html", "UserScreen"),
				                                              .model = std::move(vm)};
							 }
						 });

		// ========================================================================
		// Startup
		// ========================================================================
		std::cout << "=== Coroute Unified Architecture Demo ===\n\n";
		std::cout << "Key concepts:\n";
		std::cout << "  - Business logic in API routes\n";
		std::cout << "  - Views orchestrate via app.fetch()\n";
		std::cout << "  - Auth propagates automatically\n\n";
		std::cout << "Try:\n";
		std::cout << "  http://localhost:8080/           (home, fetches /api/items)\n";
		std::cout << "  http://localhost:8080/profile    (profile, fetches /api/user)\n";
		std::cout << "  http://localhost:8080/user/Bob   (user detail)\n";
		std::cout << "  http://localhost:8080/api/items  (API endpoint)\n\n";

#ifdef COROUTE_CLIENT_MODE
		app.run(RunOptions{.port = 8080, .api_domain = "http://localhost:8080"});
#else
		// Server mode - no api_domain
		app.run(RunOptions{.port = 8080});
#endif
	}
	catch (const std::exception& e)
	{
		std::cerr << "Fatal error: " << e.what() << std::endl;
		return 1;
	}
	catch (...)
	{
		std::cerr << "Unknown fatal error" << std::endl;
		return 1;
	}
	return 0;
}
