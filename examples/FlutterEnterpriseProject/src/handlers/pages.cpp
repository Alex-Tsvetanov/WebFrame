#include "pages.hpp"
#include "services/user_service.hpp"
#include "coroute/core/cookie.hpp"
#include "viewmodels/dashboard_vm.hpp"
#include "viewmodels/login_vm.hpp"

namespace project::handlers::pages
{

	void register_routes(coroute::App& app, [[maybe_unused]] services::UserService& user_service)
	{
		// Home page / Dashboard
		app.view<viewmodels::DashboardVm>("/",
		        [](coroute::Request& req) -> coroute::View<viewmodels::DashboardVm>
		        {
					viewmodels::DashboardVm vm;
					vm.title = "Task Dashboard";
					vm.authenticated = false;

					auto cookies = coroute::CookieJar::from_request(req);
					auto username_cookie = cookies.get("username");
					if (username_cookie)
					{
						vm.authenticated = true;
						vm.username = std::string(*username_cookie);
					}

					co_return coroute::ViewResult<viewmodels::DashboardVm>{
						.templates = coroute::ViewTemplates("pages/index.html", "DashboardScreen"),
						.model = std::move(vm)
					};
				});

		// Login page
		app.view<viewmodels::LoginVm>("/login",
		        [](coroute::Request& req) -> coroute::View<viewmodels::LoginVm>
		        {
					viewmodels::LoginVm vm;
					vm.title = "Login";
					vm.error = "";

					co_return coroute::ViewResult<viewmodels::LoginVm>{
						.templates = coroute::ViewTemplates("pages/login.html", "LoginScreen"),
						.model = std::move(vm)
					};
				});

		// Logout (redirect after clearing session)
		app.get("/logout",
		        [](coroute::Request&) -> coroute::Task<coroute::Response>
		        {
					auto response = coroute::Response::redirect("/login");
					// Clear cookies by setting them to expire
					response.set_header("Set-Cookie", "user_id=; Path=/; Max-Age=0");
					response.add_header("Set-Cookie", "username=; Path=/; Max-Age=0");
					co_return response;
				});
	}

}  // namespace project::handlers::pages
