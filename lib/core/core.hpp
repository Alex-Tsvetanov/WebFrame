/**
 *  @file   webframe.hpp
 *  @brief  Single header containing all the necessary tools regarding WebFrame
 *  @author Alex Tsvetanov
 *  @date   2022-03-07
 ***********************************************/

#pragma once

#include <chrono>
#include <filesystem>
#include <fstream>
#include <functional>
#include <future>
#include <iomanip>
#include <ios>
#include <iostream>
#include <map>
#include <optional>
#include <regex>
#include <set>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>
#include <type_traits>
#include <utility>

#include "Core-version.hpp"
#include "c/host.h"
#include "utils/regex_matcher_wrapper.hpp"

#ifdef USE_INJA
#include <inja/inja.hpp>
#endif

#include "http/processing/loading_state.hpp"
#include "http/processing/path_variables.hpp"
#include "http/processing/request.hpp"
#include "http/request/method.hpp"
#include "http/response/response.hpp"
#include "http/response/status_line.hpp"
#include "utils/generator.hpp"
#include "utils/lambda2function.hpp"
#include "utils/respond_manager.hpp"
#include "utils/server_status.hpp"
#include "utils/sync_file.hpp"

using namespace std::literals::string_literals;

namespace {
	//           regex    path var types
	std::pair<std::string, std::vector<std::string>> convert_path_to_regex(const std::string& str) {
		static const std::string regexAnyChar = "A-Za-z_%0-9.\\-";
		std::vector<std::string> v;
		std::string format;
		for (size_t i = 0; i < str.size(); i++) {
			if (str[i] == '{') {
				std::string var_type;
				for (i++; i < str.size() && str[i] != '}'; i++) {
					if (str[i] == ':') break;
					var_type += str[i];
				}
				if (var_type == "") var_type = "string";
				if (str[i] == '}') {
					i -= var_type.size() + 1;
					var_type = "string";
				}
				v.push_back(var_type);
				format += "(";
				std::string curr_val_regex;
				for (i++; i < str.size() && str[i] != '}'; i++) {
					if (str[i] == ':') break;
					curr_val_regex += str[i];
				}
				if (curr_val_regex == "string" || curr_val_regex == "text") curr_val_regex = "[" + regexAnyChar + "]+";
				if (curr_val_regex == "char" || curr_val_regex == "symbol") curr_val_regex = "[" + regexAnyChar + "]";
				if (curr_val_regex == "digit") curr_val_regex = "[0-9]";
				if (curr_val_regex == "number") curr_val_regex = "[0-9]+";
				if (curr_val_regex == "path") curr_val_regex = "[" + regexAnyChar + "\\/]+";
				format += curr_val_regex;
				format += ")";
			} else if (str[i] == '/') {
				format += "\\/";
			} else {
				format += str[i];
			}
		}
		return {format, v};
	}

	template <typename T1>
	T1 my_min(T1 a, T1 b) {
		return (((a) < (b)) ? (a) : (b));
	}
}  // namespace

namespace webframe::core {
	class router;
	class application;
}  // namespace webframe::core

#include "internal/application/application.hpp"
#include "internal/router/router.hpp"
#include "internal/threads/threads.hpp"
