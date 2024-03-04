/**
 *  @file   codes.hpp
 *  @brief  Utils for all possible HTTP response codes
 *  @author Alex Tsvetanov
 *  @date   2022-03-07
 ***********************************************/

#pragma once

#include "constexpr.hpp"
#include <string_view>
#include <array>

namespace http_codes {
/**
 *  @brief   List of all HTTP response messages (grouped by first digit of the
 *code)
 ***********************************************/
constexpr std::array<std::array<std::string_view, 18>, 6> codes = {{
    /*0xx errors*/ {{}},
    /*1xx errors*/ {{"Continue", "Switching Protocols"}},
    /*2xx errors*/
    {{"OK", "Created", "Accepted", "Non-Authoritative Information",
      "No Content", "Reset Content", "Partial Content"}},
    /*3xx errors*/
    {{"Multiple Choices", "Moved Permanently", "Found", "See Other",
      "Not Modified", "Use Proxy", "Temporary Redirect"}},
    /*4xx errors*/
    {{"Bad Request", "Unauthorized", "Payment Required", "Forbidden",
      "Not Found", "Method Not Allowed", "Not Acceptable",
      "Proxy Authentication Required", "Request Time-out", "Conflict", "Gone",
      "Length Required", "Precondition Failed", "Request Entity Too Large",
      "Request-URI Too Large", "Unsupported Media Type",
      "Requested range not satisfiable", "Expectation Failed"}},
    /*5xx errors*/
    {{"Internal Server Error", "Not Implemented", "Bad Gateway",
      "Service Unavailable", "Gateway Time-out", "HTTP Version not supported"}},
}};

/**
 *  @brief   Transforms HTTP code into HTTP response message
 *  @param   code The HTTP code which's message is requested as integer
 ***********************************************/
constexpr inline std::string_view get_reason_by_code(unsigned int code) {
  return codes[code / 100][code % 100];
}

/**
 *  @brief   Transforms HTTP code to HTTP response message
 *  @param   code_str The HTTP code which's message is requested as a string
 ***********************************************/
constexpr inline std::string_view
get_reason_by_code(const std::string_view &code_str) {
  return get_reason_by_code(webframe::_compile_time::string_to_uint(code_str));
}
} // namespace http_codes
