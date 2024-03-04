#pragma once

#include "status_line.hpp"
#include <map>
#include <string>
#include <numeric>

namespace webframe::core {
/**
 *  @brief   Type of the response
 *  @details This type represents the status line, all headers, and the body of
 *the response
 ***********************************************/
class response {
private:
  status_line status;
  std::map<std::string, std::string> header;
  std::string body;
  std::string output;

public:
  explicit response(const std::string &html);
  response(const std::string &http, const std::string &html);
  response(const status_line &status, const std::string &html);
  response(const status_line &s = status_line("2.0", "204"),
           const std::map<std::string, std::string> &m = {},
           const std::string &_body = "");
  response(const std::string &http, const response &r);

private:
  void rebuild_string();

public:
  const std::string &to_string() const;
};
} // namespace webframe::core
