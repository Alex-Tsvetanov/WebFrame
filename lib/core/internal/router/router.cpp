#include "router.hpp"

namespace webframe::core {
	template <typename Ret, typename... Ts>
	router& router::route(const std::string& path, std::function<Ret(Ts...)> const& res) {
		if (routes.find(path) == routes.end())
			routes[path] = responser(res);
		else  // rewriting path
			routes[path] = responser(res);
		return *this;
	}

	template <typename F>
	router& router::route(const std::string& path, F _res) {
		const auto res = wrap(_res);
		if (routes.find(path) == routes.end())
			routes[path] = utils::responser(res);
		else  // rewriting path
			routes[path] = utils::responser(res);
		return *this;
	}
}  // namespace webframe::core