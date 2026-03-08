#pragma once

#include "coroute/core/app.hpp"

namespace flutter_project::services
{
	class TaskService;
}

namespace flutter_project::handlers::api::tasks
{

	void register_routes(coroute::App& app, services::TaskService& task_service);

}  // namespace flutter_project::handlers::api::tasks
