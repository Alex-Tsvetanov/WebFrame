#include "status_line.hpp"

namespace webframe::core {

	void status_line::rebuild_string() {
		output = "HTTP/" + this->http + " " + this->code + " " +
		         std::string(http_codes::get_reason_by_code(this->code.c_str())) + end_line.data();
	}

	status_line::status_line(const std::string& _code) : http("1.1"), code(_code) { rebuild_string(); }

	status_line::status_line(const std::string& _http, const std::string& _code) : http(_http), code(_code) {
		rebuild_string();
	}

	const std::string& status_line::to_string() const { return output; }

}  // namespace webframe::core
