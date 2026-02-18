#pragma once

#include <string>
#include <string_view>
#include <chrono>
#include <optional>
#include <ctime>

namespace coroute::time
{

	/**
	 * @brief Converts a system clock time point to an HTTP-formatted date string (RFC 1123).
	 * Format: "Wed, 21 Oct 2015 07:28:00 GMT"
	 */
	std::string to_http_date(std::chrono::system_clock::time_point tp);

	/**
	 * @brief Parses an HTTP-formatted date string (RFC 1123) into a system clock time point.
	 * Supports standard RFC 1123 format.
	 */
	std::optional<std::chrono::system_clock::time_point> from_http_date(std::string_view date);

	/**
	 * @brief Thread-safe version of std::gmtime.
	 */
	std::tm gmtime(std::time_t t);

	/**
	 * @brief Thread-safe version of std::localtime.
	 */
	std::tm localtime(std::time_t t);

}  // namespace coroute::time
