#pragma once

#include "coroute/core/app.hpp"

namespace flutter_project::services
{
	class UserService;
}

namespace flutter_project::handlers::api::users
{

	void register_routes(coroute::App& app, services::UserService& user_service);

}  // namespace flutter_project::handlers::api::users
