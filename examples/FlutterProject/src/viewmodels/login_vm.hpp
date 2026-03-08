#pragma once

#include <string>
#include <nlohmann/json.hpp>

namespace flutter_project::viewmodels
{

	struct LoginVm
	{
		std::string error;
	};

	inline void to_json(nlohmann::json& j, const LoginVm& vm)
	{
		j = {{"error", vm.error}};
	}

}  // namespace flutter_project::viewmodels
