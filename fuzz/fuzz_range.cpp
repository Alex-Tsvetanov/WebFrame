// Range header parsing.
//
// A header value taken verbatim from any client and turned into offsets into a file.
// The historical failures on this surface are not parse crashes but arithmetic: a
// range that normalises to a negative length, an end before a start, or a count of
// ranges large enough to make the response amplify the request. Apache's CVE-2011-3192
// was the last of those.
//
// So the harness checks the numbers as well as the absence of a crash, both before and
// after normalising against a file size.

#include <coroute/core/range.hpp>

#include <cstddef>
#include <cstdint>
#include <string_view>

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size)
{
	const std::string_view header(reinterpret_cast<const char*>(data), size);

	auto parsed = coroute::range::parse(header);
	if (!parsed)
	{
		return 0;
	}

	for (auto range : parsed->ranges)
	{
		// A parsed range may be open at either end, but a bound that is present must be
		// a position in a file, and a file has no negative positions.
		if (range.start && *range.start < 0)
		{
			__builtin_trap();
		}
		if (range.end && *range.end < 0)
		{
			__builtin_trap();
		}
		// Both ends present and inverted is unsatisfiable and must not survive parsing
		// as though it were a range, because length() would then be negative.
		if (range.start && range.end && *range.start > *range.end)
		{
			__builtin_trap();
		}

		// Normalised against a file size, a range that reports itself satisfiable has
		// to be inside the file and have a positive length. This is the calculation a
		// static file handler feeds straight into a read.
		constexpr std::int64_t total = 4096;
		if (range.normalize(total))
		{
			if (range.get_start() < 0 || range.get_end() >= total)
			{
				__builtin_trap();
			}
			if (range.length() <= 0)
			{
				__builtin_trap();
			}
		}
	}

	return 0;
}
