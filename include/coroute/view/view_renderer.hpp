#pragma once

#include <string_view>
#include <vector>


#include "coroute/core/response.hpp"
#include "coroute/coro/task.hpp"
#include "coroute/view/view_types.hpp"


namespace coroute {

// ============================================================================
// IViewRenderer - Abstract interface for platform-specific renderers
// ============================================================================

/// Interface for view renderers. Each platform (web, desktop, mobile)
/// implements this interface to render views using platform-specific
/// templating engines.
struct IViewRenderer {
  virtual ~IViewRenderer() = default;

  /// Validate that all registered templates exist.
  /// Should be called at startup before accepting requests.
  /// Throws std::runtime_error with detailed message if validation fails.
  virtual void
  validate_templates(const std::vector<ViewTemplates> &templates) = 0;

  /// Render a view to a Response.
  /// For web, returns Response::html().
  /// For desktop/mobile, implementation may differ.
  virtual Task<Response> render(std::string_view template_id,
                                const ViewResultAny &view_result) = 0;
};

} // namespace coroute
