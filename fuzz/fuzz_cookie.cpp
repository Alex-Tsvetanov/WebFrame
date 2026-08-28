// Cookie header parsing.
//
// One header, arbitrary content, split into name and value pairs. Nothing here is
// authenticated, and the result is what session lookup and authorisation read from,
// so a parse that invents a pair or loses one is a correctness problem before it is a
// memory-safety one.

#include <coroute/core/cookie.hpp>

#include <cstddef>
#include <cstdint>
#include <string_view>

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size)
{
	const std::string_view header(reinterpret_cast<const char*>(data), size);

	const auto jar = coroute::CookieJar::parse(header);

	// Whatever came out was carved from the header, so the pairs cannot together be
	// longer than what they were carved from. Growth means the parser produced octets
	// the client never sent.
	std::size_t total = 0;
	for (const auto& [name, value] : jar.all())
	{
		total += name.size() + value.size();
	}
	if (total > header.size())
	{
		__builtin_trap();
	}

	return 0;
}
