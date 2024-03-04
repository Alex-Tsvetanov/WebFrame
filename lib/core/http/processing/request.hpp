/**
 *  @file   base.hpp
 *  @brief  Basic utils to handle web requests and reponses
 *  @author Alex Tsvetanov
 *  @date   2022-03-07
 ***********************************************/

#pragma once

#include "../predef.hpp"
#include "loading_state.hpp"
#include "../request/method.hpp"
#include <iostream>
#include <map>
#include <numeric>
#include <regex>
#include <stdexcept>
#include <string>
#include <utility>

namespace webframe::core {
/**
 *  @brief   Type of the request
 *  @details This type represents the HTTP version, the URL, the URL variables,
 *all headers, and the body of the request
 *  @see     webframe::path_vars
 ***********************************************/
class request {
private:
  LoadingState loading;
  std::string remaining_to_parse;
  
  /**
   *  @brief   Type of request prameters
   *  @details This is the type of all parameters passed as request paramertes
   *  @note    Ex. In 'url?param=value' param and value will be saved in the map
   *as key and value.
  ***********************************************/
  using req_vars = std::map<std::string, std::string>;

public:
  method m;
  std::string uri;
  req_vars request_params;
  std::string http;
  std::map<std::string, std::string> header;
  std::string body;

public:
  request();
  request(method _m, const std::string &h,
          const std::map<std::string, std::string> &m, const std::string &_body);

  LoadingState getState() const;

  LoadingState loadMore(const char *buff, const size_t n);
};
} // namespace webframe::core
