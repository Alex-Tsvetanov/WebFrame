#include "pages.hpp"
#include "services/user_service.hpp"
#include "services/task_service.hpp"
#include "viewmodels/login_vm.hpp"
#include "viewmodels/dashboard_vm.hpp"
#include "viewmodels/task_vm.hpp"
#include "coroute/core/cookie.hpp"
#include "coroute/view/view_types.hpp"

#include <nlohmann/json.hpp>
#include <format>

using namespace coroute;

namespace flutter_project::handlers::pages
{

	void register_routes(coroute::App& app,
	                     [[maybe_unused]] services::UserService& user_service,
	                     [[maybe_unused]] services::TaskService& task_service)
	{
		// ====================================================================
		// Login View
		// Web:     renders login.html (Inja template)
		// Flutter: mounts "LoginScreen" widget via ScreenRegistry
		// ====================================================================
		app.view<viewmodels::LoginVm>("/login",
		                              [](Request /*req*/) -> View<viewmodels::LoginVm>
		                              {
										  co_return ViewResult<viewmodels::LoginVm>{
											  .templates = ViewTemplates("login.html", "LoginScreen", "LoginScreen"),
											  .model = viewmodels::LoginVm{}};
									  });

		// ====================================================================
		// Dashboard View (Home)
		// Fetches tasks and stats from internal API routes, builds DashboardVm.
		// Web:     renders index.html
		// Flutter: mounts "DashboardScreen" widget
		// ====================================================================
		app.view<viewmodels::DashboardVm>("/",
		                                  [](Request req) -> View<viewmodels::DashboardVm>
		                                  {
											  auto* app_ptr = App::instance();
											  viewmodels::DashboardVm vm;

											  // Resolve authentication state from cookies
											  auto cookies = CookieJar::from_request(req);
											  auto user_id_cookie = cookies.get("user_id");
											  auto username_cookie = cookies.get("username");

											  vm.authenticated = user_id_cookie.has_value();
											  if (username_cookie)
											  {
												  vm.username = std::string(*username_cookie);
											  }

											  // Fetch task list from remote API (auth propagated via req headers)
											  auto tasks_resp = co_await app_ptr->fetch_get(req, "/api/tasks");
											  if (tasks_resp.status() == 200)
											  {
												  try
												  {
													  auto arr = nlohmann::json::parse(tasks_resp.body());
													  for (const auto& t : arr)
													  {
														  vm.tasks.push_back(viewmodels::TaskSummary{
															  .id = t.value("id", int64_t{0}),
															  .title = t.value("title", ""),
															  .description = t.value("description", ""),
															  .status = t.value("status", "pending")});
													  }
												  }
												  catch (const std::exception& e)
												  {
													  std::cerr << "[pages] /api/tasks parse error: " << e.what() << '\n';
												  }
											  }

											  // Fetch stats from remote API
											  auto stats_resp = co_await app_ptr->fetch_get(req, "/api/stats/tasks");
											  if (stats_resp.status() == 200)
											  {
												  try
												  {
													  auto s = nlohmann::json::parse(stats_resp.body());
													  vm.stats.total = s.value("total", 0);
													  vm.stats.pending = s.value("pending", 0);
													  vm.stats.in_progress = s.value("in_progress", 0);
													  vm.stats.completed = s.value("completed", 0);
												  }
												  catch (const std::exception& e)
												  {
													  std::cerr << "[pages] /api/stats/tasks parse error: " << e.what() << '\n';
												  }
											  }

											  co_return ViewResult<viewmodels::DashboardVm>{
												  .templates = ViewTemplates("index.html", "DashboardScreen", "DashboardScreen"),
												  .model = std::move(vm)};
										  });

		// ====================================================================
		// Task Detail View
		// Fetches a single task by ID from the internal API.
		// Web:     renders task_detail.html
		// Flutter: mounts "TaskDetailScreen" widget
		// ====================================================================
		app.view<viewmodels::TaskVm>("/task/{id}",
		                             [](Request req) -> View<viewmodels::TaskVm>
		                             {
										 auto* app_ptr = App::instance();
										 auto id_result = req.param<int64_t>(0);
										 if (!id_result)
										 {
											 co_return ViewResult<viewmodels::TaskVm>{
												 .templates =
												     ViewTemplates("task_detail.html", "TaskDetailScreen", "TaskDetailScreen"),
												 .model = viewmodels::TaskVm{}};
										 }

										 auto task_resp =
								     co_await app_ptr->fetch_get(req, std::format("/api/tasks/{}", *id_result));

										 viewmodels::TaskVm vm;
										 if (task_resp.status() == 200)
										 {
											 try
											 {
												 auto t = nlohmann::json::parse(task_resp.body());
												 vm.id = t.value("id", int64_t{0});
												 vm.title = t.value("title", "");
												 vm.description = t.value("description", "");
												 vm.status = t.value("status", "pending");
												 vm.created_by = t.value("created_by", int64_t{0});
												 vm.created_at = t.value("created_at", int64_t{0});
												 vm.updated_at = t.value("updated_at", int64_t{0});
											 }
											 catch (const std::exception& e)
											 {
												 std::cerr << "[pages] /api/tasks/{id} parse error: " << e.what() << '\n';
											 }
										 }

										 co_return ViewResult<viewmodels::TaskVm>{
											 .templates =
											     ViewTemplates("task_detail.html", "TaskDetailScreen", "TaskDetailScreen"),
											 .model = std::move(vm)};
									 });

		// ====================================================================
		// Logout (web-only: clears cookies and redirects to /login)
		// Flutter clients call POST /api/logout directly via the API.
		// ====================================================================
		app.get("/logout",
		        [](Request /*req*/) -> Task<Response>
		        {
					auto response = Response::redirect("/login");
					response.set_header("Set-Cookie", "user_id=; Path=/; Max-Age=0");
					response.add_header("Set-Cookie", "username=; Path=/; Max-Age=0");
					co_return response;
				});
	}

}  // namespace flutter_project::handlers::pages
