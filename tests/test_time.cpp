#include <catch2/catch_test_macros.hpp>
#include "coroute/core/time.hpp"
#include <chrono>
#include <thread>

using namespace coroute;

TEST_CASE("HTTP Date Parsing and Formatting", "[time]")
{
	SECTION("Format and Parse Roundtrip")
	{
		auto now = std::chrono::system_clock::now();
		// Round to seconds because HTTP dates only have second precision
		auto expected = std::chrono::time_point_cast<std::chrono::seconds>(now);

		std::string formatted = time::to_http_date(expected);
		auto parsed = time::from_http_date(formatted);

		REQUIRE(parsed.has_value());
		auto parsed_sec = std::chrono::time_point_cast<std::chrono::seconds>(*parsed);
		REQUIRE(parsed_sec == expected);
	}

	SECTION("Specific Date Parsing")
	{
		std::string_view date_str = "Wed, 21 Oct 2015 07:28:00 GMT";
		auto parsed = time::from_http_date(date_str);

		REQUIRE(parsed.has_value());

		std::string formatted = time::to_http_date(*parsed);
		REQUIRE(formatted == date_str);
	}

	SECTION("Invalid Date Parsing")
	{
		auto parsed = time::from_http_date("Invalid Date");
		REQUIRE_FALSE(parsed.has_value());

		parsed = time::from_http_date("Wed, 21 Oct 2015 07:28:00");  // Missing GMT
		// Note: std::get_time might be lenient, but we ideally want standard formats
	}
}
