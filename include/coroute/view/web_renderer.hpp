#pragma once

#include <filesystem>
#include <sstream>
#include <stdexcept>


#include "coroute/view/view_renderer.hpp"

#ifdef COROUTE_HAS_TEMPLATES
#include <inja/inja.hpp>
#endif

namespace coroute {

// Forward declaration
class App;

// ============================================================================
// WebViewRenderer - Inja-based renderer for web platform
// ============================================================================

#ifdef COROUTE_HAS_TEMPLATES

/// Web view renderer using inja template engine.
/// Renders ViewModels to HTML responses.
class WebViewRenderer : public IViewRenderer {
  App &app_;

public:
  explicit WebViewRenderer(App &app) : app_(app) {}

  /// Validate all templates exist in the template directory.
  /// Throws with detailed error listing missing templates.
  void validate_templates(const std::vector<ViewTemplates> &templates) override;

  /// Render a template with the given view result to an HTML response.
  Task<Response> render(std::string_view template_id,
                        const ViewResultAny &view_result) override;
};

#else

/// Stub renderer when templates are not available.
class WebViewRenderer : public IViewRenderer {
public:
  explicit WebViewRenderer(App & /*app*/) {}

  void validate_templates(
      const std::vector<ViewTemplates> & /*templates*/) override {
    // No templates to validate
  }

  Task<Response> render(std::string_view /*template_id*/,
                        const ViewResultAny & /*view_result*/
                        ) override {
    co_return Response::internal_error(
        "Templates not enabled. Build with COROUTE_HAS_TEMPLATES");
  }
};

#endif // COROUTE_HAS_TEMPLATES

} // namespace coroute
