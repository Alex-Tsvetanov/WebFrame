#include "path_variables.hpp"

namespace webframe::core {
  path_vars::var::var() : value(""), type("string") {}
  path_vars::var::var(const std::string &_value) : value(_value), type("string") {}
  path_vars::var::var(const std::string &_value, const std::string &_type)
      : value(_value), type(_type) {}
      
  path_vars::var::var(const std::pair<std::string, std::string>& details)
      : value(details.first), type(details.second) {}

  const std::string &path_vars::var::get() const { return value; }

  path_vars::var::operator int() const {
    if (value.size() == 0)
      throw std::invalid_argument("path_vars::var::value is empty.");
    int ans = 0;
    if (value[0] == '-')
      ans = -(value[1] - '0');
    for (size_t i = (value[0] == '-'); i < value.size(); i++)
      if (value[i] >= '0' && value[i] <= '9')
        ans = ans * 10 + value[i] - '0';
      else
        throw std::invalid_argument("path_vars::var::value is not matching "
                                    "path_vars::var::type (not integer)");
    return ans;
  }

  path_vars::var::operator long long() const {
    if (value.size() == 0)
      throw std::invalid_argument("path_vars::var::value is empty.");
    int ans = 0;
    if (value[0] == '-')
      ans = -(value[1] - '0');
    for (size_t i = (value[0] == '-'); i < value.size(); i++)
      if (value[i] >= '0' && value[i] <= '9')
        ans = ans * 10 + value[i] - '0';
      else
        throw std::invalid_argument("path_vars::var::value is not matching "
                                    "path_vars::var::type (not integer)");
    return ans;
  }
  
  path_vars::var::operator const char *() const {
    if (value.size() == 0)
      throw std::invalid_argument("path_vars::var::value is empty.");
    return value.c_str();
  }
  
  path_vars::var::operator char() const {
    if (value.size() == 0)
      throw std::invalid_argument("path_vars::var::value is empty.");
    if (value.size() != 1)
      throw std::invalid_argument("path_vars::var::value is too long.");
    return value[0];
  }

  path_vars::var::operator std::string() const {
    if (value.size() == 0)
      throw std::invalid_argument("path_vars::var::value is empty.");
    return value;
  }
  
  path_vars::var::operator const std::string &() const {
    if (value.size() == 0)
      throw std::invalid_argument("path_vars::var::value is empty.");
    return value;
  }
  
  template <typename T> path_vars::var::operator T &() const {
    return T(value);
  }
  
  template <typename T> path_vars::var::operator T() const {
    return T(value);
  }
  
  path_vars::path_vars() {}
  path_vars::path_vars(std::initializer_list<path_vars::var> l) : vars(l) {}

  const path_vars::var path_vars::operator[](long long unsigned int ind) const { return vars[ind]; }
  
  path_vars &path_vars::operator+=(const var &v) {
    vars.push_back(v);
    return *this;
  }
  
  size_t path_vars::size() const { return vars.size(); }

} // namespace webframe::core