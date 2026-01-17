#include "task_service.hpp"
#include "handlers/websocket/task_hub.hpp"
#include <chrono>
#include <algorithm>

namespace project::services {

TaskService::TaskService(handlers::websocket::TaskHub& hub) : hub_(hub) {
    // Create some demo tasks
    models::CreateTaskRequest demo1{.title = "Welcome to Task Dashboard", .description = "This is a demo task"};
    models::CreateTaskRequest demo2{.title = "Try creating a new task", .description = "Click the + button"};
    models::CreateTaskRequest demo3{.title = "Connect via WebSocket", .description = "Changes sync in real-time"};
    
    create(demo1, 1);
    create(demo2, 1);
    create(demo3, 1);
}

models::Task TaskService::create(const models::CreateTaskRequest& req, int64_t created_by) {
    models::Task task;
    {
        std::lock_guard lock(mutex_);
        
        auto now = std::chrono::system_clock::now();
        auto timestamp = std::chrono::duration_cast<std::chrono::seconds>(
            now.time_since_epoch()).count();
        
        task = models::Task{
            .id = next_id_++,
            .title = req.title,
            .description = req.description,
            .status = models::TaskStatus::Pending,
            .user_id = req.user_id,
            .created_by = created_by,
            .created_at = timestamp,
            .updated_at = timestamp
        };
        
        tasks_[task.id] = task;
    }
    // Broadcast to WebSocket clients (outside lock)
    broadcast_created(task);
    
    return task;
}

std::optional<models::Task> TaskService::find(int64_t id) {
    std::lock_guard lock(mutex_);
    
    auto it = tasks_.find(id);
    if (it == tasks_.end()) {
        return std::nullopt;
    }
    return it->second;
}

std::vector<models::Task> TaskService::list(const models::TaskFilter& filter) {
    std::lock_guard lock(mutex_);
    
    std::vector<models::Task> result;
    result.reserve(tasks_.size());
    
    for (const auto& [id, task] : tasks_) {
        // Apply filters
        if (filter.status && task.status != *filter.status) continue;
        if (filter.user_id && task.user_id != *filter.user_id) continue;
        if (filter.created_by && task.created_by != *filter.created_by) continue;
        
        result.push_back(task);
    }
    
    // Sort by created_at descending (newest first)
    std::sort(result.begin(), result.end(), [](const auto& a, const auto& b) {
        return a.created_at > b.created_at;
    });
    
    // Apply pagination
    if (filter.offset > 0 && static_cast<size_t>(filter.offset) < result.size()) {
        result.erase(result.begin(), result.begin() + filter.offset);
    }
    if (filter.limit > 0 && static_cast<size_t>(filter.limit) < result.size()) {
        result.resize(filter.limit);
    }
    
    return result;
}

std::optional<models::Task> TaskService::update(int64_t id, const models::UpdateTaskRequest& req) {
    models::Task task_copy;
    {
        std::lock_guard lock(mutex_);
        
        auto it = tasks_.find(id);
        if (it == tasks_.end()) {
            return std::nullopt;
        }
        
        auto& task = it->second;
        
        if (req.title) task.title = *req.title;
        if (req.description) task.description = *req.description;
        if (req.status) task.status = *req.status;
        if (req.user_id) task.user_id = *req.user_id;
        
        auto now = std::chrono::system_clock::now();
        task.updated_at = std::chrono::duration_cast<std::chrono::seconds>(
            now.time_since_epoch()).count();
        
        task_copy = task;
    }
    // Broadcast outside the lock
    broadcast_updated(task_copy);
    
    return task_copy;
}

bool TaskService::remove(int64_t id) {
    {
        std::lock_guard lock(mutex_);
        
        auto it = tasks_.find(id);
        if (it == tasks_.end()) {
            return false;
        }
        
        tasks_.erase(it);
    }
    // Broadcast outside the lock
    broadcast_deleted(id);
    
    return true;
}

TaskService::Stats TaskService::get_stats() {
    std::lock_guard lock(mutex_);
    
    Stats stats;
    stats.total = static_cast<int>(tasks_.size());
    
    for (const auto& [id, task] : tasks_) {
        switch (task.status) {
            case models::TaskStatus::Pending: stats.pending++; break;
            case models::TaskStatus::InProgress: stats.in_progress++; break;
            case models::TaskStatus::Completed: stats.completed++; break;
        }
    }
    
    return stats;
}

void TaskService::broadcast_created(const models::Task& task) {
    nlohmann::json msg;
    msg["type"] = "task_created";
    msg["data"] = task.to_json();
    auto json_str = msg.dump();
    std::cout << "[Broadcast] task_created: " << json_str.substr(0, 100) << "...\n";
    hub_.broadcast(json_str);
}

void TaskService::broadcast_updated(const models::Task& task) {
    nlohmann::json msg;
    msg["type"] = "task_updated";
    msg["data"] = task.to_json();
    auto json_str = msg.dump();
    std::cout << "[Broadcast] task_updated: " << json_str.substr(0, 100) << "...\n";
    hub_.broadcast(json_str);
}

void TaskService::broadcast_deleted(int64_t id) {
    nlohmann::json msg;
    msg["type"] = "task_deleted";
    msg["data"]["id"] = id;
    hub_.broadcast(msg.dump());
}

} // namespace project::services
