#pragma once

#include "coroute/core/app.hpp"

namespace project::services {
    class UserService;
}

namespace project::handlers::pages {

void register_routes(coroute::App& app, services::UserService& user_service);

} // namespace project::handlers::pages
