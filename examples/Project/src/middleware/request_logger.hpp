#pragma once

#include "coroute/core/app.hpp"

namespace project::middleware {

// Creates a middleware that logs all requests
coroute::Middleware request_logger();

} // namespace project::middleware
