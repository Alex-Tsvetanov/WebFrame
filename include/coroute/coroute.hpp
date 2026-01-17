#pragma once

// Main include file for Coroute v2

// Utilities
#include "coroute/util/expected.hpp"
#include "coroute/util/from_string.hpp"

// Core
#include "coroute/core/app.hpp"
#include "coroute/core/auth_state.hpp"
#include "coroute/core/chunked.hpp"
#include "coroute/core/compression.hpp"
#include "coroute/core/cookie.hpp"
#include "coroute/core/error.hpp"
#include "coroute/core/form.hpp"
#include "coroute/core/json.hpp"
#include "coroute/core/logging.hpp"
#include "coroute/core/metrics.hpp"
#include "coroute/core/range.hpp"
#include "coroute/core/request.hpp"
#include "coroute/core/response.hpp"
#include "coroute/core/router.hpp"
#include "coroute/core/session.hpp"
#include "coroute/core/static_files.hpp"

// Coroutines
#include "coroute/coro/cancellation.hpp"
#include "coroute/coro/task.hpp"

// Network
#include "coroute/net/connection_pool.hpp"
#include "coroute/net/io_context.hpp"
#include "coroute/net/websocket.hpp"

#ifdef COROUTE_HAS_TLS
#include "coroute/net/tls.hpp"
#endif

// Utilities
#include "coroute/util/object_pool.hpp"
#include "coroute/util/zero_copy.hpp"

// View system
#ifdef COROUTE_HAS_TEMPLATES
#include "coroute/view/view_renderer.hpp"
#include "coroute/view/view_types.hpp"
#include "coroute/view/web_renderer.hpp"

#endif

namespace coroute {

// Version info
constexpr int VERSION_MAJOR = 2;
constexpr int VERSION_MINOR = 0;
constexpr int VERSION_PATCH = 0;
constexpr const char *VERSION_STRING = "2.0.0";

} // namespace coroute
