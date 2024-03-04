#include "./method.hpp"

namespace {
	constexpr bool strings_equal(char const *a, char const *b) {
		return std::string_view(a) == b;
	}
}

namespace webframe::core {
	const char* method_to_string(method m) {
		return (m == method::GET)       ? "GET"
				: (m == method::HEAD)    ? "HEAD"
				: (m == method::POST)    ? "POST"
				: (m == method::PUT)     ? "PUT"
				: (m == method::DDELETE) ? "DELETE"
				: (m == method::CONNECT) ? "CONNECT"
				: (m == method::OPTIONS) ? "OPTIONS"
				: (m == method::TRACE)   ? "TRACE"
				: (m == method::PATCH)   ? "PATCH"
				: (throw std::invalid_argument("Not valid METHOD Type"), "Undefined");
	}
	method string_to_method(const char *m) {
		return (strcmp(m, "GET") == 0)       ? method::GET
				: (strcmp(m, "HEAD") == 0)    ? method::HEAD
				: (strcmp(m, "POST") == 0)    ? method::POST
				: (strcmp(m, "PUT") == 0)     ? method::PUT
				: (strcmp(m, "DELETE") == 0)  ? method::DDELETE
				: (strcmp(m, "CONNECT") == 0) ? method::CONNECT
				: (strcmp(m, "OPTIONS") == 0) ? method::OPTIONS
				: (strcmp(m, "TRACE") == 0)   ? method::TRACE
				: (strcmp(m, "PATCH") == 0)   ? method::PATCH
				: (throw std::invalid_argument(std::string(m) +
												" is not a valid METHOD Type"), method::undefined);
	}
}