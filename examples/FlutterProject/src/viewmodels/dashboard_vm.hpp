#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <nlohmann/json.hpp>

namespace flutter_project::viewmodels
{

	struct TaskSummary
	{
		int64_t id = 0;
		std::string title;
		std::string description;
		std::string status;
	};

	struct DashboardStats
	{
		int total = 0;
		int pending = 0;
		int in_progress = 0;
		int completed = 0;
	};

	struct DashboardVm
	{
		bool authenticated = false;
		std::string username;
		std::vector<TaskSummary> tasks;
		DashboardStats stats;
	};

	inline void to_json(nlohmann::json& j, const TaskSummary& t)
	{
		j = {
			{        "id",          t.id},
			{     "title",       t.title},
			{"description", t.description},
			{    "status",      t.status}
        };
	}

	inline void to_json(nlohmann::json& j, const DashboardStats& s)
	{
		j = {
			{      "total",       s.total},
			{    "pending",     s.pending},
			{"in_progress", s.in_progress},
			{  "completed",   s.completed}
        };
	}

	inline void to_json(nlohmann::json& j, const DashboardVm& vm)
	{
		j = {
			{"authenticated", vm.authenticated},
			{      "username",       vm.username},
			{         "tasks",          vm.tasks},
			{         "stats",          vm.stats}
        };
	}

}  // namespace flutter_project::viewmodels
