#pragma once

#include <string>
#include <nlohmann/json.hpp>

namespace project::viewmodels {

struct LoginVm {
	std::string title;
	std::string error;
	
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(LoginVm, title, error)
};

} // namespace project::viewmodels
