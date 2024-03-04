#include "response.hpp"

namespace webframe::core {

	response::response(const std::string& html)
	    : response::response(status_line("1.1", "200"), {{"Content-type", "text/html"}}, html) {
		rebuild_string();
	}
	response::response(const std::string& http, const std::string& html)
	    : response(status_line(http, "200"), {{"Content-type", "text/html"}}, html) {
		rebuild_string();
	}
	response::response(const status_line& status, const std::string& html)
	    : response::response(status, {{"Content-type", "text/html"}}, html) {
		rebuild_string();
	}
	response::response(const status_line& s, const std::map<std::string, std::string>& m, const std::string& _body)
	    : status(s), header(m), body(_body) {
		rebuild_string();
	}
	response::response(const std::string& http, const response& r)
	    : status(http, r.status.code), header(r.header), body(r.body) {
		rebuild_string();
	}
	void response::rebuild_string() {
		output.clear();
		output += status.to_string();
		output +=
		    std::accumulate(std::begin(header), std::end(header), std::string(""),
		                    [](const std::string& all, const std::pair<std::string, std::string>& x) -> std::string {
			                    return all + x.first + ": " + x.second + end_line.data();
		                    });
		output += end_line;
		output += body;
	}

	const std::string& response::to_string() const { return output; }
}  // namespace webframe::core
