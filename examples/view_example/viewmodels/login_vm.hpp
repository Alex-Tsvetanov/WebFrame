#pragma once

#include <string>
#include <nlohmann/json.hpp>

struct LoginVm {
	std::string error;
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(LoginVm, error)
};
