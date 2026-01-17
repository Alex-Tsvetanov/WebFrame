#include "task.hpp"

namespace project::models {

std::string to_string(TaskStatus status) {
    switch (status) {
        case TaskStatus::Pending: return "pending";
        case TaskStatus::InProgress: return "in_progress";
        case TaskStatus::Completed: return "completed";
    }
    return "pending";
}

TaskStatus status_from_string(const std::string& s) {
    if (s == "in_progress") return TaskStatus::InProgress;
    if (s == "completed") return TaskStatus::Completed;
    return TaskStatus::Pending;
}

nlohmann::json Task::to_json() const {
    return {
        {"id", id},
        {"title", title},
        {"description", description},
        {"status", to_string(status)},
        {"user_id", user_id},
        {"created_by", created_by},
        {"created_at", created_at},
        {"updated_at", updated_at}
    };
}

Task Task::from_json(const nlohmann::json& j) {
    Task task;
    task.id = j.value("id", int64_t{0});
    task.title = j.value("title", "");
    task.description = j.value("description", "");
    task.status = status_from_string(j.value("status", "pending"));
    task.user_id = j.value("user_id", int64_t{0});
    task.created_by = j.value("created_by", int64_t{0});
    task.created_at = j.value("created_at", int64_t{0});
    task.updated_at = j.value("updated_at", int64_t{0});
    return task;
}

std::optional<std::string> Task::validate(const nlohmann::json& j) {
    if (!j.contains("title") || !j["title"].is_string()) {
        return "title is required";
    }
    if (j["title"].get<std::string>().empty()) {
        return "title cannot be empty";
    }
    return std::nullopt;
}

// CreateTaskRequest
CreateTaskRequest CreateTaskRequest::from_json(const nlohmann::json& j) {
    return {
        .title = j.value("title", ""),
        .description = j.value("description", ""),
        .user_id = j.value("user_id", int64_t{0})
    };
}

std::optional<std::string> CreateTaskRequest::validate(const nlohmann::json& j) {
    if (!j.contains("title") || j["title"].get<std::string>().empty()) {
        return "title is required";
    }
    return std::nullopt;
}

// UpdateTaskRequest
UpdateTaskRequest UpdateTaskRequest::from_json(const nlohmann::json& j) {
    UpdateTaskRequest req;
    if (j.contains("title")) {
        req.title = j["title"].get<std::string>();
    }
    if (j.contains("description")) {
        req.description = j["description"].get<std::string>();
    }
    if (j.contains("status")) {
        req.status = status_from_string(j["status"].get<std::string>());
    }
    if (j.contains("user_id")) {
        req.user_id = j["user_id"].get<int64_t>();
    }
    return req;
}

nlohmann::json to_json_array(const std::vector<Task>& tasks) {
    nlohmann::json arr = nlohmann::json::array();
    for (const auto& task : tasks) {
        arr.push_back(task.to_json());
    }
    return arr;
}

} // namespace project::models
