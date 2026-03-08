#include "tasks.hpp"
#include "services/task_service.hpp"
#include "models/task.hpp"
#include "coroute/core/cookie.hpp"
#include "coroute/core/app.hpp"

#include <iostream>

namespace flutter_project::handlers::api::tasks
{

	// Helper to create JSON error response
	static coroute::Response json_error(int code, const std::string& message)
	{
		nlohmann::json err;
		err["error"]["code"] = code;
		err["error"]["message"] = message;

		coroute::Response resp;
		resp.set_status(code);
		resp.set_body(err.dump());
		resp.set_header("Content-Type", "application/json");
		return resp;
	}

	// Helper to create JSON success response with status
	static coroute::Response json_response(const std::string& body, int status = 200)
	{
		coroute::Response resp;
		resp.set_status(status);
		resp.set_body(body);
		resp.set_header("Content-Type", "application/json");
		return resp;
	}

	// Helper to check if user is authenticated
	static bool is_authenticated(const coroute::Request& req)
	{
		auto cookies = coroute::CookieJar::from_request(req);
		return cookies.has("user_id");
	}

	// Helper to get user ID from cookie
	static int64_t get_user_id(const coroute::Request& req)
	{
		auto cookies = coroute::CookieJar::from_request(req);
		auto user_id_cookie = cookies.get("user_id");
		if (!user_id_cookie) return 0;
		try
		{
			return std::stoll(std::string(*user_id_cookie));
		}
		catch (const std::exception&)
		{
			return 0;
		}
	}

	// Retrieve the TaskService from the App's stored service pointer.
	// Stored once at register_routes time; safe to dereference for the app lifetime.
	static services::TaskService* g_task_service = nullptr;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

