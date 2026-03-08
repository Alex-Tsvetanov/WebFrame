#pragma once

#include "coroute/core/app.hpp"

namespace flutter_project::services
{
	class UserService;
	class TaskService;
}  // namespace flutter_project::services

namespace flutter_project::handlers::pages
{

	void register_routes(coroute::App& app,
	                     services::UserService& user_service,
	                     services::TaskService& task_service);

}  // namespace flutter_project::handlers::pages
