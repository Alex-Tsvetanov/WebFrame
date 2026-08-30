// The two routers the DFA router is measured against.
//
// Compiled only when COROUTE_ROUTER_ARMS is on. Neither belongs in a shipped framework:
// one pulls in Crow and its asio dependency, the other pulls in <regex>. They exist so
// that all three arms of the routing experiment come out of one binary, which is the
// only way a difference smaller than build-to-build variation can be resolved at all.
//
// Both are used as their authors intended rather than reimplemented:
//
//   Radix   crow::Trie from CrowCpp/Crow, the compressed prefix tree Crow's own router
//           dispatches with. Named rather than hand-rolled so the comparison is against
//           something a reader can go and read.
//   Regex   std::regex tried in registration order. This is not a library choice; it is
//           what a hand-written router does, and it is in the design because that is
//           what the DFA is usually replacing.
//
// Both do the same work the DFA arm does, including materialising captured parameters
// as strings. An arm that skipped that would win on an operation it never performed.

#include "coroute/core/router.hpp"

#include <array>
#include <regex>
#include <stdexcept>
#include <string>
#include <vector>

#include <crow/routing.h>

// Crow reaches asio, asio reaches windows.h, and windows.h defines DELETE as an access
// mask. HttpMethod::DELETE then stops being an identifier. Undefined here rather than
// renamed in the enum: the enum is public API and this file is the only place in the
// project that pulls windows.h in through a third-party header.
#ifdef DELETE
#undef DELETE
#endif

namespace coroute
{

	namespace
	{
		constexpr size_t kMethodSlots = 8;

		size_t method_slot(HttpMethod method) noexcept
		{
			switch (method)
			{
				case HttpMethod::GET:
					return 0;
				case HttpMethod::POST:
					return 1;
				case HttpMethod::PUT:
					return 2;
				case HttpMethod::DELETE:
					return 3;
				case HttpMethod::PATCH:
					return 4;
				case HttpMethod::HEAD:
					return 5;
				case HttpMethod::OPTIONS:
					return 6;
				default:
					return 0;
			}
		}

		// "/user/{id}" in the framework's syntax becomes "/user/<str>" in Crow's.
		// Only the brace form is translated; the framework's "*" wildcards become
		// Crow's "<path>", which is the same "match across segments" meaning.
		std::string to_crow_pattern(std::string_view pattern)
		{
			std::string out;
			out.reserve(pattern.size() + 8);
			for (size_t i = 0; i < pattern.size();)
			{
				if (pattern[i] == '{')
				{
					const size_t end = pattern.find('}', i);
					if (end == std::string_view::npos)
					{
						out += pattern[i++];
						continue;
					}
					out += "<str>";
					i = end + 1;
				}
				else if (pattern[i] == '*')
				{
					out += "<path>";
					i += (i + 1 < pattern.size() && pattern[i + 1] == '*') ? 2 : 1;
				}
				else
				{
					out += pattern[i++];
				}
			}
			return out;
		}

		// The same pattern as an anchored std::regex. Character class matches the one
		// Router::convert_pattern hands the DFA, so the three arms accept the same set
		// of paths rather than three subtly different ones.
		std::string to_regex_pattern(std::string_view pattern)
		{
			static constexpr std::string_view kSegment = R"([A-Za-z0-9_.%\-]+)";
			static constexpr std::string_view kAny = R"([A-Za-z0-9_.%\-/]+)";

			std::string out = "^";
			for (size_t i = 0; i < pattern.size();)
			{
				const char c = pattern[i];
				if (c == '{')
				{
					const size_t end = pattern.find('}', i);
					if (end == std::string_view::npos)
					{
						out += R"(\{)";
						++i;
						continue;
					}
					out += '(';
					out += kSegment;
					out += ')';
					i = end + 1;
				}
				else if (c == '*')
				{
					out += '(';
					out += (i + 1 < pattern.size() && pattern[i + 1] == '*') ? kAny : kSegment;
					out += ')';
					i += (i + 1 < pattern.size() && pattern[i + 1] == '*') ? 2 : 1;
				}
				else
				{
					if (std::string_view(R"(.+?()[]^$|\/)").find(c) != std::string_view::npos)
					{
						out += '\\';
					}
					out += c;
					++i;
				}
			}
			out += '$';
			return out;
		}
	}  // namespace

	struct RouterArms
	{
		// Crow keys rule 0 as "no match", so route ids are stored one higher and
		// unwound on the way out.
		std::array<crow::Trie, kMethodSlots> radix{};
		std::array<std::vector<std::pair<std::regex, size_t>>, kMethodSlots> regexes{};
		bool radix_optimised = false;

		void add(RouterBackend backend, HttpMethod method, std::string_view pattern, size_t route_id)
		{
			const size_t slot = method_slot(method);
			if (backend == RouterBackend::Radix)
			{
				radix[slot].add(to_crow_pattern(pattern), route_id + 1);
				radix_optimised = false;
			}
			else
			{
				regexes[slot].emplace_back(std::regex(to_regex_pattern(pattern), std::regex::ECMAScript |
				                                                                     std::regex::optimize),
				                           route_id);
			}
		}

		// Crow compresses the tree in Router::validate rather than in add, so the
		// equivalent has to happen here before the first lookup. Doing it lazily on
		// first match would put a one-off cost inside a measured request.
		void finalise(RouterBackend backend)
		{
			if (backend != RouterBackend::Radix || radix_optimised) return;
			for (auto& trie : radix) trie.optimize();
			radix_optimised = true;
		}

		// Returns the route id, or npos.
		size_t match(RouterBackend backend, HttpMethod method, std::string_view path,
		             std::vector<std::string>& params) const
		{
			const size_t slot = method_slot(method);
			if (backend == RouterBackend::Radix)
			{
				// Crow's Trie::find takes a std::string. The copy is Crow's interface
				// and is left in place rather than worked around, because the arm is
				// meant to be the library as a reader would use it.
				const auto found = radix[slot].find(std::string(path));
				if (found.rule_index == 0) return static_cast<size_t>(-1);
				params = found.r_params.string_params;
				return static_cast<size_t>(found.rule_index) - 1;
			}

			std::cmatch m;
			for (const auto& [re, id] : regexes[slot])
			{
				if (std::regex_match(path.data(), path.data() + path.size(), m, re))
				{
					params.clear();
					params.reserve(m.size() > 0 ? m.size() - 1 : 0);
					for (size_t g = 1; g < m.size(); ++g) params.emplace_back(m[g].first, m[g].second);
					return id;
				}
			}
			return static_cast<size_t>(-1);
		}
	};

	std::shared_ptr<RouterArms> make_router_arms() { return std::make_shared<RouterArms>(); }

	void router_arms_add(RouterArms& arms, RouterBackend backend, HttpMethod method, std::string_view pattern,
	                     size_t route_id)
	{
		arms.add(backend, method, pattern, route_id);
	}

	void router_arms_finalise(RouterArms& arms, RouterBackend backend) { arms.finalise(backend); }

	size_t router_arms_match(const RouterArms& arms, RouterBackend backend, HttpMethod method, std::string_view path,
	                         std::vector<std::string>& params)
	{
		return arms.match(backend, method, path, params);
	}

}  // namespace coroute
