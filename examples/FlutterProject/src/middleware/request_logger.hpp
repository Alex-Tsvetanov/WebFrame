#pragma once

#include "coroute/core/app.hpp"

namespace flutter_project::middleware
{

	// Creates a middleware that logs all requests
	coroute::Middleware request_logger();

}  // namespace flutter_project::middleware
