#include "coroute/view/web_renderer.hpp"
#include "coroute/core/app.hpp"

#include <sstream>
#include <stdexcept>

namespace coroute {

#ifdef COROUTE_HAS_TEMPLATES

void WebViewRenderer::validate_templates(
    const std::vector<ViewTemplates> &templates) {
  std::vector<std::string> missing;
  std::filesystem::path template_dir = app_.template_dir();

  for (const auto &vt : templates) {
    // Check web template (the only one we render in web builds)
    std::string template_file = vt.web;
    if (!template_file.empty()) {
      // Add .html extension if not present
      if (template_file.find('.') == std::string::npos) {
        template_file += ".html";
      }

      auto full_path = template_dir / template_file;
      if (!std::filesystem::exists(full_path)) {
        missing.push_back(template_file + " (resolved: " + full_path.string() +
                          ")");
      }
    }
  }

  if (!missing.empty()) {
    std::ostringstream oss;
    oss << "View template validation failed. Missing templates:\n";
    for (const auto &m : missing) {
      oss << "  - " << m << "\n";
    }
    oss << "Template directory: " << template_dir.string();
    throw std::runtime_error(oss.str());
  }
}

Task<Response> WebViewRenderer::render(std::string_view template_id,
                                       const ViewResultAny &view_result) {
  std::string template_file(template_id);

  // Add .html extension if not present
  if (template_file.find('.') == std::string::npos) {
    template_file += ".html";
  }

  // Convert model to JSON using the stored conversion function
  nlohmann::json data = view_result.to_json();

  // Render using app's template engine
  co_return app_.render_html(template_file, data);
}

#endif // COROUTE_HAS_TEMPLATES

} // namespace coroute
