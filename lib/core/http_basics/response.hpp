#pragma once
#include "../core.hpp"
#include <map>
#include <string>

namespace webframe::core
{
	/**
		*  @brief   Type of the response
		*  @details This type represents the status line, all headers, and the body of the response
		***********************************************/
	class response
	{
	private:
		status_line status;
		std::map < std::string, std::string > header;
		std::string body;
		std::string output;
	public:
		explicit response(const std::string& html) :
			response(status_line("1.1", "200"), { {"Content-type", "text/html"} }, html)
		{
			rebuild_string();
		}
		response(const std::string& http, const std::string& html) :
			response(status_line(http, "200"), { {"Content-type", "text/html"} }, html)
		{
			rebuild_string();
		}
		response(const status_line& status, const std::string& html) :
			response(status, { {"Content-type", "text/html"} }, html)
		{
			rebuild_string();
		}
		response(const status_line& s = status_line("2.0", "204"), const std::map < std::string, std::string >& m = {}, const std::string& _body = "") :
			status(s), header(m), body(_body)
		{
			rebuild_string();
		}
		response(const std::string& http, const response& r) :
			status(http, r.status.code), header(r.header), body(r.body)
		{
			rebuild_string();
		}
	private:
		void rebuild_string()
		{
			output.clear();
			output += status.to_string();
			output += std::accumulate(std::begin(header), std::end(header), std::string(""),
				[](const std::string& all, const std::pair<std::string, std::string>& x) -> std::string {
					return all + x.first + ": " + x.second + end_line;
				});
			output += end_line;
			output += body;
		}
	public:
		const std::string& to_string() const
		{
			return output;
		}
	};
}
