#pragma once

#include <any>
#include <functional>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>

#include <nlohmann/json.hpp>

#include "coroute/coro/task.hpp"

namespace coroute {

// ============================================================================
// ViewTemplates - Platform-specific template identifiers
// ============================================================================

struct ViewTemplates {
  std::string web;
  std::string mobile;
  std::string desktop;

  /// Construct with same template for all platforms
  explicit ViewTemplates(std::string all)
      : web(all), mobile(all), desktop(std::move(all)) {}

  /// Construct with platform-specific templates
  ViewTemplates(std::string w, std::string m, std::string d)
      : web(std::move(w)), mobile(std::move(m)), desktop(std::move(d)) {}
};

// ============================================================================
// ViewResult<VM> - Typed view result
// ============================================================================

template <typename VM> struct ViewResult {
  ViewTemplates templates;
  VM model;
};

// ============================================================================
// ViewResultAny - Type-erased view result for routing storage
// ============================================================================

/// Type-erased view result using std::any and a type-erased JSON conversion
/// function
struct ViewResultAny {
  ViewTemplates templates;
  std::any model;
  std::function<nlohmann::json(const std::any &)> to_json_fn;

  /// Create from a typed ViewResult using templated constructor
  template <typename VM>
  ViewResultAny(ViewResult<VM> result)
      : templates(std::move(result.templates)), model(std::move(result.model)),
        to_json_fn([](const std::any &m) -> nlohmann::json {
          // Use nlohmann's automatic conversion which handles ADL properly
          return nlohmann::json(std::any_cast<const VM &>(m));
        }) {}

  /// Convert the model to JSON
  [[nodiscard]] nlohmann::json to_json() const { return to_json_fn(model); }
};

// ============================================================================
// View<VM> - Coroutine type alias for view handlers
// ============================================================================

/// View<VM> is an alias for Task<ViewResult<VM>>.
/// Handlers returning View<VM> must co_return a ViewResult<VM>.
template <typename VM> using View = Task<ViewResult<VM>>;

} // namespace coroute
