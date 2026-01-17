#pragma once

#include "coroute/core/app.hpp"

namespace project::middleware {

// Creates a middleware that requires authentication
// Checks for user_id in session, returns 401 if not present
coroute::Middleware require_auth();

// Creates a middleware that optionally loads user info
// Does not reject unauthenticated requests, just sets context
coroute::Middleware load_user();

} // namespace project::middleware
