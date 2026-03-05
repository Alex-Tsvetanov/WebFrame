#pragma once

#include <string>
#include <nlohmann/json.hpp>

namespace project::viewmodels {

struct DashboardVm {
	std::string title;
	bool authenticated;
	std::string username;
	
	// Add default constructor and parameterized if needed, or just intrusive macro
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(DashboardVm, title, authenticated, username)
};

} // namespace project::viewmodels
