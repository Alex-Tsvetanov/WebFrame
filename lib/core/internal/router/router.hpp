#pragma once

#include "../../core.hpp"

namespace webframe::core {
	class router {
	private:
  		std::map<std::string, utils::responser> routes;

	public:
		template <typename Ret, typename... Ts>
		router& route(const std::string& path,
		              std::function<Ret(Ts...)> const& res);

		template <typename F>
		router& route(const std::string& path, F _res);

		friend class application;
	};
}

#include "router.cpp"