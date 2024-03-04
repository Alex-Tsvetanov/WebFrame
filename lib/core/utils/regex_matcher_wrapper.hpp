#pragma once

#include <optional>
#include <vector>
#include <matcher/core.hpp>

namespace webframe::core
{
    template<typename RegexData>
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
    };
} // namespace webframe::core
