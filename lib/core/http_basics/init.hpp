#pragma once

#include <map>
#include <string>
#include <string_view>

namespace webframe::core {
const std::string end_line = "\n";

/**
 *  @brief   Helper function for itoa
 *  @details Reverses a string
 ***********************************************/
void reverse(char str[], int length) {
  int start = 0;
  int end = length - 1;
  while (start < end) {
    std::swap(*(str + start), *(str + end));
    start++;
    end--;
  }
}

/**
 *  @brief   Implementation of itoa
 *  @details Converts integer to string
 ***********************************************/
char *itoa(int num, char *str, int base) {
  int i = 0;
  bool isNegative = false;

  /* Handle 0 explicitly, otherwise empty string is printed for 0 */
  if (num == 0) {
    str[i++] = '0';
    str[i] = '\0';
    return str;
  }

  // In standard itoa(), negative numbers are handled only with
  // base 10. Otherwise numbers are considered unsigned.
  if (num < 0 && base == 10) {
    isNegative = true;
    num = -num;
  }

  // Process individual digits
  while (num != 0) {
    int rem = num % base;
    str[i++] = (rem > 9) ? static_cast<char>(rem - 10) + 'a'
                         : static_cast<char>(rem) + '0';
    num = num / base;
  }

  // If number is negative, append '-'
  if (isNegative)
    str[i++] = '-';

  str[i] = '\0'; // Append string terminator

  // Reverse the string
  reverse(str, i);

  return str;
}

/**
 *  @brief   Type of request prameters
 *  @details This is the type of all parameters passed as request paramertes
 *  @note    Ex. In 'url?param=value' param and value will be saved in the map
 *as key and value.
 ***********************************************/
using req_vars = std::map<std::string, std::string>;

/**
 *  @brief    Casts webframe::method to const char*
 *  @see      webframe::method
 ***********************************************/
constexpr const char *method_to_string(const method &m) {
  return (m == method::GET)       ? "GET"
         : (m == method::HEAD)    ? "HEAD"
         : (m == method::POST)    ? "POST"
         : (m == method::PUT)     ? "PUT"
         : (m == method::DDELETE) ? "DELETE"
         : (m == method::CONNECT) ? "CONNECT"
         : (m == method::OPTIONS) ? "OPTIONS"
         : (m == method::TRACE)   ? "TRACE"
         : (m == method::PATCH)
             ? "PATCH"
             : ([]() -> const char * {
                 throw std::invalid_argument("Not valid METHOD Type");
               })();
}

constexpr bool strings_equal(char const *a, char const *b) {
  return std::string_view(a) == b;
}

/**
 *  @brief    Casts const char* to webframe::method
 *  @see      webframe::method
 ***********************************************/
constexpr method string_to_method(const char *m) {
  return (strings_equal(m, "GET"))       ? method::GET
         : (strings_equal(m, "HEAD"))    ? method::HEAD
         : (strings_equal(m, "POST"))    ? method::POST
         : (strings_equal(m, "PUT"))     ? method::PUT
         : (strings_equal(m, "DELETE"))  ? method::DDELETE
         : (strings_equal(m, "CONNECT")) ? method::CONNECT
         : (strings_equal(m, "OPTIONS")) ? method::OPTIONS
         : (strings_equal(m, "TRACE"))   ? method::TRACE
         : (strings_equal(m, "PATCH"))
             ? method::PATCH
             : throw std::invalid_argument(std::string(m) +
                                           " is not a valid METHOD Type");
}
} // namespace webframe::core

#include "path_variables.hpp"
#include "request.hpp"
#include "response.hpp"
#include "status_line.hpp"