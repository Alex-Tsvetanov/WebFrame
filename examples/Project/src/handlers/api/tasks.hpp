#pragma once

#include "coroute/core/app.hpp"

namespace project::services {
    class TaskService;
}

namespace project::handlers::api::tasks {

void register_routes(coroute::App& app, services::TaskService& task_service);

} // namespace project::handlers::api::tasks
