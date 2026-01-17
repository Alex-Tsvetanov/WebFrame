#pragma once

#include "models/task.hpp"
#include <vector>
#include <optional>
#include <unordered_map>
#include <mutex>

// Forward declaration
namespace project::handlers::websocket {
    class TaskHub;
}

namespace project::services {

class TaskService {
public:
    explicit TaskService(handlers::websocket::TaskHub& hub);
    
    // CRUD operations
    models::Task create(const models::CreateTaskRequest& req, int64_t created_by);
    std::optional<models::Task> find(int64_t id);
    std::vector<models::Task> list(const models::TaskFilter& filter = {});
    std::optional<models::Task> update(int64_t id, const models::UpdateTaskRequest& req);
    bool remove(int64_t id);
    
    // Statistics
    struct Stats {
        int total = 0;
        int pending = 0;
        int in_progress = 0;
        int completed = 0;
    };
    Stats get_stats();

private:
    void broadcast_created(const models::Task& task);
    void broadcast_updated(const models::Task& task);
    void broadcast_deleted(int64_t id);
    
    // In-memory storage (replace with database in production)
    std::unordered_map<int64_t, models::Task> tasks_;
    int64_t next_id_ = 1;
    mutable std::mutex mutex_;
    
    handlers::websocket::TaskHub& hub_;
};

} // namespace project::services
