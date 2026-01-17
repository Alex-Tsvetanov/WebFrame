#pragma once

#include <functional>
#include <string>
#include <string_view>
#include <vector>

#include "coroute/coro/task.hpp"

namespace coroute {

// ============================================================================
// ViewExecutionContext - Minimal, platform-agnostic
// ============================================================================

/// Minimal execution context for views.
/// Contains ONLY framework-owned metadata, no user-defined types.
/// This is intentionally minimal—views should fetch data via app.fetch().
struct ViewExecutionContext {
  /// Route pattern that matched (e.g., "/user/{id}").
  std::string_view route;

  /// View/template name being rendered.
  std::string_view view_name;
};

// ============================================================================
// View Middleware Types (Limited Scope)
// ============================================================================

/// View middleware signature.
/// Use for UI-level concerns ONLY:
///   - Access gating (not auth - that's in API routes)
///   - ViewModel enrichment from global UI state
///   - Pre-flight orchestration (sync, cache warmup)
///   - View-level error normalization
///
/// NOT for:
///   - Authentication (use API routes + app.fetch())
///   - Sessions (use HTTP middleware)
///   - Cookies/headers (use HTTP middleware)
///   - Redirects (return appropriate Response)
///   - Business logic (belongs in API routes)
using ViewMiddleware = std::function<Task<void>(ViewExecutionContext &)>;

// ============================================================================
// ViewMiddlewareChain - Execute middleware in order
// ============================================================================

class ViewMiddlewareChain {
public:
  void add(ViewMiddleware mw) { middleware_.push_back(std::move(mw)); }

  bool empty() const noexcept { return middleware_.empty(); }
  size_t size() const noexcept { return middleware_.size(); }

  /// Execute all middleware in registration order.
  Task<void> execute(ViewExecutionContext &ctx) const {
    for (const auto &mw : middleware_) {
      co_await mw(ctx);
    }
  }

private:
  std::vector<ViewMiddleware> middleware_;
};

} // namespace coroute
