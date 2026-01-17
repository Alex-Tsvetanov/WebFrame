#pragma once

#include <string>
#include <cstdint>
#include <optional>
#include <vector>
#include <nlohmann/json.hpp>

namespace project::models {

enum class TaskStatus {
    Pending,
    InProgress,
    Completed
};

// Convert TaskStatus to/from string
std::string to_string(TaskStatus status);
TaskStatus status_from_string(const std::string& s);

struct Task {
    int64_t id = 0;
    std::string title;
    std::string description;
    TaskStatus status = TaskStatus::Pending;
    int64_t user_id = 0;        // Assigned user
    int64_t created_by = 0;     // Creator
    int64_t created_at = 0;
    int64_t updated_at = 0;
    
    // Serialization
    nlohmann::json to_json() const;
    static Task from_json(const nlohmann::json& j);
    
    // Validation
    static std::optional<std::string> validate(const nlohmann::json& j);
};

// Request DTOs
struct CreateTaskRequest {
    std::string title;
    std::string description;
    int64_t user_id = 0;  // Optional: assign to user
    
    static CreateTaskRequest from_json(const nlohmann::json& j);
    static std::optional<std::string> validate(const nlohmann::json& j);
};

struct UpdateTaskRequest {
    std::optional<std::string> title;
    std::optional<std::string> description;
    std::optional<TaskStatus> status;
    std::optional<int64_t> user_id;
    
    static UpdateTaskRequest from_json(const nlohmann::json& j);
};

// Filter for listing tasks
struct TaskFilter {
    std::optional<TaskStatus> status;
    std::optional<int64_t> user_id;
    std::optional<int64_t> created_by;
    int limit = 100;
    int offset = 0;
};

// Helper to convert vector of tasks to JSON array
nlohmann::json to_json_array(const std::vector<Task>& tasks);

} // namespace project::models
