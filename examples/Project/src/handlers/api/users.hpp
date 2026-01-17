#pragma once

#include "coroute/core/app.hpp"

namespace project::services {
    class UserService;
}

namespace project::handlers::api::users {

void register_routes(coroute::App& app, services::UserService& user_service);

} // namespace project::handlers::api::users
