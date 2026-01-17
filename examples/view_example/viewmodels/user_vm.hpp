#pragma once

#include <nlohmann/json.hpp>
#include <string>

struct UserVm {
  std::string name;
  std::string greeting;
  bool logged_in;
};

inline void to_json(nlohmann::json &j, const UserVm &vm) {
  j = {{"name", vm.name},
       {"greeting", vm.greeting},
       {"logged_in", vm.logged_in}};
}