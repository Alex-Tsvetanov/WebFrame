#pragma once

#include <string>

#include "../predef.hpp"

namespace webframe::core {
	/**
	 *  @brief   Type of the status line of the response
	 *  @details This type represents the HTTP version and the HTTP code of the
	 *reponse
	 ***********************************************/
	struct status_line {
	public:
	private:
		std::string http;
		std::string code;
		std::string output;

		void rebuild_string();

	public:
		explicit status_line(const std::string& _code);

		status_line(const std::string& _http, const std::string& _code);

		const std::string& to_string() const;

		friend class response;
	};
}  // namespace webframe::core
