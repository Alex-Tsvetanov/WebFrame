#include "tasks.hpp"
#include "services/task_service.hpp"
#include "models/task.hpp"
#include "coroute/core/cookie.hpp"

namespace project::handlers::api::tasks {

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

// Helper to create JSON success response with status
static coroute::Response json_response(const std::string& body, int status = 200) {
    coroute::Response resp;
    resp.set_status(status);
    resp.set_body(body);
    resp.set_header("Content-Type", "application/json");
    return resp;
}

// Helper to check if user is authenticated
static bool is_authenticated(const coroute::Request& req) {
    auto cookies = coroute::CookieJar::from_request(req);
    return cookies.has("user_id");
}

// Helper to get user ID from cookie
static int64_t get_user_id(const coroute::Request& req) {
    auto cookies = coroute::CookieJar::from_request(req);
    auto user_id_cookie = cookies.get("user_id");
    if (!user_id_cookie) return 0;
    try {
        return std::stoll(std::string(*user_id_cookie));
    } catch (...) {
        return 0;
    }
}

void register_routes(coroute::App& app, services::TaskService& task_service) {
    
    // GET /api/stats/tasks - Get task statistics (different path to avoid {id} conflict)
    app.get("/api/stats/tasks", [&task_service](coroute::Request&) -> coroute::Task<coroute::Response> {
        try {
            auto stats = task_service.get_stats();
            
            nlohmann::json response;
            response["total"] = stats.total;
            response["pending"] = stats.pending;
            response["in_progress"] = stats.in_progress;
            response["completed"] = stats.completed;
            
            co_return coroute::Response::json(response.dump());
        } catch (const std::exception& e) {
            co_return json_error(500, std::string("Stats error: ") + e.what());
        }
    });
    
    // GET /api/tasks - List all tasks
    app.get("/api/tasks", [&task_service](coroute::Request& req) -> coroute::Task<coroute::Response> {
        try {
            models::TaskFilter filter;
            
            // Parse query parameters for filtering
            if (auto status = req.query_opt<std::string>("status")) {
                filter.status = models::status_from_string(*status);
            }
            if (auto user_id = req.query_opt<int64_t>("user_id")) {
                filter.user_id = *user_id;
            }
            if (auto limit = req.query_opt<int>("limit")) {
                filter.limit = *limit;
            }
            if (auto offset = req.query_opt<int>("offset")) {
                filter.offset = *offset;
            }
            
            auto tasks = task_service.list(filter);
            co_return coroute::Response::json(models::to_json_array(tasks).dump());
        } catch (const std::exception& e) {
            co_return json_error(500, std::string("List error: ") + e.what());
        }
    });
    
    // POST /api/tasks - Create a new task (requires auth)
    app.post("/api/tasks", [&task_service](coroute::Request& req) -> coroute::Task<coroute::Response> {
        // Require authentication
        if (!is_authenticated(req)) {
            co_return json_error(401, "Authentication required");
        }
        
        try {
            auto body = nlohmann::json::parse(req.body());
            
            // Validate request
            if (auto error = models::CreateTaskRequest::validate(body)) {
                co_return json_error(400, *error);
            }
            
            auto create_req = models::CreateTaskRequest::from_json(body);
            int64_t created_by = get_user_id(req);
            
            auto task = task_service.create(create_req, created_by);
            co_return json_response(task.to_json().dump(), 201);
            
        } catch (const nlohmann::json::exception&) {
            co_return json_error(400, "Invalid JSON");
        }
    });
    
    // GET /api/tasks/{id} - Get a specific task
    app.get("/api/tasks/{id}", [&task_service](coroute::Request& req) -> coroute::Task<coroute::Response> {
        auto id_result = req.param<int64_t>(0);
        if (!id_result) {
            co_return json_error(400, "Invalid task ID");
        }
        
        auto task = task_service.find(*id_result);
        if (!task) {
            co_return json_error(404, "Task not found");
        }
        
        co_return coroute::Response::json(task->to_json().dump());
    });
    
    // PUT /api/tasks/{id} - Update a task (requires auth)
    app.put("/api/tasks/{id}", [&task_service](coroute::Request& req) -> coroute::Task<coroute::Response> {
        // Require authentication
        if (!is_authenticated(req)) {
            co_return json_error(401, "Authentication required");
        }
        
        auto id_result = req.param<int64_t>(0);
        if (!id_result) {
            co_return json_error(400, "Invalid task ID");
        }
        
        try {
            auto body = nlohmann::json::parse(req.body());
            auto update_req = models::UpdateTaskRequest::from_json(body);
            
            auto task = task_service.update(*id_result, update_req);
            if (!task) {
                co_return json_error(404, "Task not found");
            }
            
            co_return coroute::Response::json(task->to_json().dump());
            
        } catch (const nlohmann::json::exception&) {
            co_return json_error(400, "Invalid JSON");
        }
    });
    
    // DELETE /api/tasks/{id} - Delete a task (requires auth)
    app.del("/api/tasks/{id}", [&task_service](coroute::Request& req) -> coroute::Task<coroute::Response> {
        // Require authentication
        if (!is_authenticated(req)) {
            co_return json_error(401, "Authentication required");
        }
        
        auto id_result = req.param<int64_t>(0);
        if (!id_result) {
            co_return json_error(400, "Invalid task ID");
        }
        
        if (!task_service.remove(*id_result)) {
            co_return json_error(404, "Task not found");
        }
        
        co_return json_response("{}", 204);
    });
}

} // namespace project::handlers::api::tasks