	void register_routes(coroute::App& app, services::TaskService& task_service)
	{
		g_task_service = &task_service;

		// GET /api/stats/tasks - Get task statistics
		app.get("/api/stats/tasks",
		        [](coroute::Request /*req*/) -> coroute::Task<coroute::Response>
		        {
					try
					{
						auto stats = g_task_service->get_stats();

						nlohmann::json response;
						response["total"] = stats.total;
						response["pending"] = stats.pending;
						response["in_progress"] = stats.in_progress;
						response["completed"] = stats.completed;

						co_return coroute::Response::json(response.dump());
					}
					catch (const std::exception& e)
					{
						co_return json_error(500, std::string("Stats error: ") + e.what());
					}
				});

		// GET /api/tasks - List all tasks
		app.get("/api/tasks",
		        [](coroute::Request req) -> coroute::Task<coroute::Response>
		        {
					try
					{
						models::TaskFilter filter;

						if (auto status = req.query_opt<std::string>("status"))
						{
							filter.status = models::status_from_string(*status);
						}
						if (auto user_id = req.query_opt<int64_t>("user_id"))
						{
							filter.user_id = user_id;
						}
						if (auto limit = req.query_opt<int>("limit"))
						{
							filter.limit = *limit;
						}
						if (auto offset = req.query_opt<int>("offset"))
						{
							filter.offset = *offset;
						}

						auto task_list = g_task_service->list(filter);
						co_return coroute::Response::json(models::to_json_array(task_list).dump());
					}
					catch (const std::exception& e)
					{
						co_return json_error(500, std::string("List error: ") + e.what());
					}
				});

		// POST /api/tasks - Create a new task (requires auth)
		app.post("/api/tasks",
		         [](coroute::Request req) -> coroute::Task<coroute::Response>
		         {
					 if (!is_authenticated(req))
					 {
						 co_return json_error(401, "Authentication required");
					 }

					 try
					 {
						 auto body = nlohmann::json::parse(req.body());

						 if (auto error = models::CreateTaskRequest::validate(body))
						 {
							 co_return json_error(400, *error);
						 }

						 auto create_req = models::CreateTaskRequest::from_json(body);
						 int64_t created_by = get_user_id(req);

						 auto task = g_task_service->create(create_req, created_by);
						 co_return json_response(task.to_json().dump(), 201);
					 }
					 catch (const nlohmann::json::exception&)
					 {
						 co_return json_error(400, "Invalid JSON");
					 }
				 });

		// GET /api/tasks/{id} - Get a specific task
		app.get("/api/tasks/{id}",
		        [](coroute::Request req) -> coroute::Task<coroute::Response>
		        {
					auto id_result = req.param<int64_t>(0);
					if (!id_result)
					{
						co_return json_error(400, "Invalid task ID");
					}

					auto task = g_task_service->find(*id_result);
					if (!task)
					{
						co_return json_error(404, "Task not found");
					}

					co_return coroute::Response::json(task->to_json().dump());
				});

		// PUT /api/tasks/{id} - Update a task (requires auth)
		app.put("/api/tasks/{id}",
		        [](coroute::Request req) -> coroute::Task<coroute::Response>
		        {
					if (!is_authenticated(req))
					{
						co_return json_error(401, "Authentication required");
					}

					auto id_result = req.param<int64_t>(0);
					if (!id_result)
					{
						co_return json_error(400, "Invalid task ID");
					}

					try
					{
						auto body = nlohmann::json::parse(req.body());
						auto update_req = models::UpdateTaskRequest::from_json(body);

						auto task = g_task_service->update(*id_result, update_req);
						if (!task)
						{
							co_return json_error(404, "Task not found");
						}

						co_return coroute::Response::json(task->to_json().dump());
					}
					catch (const nlohmann::json::exception&)
					{
						co_return json_error(400, "Invalid JSON");
					}
				});

		// POST /api/tasks/{id} - Update a task via POST (Flutter alias for PUT).
		// Bridge.submitAction always sends POST; this mirrors the PUT handler above.
		app.post("/api/tasks/{id}",
		         [](coroute::Request req) -> coroute::Task<coroute::Response>
		         {
					 if (!is_authenticated(req))
					 {
						 co_return json_error(401, "Authentication required");
					 }

					 auto id_result = req.param<int64_t>(0);
					 if (!id_result)
					 {
						 co_return json_error(400, "Invalid task ID");
					 }

					 try
					 {
						 auto body = nlohmann::json::parse(req.body());
						 auto update_req = models::UpdateTaskRequest::from_json(body);

						 auto task = g_task_service->update(*id_result, update_req);
						 if (!task)
						 {
							 co_return json_error(404, "Task not found");
						 }

						 co_return coroute::Response::json(task->to_json().dump());
					 }
					 catch (const nlohmann::json::exception&)
					 {
						 co_return json_error(400, "Invalid JSON");
					 }
				 });

		// POST /api/tasks/{id}/delete - Delete sentinel used by Flutter.
		// Bridge.submitAction cannot send DELETE; this route handles it.
		app.post("/api/tasks/{id}/delete",
		         [](coroute::Request req) -> coroute::Task<coroute::Response>
		         {
					 if (!is_authenticated(req))
					 {
						 co_return json_error(401, "Authentication required");
					 }

					 auto id_result = req.param<int64_t>(0);
					 if (!id_result)
					 {
						 co_return json_error(400, "Invalid task ID");
					 }

					 if (!g_task_service->remove(*id_result))
					 {
						 co_return json_error(404, "Task not found");
					 }

					 co_return json_response("{}", 200);
				 });

		// DELETE /api/tasks/{id} - Delete a task (requires auth)
		app.del("/api/tasks/{id}",
		        [](coroute::Request req) -> coroute::Task<coroute::Response>
		        {
					if (!is_authenticated(req))
					{
						co_return json_error(401, "Authentication required");
					}

					auto id_result = req.param<int64_t>(0);
					if (!id_result)
					{
						co_return json_error(400, "Invalid task ID");
					}

					if (!g_task_service->remove(*id_result))
					{
						co_return json_error(404, "Task not found");
					}

					co_return json_response("{}", 204);
				});
	}

}  // namespace flutter_project::handlers::api::tasks
