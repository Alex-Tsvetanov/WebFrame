#pragma once

#include <matcher/core.hpp>
#include <optional>
#include <vector>
#include <map>
#include <string>

namespace webframe::core {
	/**
	 * @brief Result of a regex match with captured groups as strings
	 */
	template <typename RegexData>
	struct MatchResultWithGroups {
		const RegexData* data;  // Pointer to avoid copy issues with non-copyable types
		std::vector<std::string> groups;  // Captured group values (index 0 = group 1, etc.)
	};

	template <typename RegexData>
	class RegexMatcher : private matcher::RegexMatcher<size_t, char> {
		const std::vector<RegexData>* container;

	public:
		RegexMatcher(std::vector<RegexData>* container) : matcher::RegexMatcher<size_t, char>() {
			this->container = container;
		}

		void add_regex(std::string regex, size_t index) {
			matcher::RegexMatcher<size_t, char>::add_regex(regex, index);
		}

		std::optional<RegexData> match(std::string path) {
			std::vector<size_t> matches;
			matches = matcher::RegexMatcher<size_t, char>::match(path);
			if (matches.size() == 0) return std::nullopt;
			return container->operator[](matches.back());
		}

		/**
		 * @brief Match with captured groups extracted as strings
		 * @param path The path to match
		 * @return Optional result containing the matched data and captured group strings
		 */
		std::optional<MatchResultWithGroups<RegexData>> match_with_groups(const std::string& path) {
			auto matches = matcher::RegexMatcher<size_t, char>::match_with_groups(path);
			if (matches.empty()) return std::nullopt;

			// Take the last match (most specific)
			const auto& match = matches.back();
			
			MatchResultWithGroups<RegexData> result;
			result.data = &(container->operator[](match.regex_id));

			// Extract group strings from positions
			// Groups are 1-indexed in the map, convert to 0-indexed vector
			size_t max_group = 0;
			for (const auto& [group_id, positions] : match.groups) {
				if (group_id > max_group) max_group = group_id;
			}
			
			result.groups.resize(max_group);
			for (const auto& [group_id, positions] : match.groups) {
				if (group_id > 0 && positions.first <= positions.second && positions.second <= path.size()) {
					result.groups[group_id - 1] = path.substr(positions.first, positions.second - positions.first);
				}
			}

			return result;
		}
	};
}  // namespace webframe::core
