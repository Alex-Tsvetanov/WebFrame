#pragma once

#include <string>
#include <cstdint>
#include <nlohmann/json.hpp>

namespace flutter_project::viewmodels
{

	struct TaskVm
	{
		int64_t id = 0;
		std::string title;
		std::string description;
		std::string status;
		int64_t created_by = 0;
		int64_t created_at = 0;
		int64_t updated_at = 0;
	};

	inline void to_json(nlohmann::json& j, const TaskVm& vm)
	{
		j = {
			{         "id",          vm.id},
			{      "title",       vm.title},
			{"description", vm.description},
			{     "status",      vm.status},
			{ "created_by",  vm.created_by},
			{ "created_at",  vm.created_at},
			{ "updated_at",  vm.updated_at}
        };
	}

}  // namespace flutter_project::viewmodels
