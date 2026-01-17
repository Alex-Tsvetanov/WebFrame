#include "coroute/core/router.hpp"
#include <sstream>

namespace coroute {

// ============================================================================
// Router Implementation - DFA-based matching via url-matcher
// ============================================================================

matcher::RegexMatcher<size_t> &Router::get_matcher_for(HttpMethod method) {
  switch (method) {
  case HttpMethod::GET:
    return get_matcher_;
  case HttpMethod::POST:
    return post_matcher_;
  case HttpMethod::PUT:
    return put_matcher_;
  case HttpMethod::DELETE:
    return delete_matcher_;
  case HttpMethod::PATCH:
    return patch_matcher_;
  case HttpMethod::HEAD:
    return head_matcher_;
  case HttpMethod::OPTIONS:
    return options_matcher_;
  default:
    return get_matcher_;
  }
}

const matcher::RegexMatcher<size_t> &
Router::get_matcher_for(HttpMethod method) const {
  switch (method) {
  case HttpMethod::GET:
    return get_matcher_;
  case HttpMethod::POST:
    return post_matcher_;
  case HttpMethod::PUT:
    return put_matcher_;
  case HttpMethod::DELETE:
    return delete_matcher_;
  case HttpMethod::PATCH:
    return patch_matcher_;
  case HttpMethod::HEAD:
    return head_matcher_;
  case HttpMethod::OPTIONS:
    return options_matcher_;
  default:
    return get_matcher_;
  }
}

std::pair<std::string, std::vector<std::string>>
Router::convert_pattern(std::string_view pattern) {
  // Convert "/user/{id}/post/{pid}" to url-matcher format
  // url-matcher doesn't support [^/] negation, so we use a positive character
  // class that matches typical URL path segment characters Note: The character
  // class content (without surrounding brackets) for use in capture groups
  static const std::string PATH_SEGMENT_CLASS = "A-Za-z0-9_.%\\-";
  static const std::string PATH_ANY_CLASS = "A-Za-z0-9_.%\\-\\/";

  std::vector<std::string> param_names;
  std::ostringstream matcher_pattern;

  size_t i = 0;
  while (i < pattern.size()) {
    char c = pattern[i];

    if (c == '{') {
      // Find closing brace
      size_t end = pattern.find('}', i);
      if (end == std::string_view::npos) {
        // Malformed pattern, treat { as literal
        matcher_pattern << "\\{";
        ++i;
        continue;
      }

      // Extract parameter name
      std::string_view param_name = pattern.substr(i + 1, end - i - 1);
      param_names.emplace_back(param_name);

      // Add capturing group for path segment: ([A-Za-z0-9_.%\-]+)
      matcher_pattern << "([" << PATH_SEGMENT_CLASS << "]+)";

      i = end + 1;
    } else if (c == '*') {
      // Wildcard - match anything (greedy)
      if (i + 1 < pattern.size() && pattern[i + 1] == '*') {
        // ** = match across path segments
        matcher_pattern << "([" << PATH_ANY_CLASS << "]+)";
        i += 2;
      } else {
        // * = match within path segment
        matcher_pattern << "([" << PATH_SEGMENT_CLASS << "]*)";
        ++i;
      }
    } else if (c == '/') {
      // Escape forward slash for url-matcher
      matcher_pattern << "\\/";
      ++i;
    } else if (c == '.' || c == '+' || c == '?' || c == '(' || c == ')' ||
               c == '[' || c == ']' || c == '^' || c == '$' || c == '|' ||
               c == '\\') {
      // Escape regex special characters
      matcher_pattern << '\\' << c;
      ++i;
    } else {
      matcher_pattern << c;
      ++i;
    }
  }

  return {matcher_pattern.str(), std::move(param_names)};
}

void Router::add(HttpMethod method, std::string pattern, Handler handler) {
  // Convert pattern to url-matcher format
  auto [matcher_pattern, param_names] = convert_pattern(pattern);

  // Store route info
  size_t route_id = routes_.size();
  routes_.push_back(RouteInfo{.pattern = std::move(pattern),
                              .handler = std::move(handler),
                              .method = method,
                              .param_names = std::move(param_names)});

  // Add to the appropriate matcher
  get_matcher_for(method).add_regex(matcher_pattern, route_id);
}

Router::MatchResult Router::match(HttpMethod method, std::string_view path) {
  MatchResult result;

  // Use url-matcher's DFA-based matching with group capture
  auto &matcher = get_matcher_for(method);
  auto matches = matcher.match_with_groups(std::string(path));

  if (matches.empty()) {
    return result;
  }

  // Take the last match (most specific, as per url-matcher convention)
  const auto &match = matches.back();
  size_t route_id = match.regex_id;

  if (route_id >= routes_.size()) {
    return result;
  }

  const auto &route = routes_[route_id];
  result.handler = &route.handler;

  // Extract captured groups as strings
  // url-matcher groups are 0-indexed
  if (!match.groups.empty()) {
    size_t max_group = 0;
    for (const auto &[group_id, positions] : match.groups) {
      if (group_id + 1 > max_group)
        max_group = group_id + 1;
    }

    result.params.resize(max_group);
    for (const auto &[group_id, positions] : match.groups) {
      if (positions.first <= positions.second &&
          positions.second <= path.size()) {
        result.params[group_id] = std::string(
            path.substr(positions.first, positions.second - positions.first));
      }
    }
  }

  return result;
}

// ============================================================================
// View Route Implementation
// ============================================================================

void Router::add_view(std::string pattern, ViewHandler handler) {
  // Convert pattern to url-matcher format
  auto [matcher_pattern, param_names] = convert_pattern(pattern);

  // Store view route info
  size_t route_id = view_routes_.size();
  view_routes_.push_back(ViewRouteInfo{
      .pattern = std::move(pattern),
      .handler = std::move(handler),
      .param_names = std::move(param_names),
      .templates = nullptr // Will be set by App if needed for validation
  });

  // Add to view matcher
  view_matcher_.add_regex(matcher_pattern, route_id);
}

Router::ViewMatchResult Router::match_view(std::string_view path) {
  ViewMatchResult result;

  // Use url-matcher's DFA-based matching with group capture
  auto matches = view_matcher_.match_with_groups(std::string(path));

  if (matches.empty()) {
    return result;
  }

  // Take the last match (most specific)
  const auto &match = matches.back();
  size_t route_id = match.regex_id;

  if (route_id >= view_routes_.size()) {
    return result;
  }

  const auto &route = view_routes_[route_id];
  result.handler = &route.handler;

  // Extract captured groups as strings
  if (!match.groups.empty()) {
    size_t max_group = 0;
    for (const auto &[group_id, positions] : match.groups) {
      if (group_id + 1 > max_group)
        max_group = group_id + 1;
    }

    result.params.resize(max_group);
    for (const auto &[group_id, positions] : match.groups) {
      if (positions.first <= positions.second &&
          positions.second <= path.size()) {
        result.params[group_id] = std::string(
            path.substr(positions.first, positions.second - positions.first));
      }
    }
  }

  return result;
}

} // namespace coroute
