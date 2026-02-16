#include "coroute/core/time.hpp"
#include <iomanip>
#include <sstream>
#include <ctime>

namespace coroute::time
{

	std::string to_http_date(std::chrono::system_clock::time_point tp)
	{
		std::time_t t = std::chrono::system_clock::to_time_t(tp);
		std::tm* tm = std::gmtime(&t);

		std::ostringstream oss;
		oss << std::put_time(tm, "%a, %d %b %Y %H:%M:%S GMT");
		return oss.str();
	}

	std::optional<std::chrono::system_clock::time_point> from_http_date(std::string_view date)
	{
		std::tm tm = {};
		std::istringstream iss{std::string(date)};

		// RFC 1123: Wed, 21 Oct 2015 07:28:00 GMT
		iss >> std::get_time(&tm, "%a, %d %b %Y %H:%M:%S GMT");

		if (iss.fail())
		{
			// Try other common formats if needed, but RFC 1123 is the standard for modern HTTP
			return std::nullopt;
		}

		std::time_t t =
#ifdef _WIN32
			_mkgmtime(&tm);
#else
			timegm(&tm);
#endif

		if (t == -1)
		{
			return std::nullopt;
		}

		return std::chrono::system_clock::from_time_t(t);
	}

}  // namespace coroute::time
