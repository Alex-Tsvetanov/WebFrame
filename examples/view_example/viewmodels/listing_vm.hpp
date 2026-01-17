#pragma once

#include <nlohmann/json.hpp>
#include <string>
#include <vector>


struct ListingVm {
  std::string title;
  std::vector<std::string> items;
};

// ADL-based to_json for ViewModel serialization
inline void to_json(nlohmann::json &j, const ListingVm &vm) {
  j = {{"title", vm.title}, {"items", vm.items}};
}