// application/x-www-form-urlencoded parsing, and the percent-decoder under it.
//
// A request body split on & and =, with every part percent-decoded. The decoder is the
// part worth fuzzing: a percent sign at the end of the input, a percent followed by one
// hex digit, or a percent followed by bytes that only look like hex are each a read
// past the end if the bounds check is written after the lookup instead of before it.

#include <coroute/core/form.hpp>

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size)
{
	const std::string_view body(reinterpret_cast<const char*>(data), size);

	// The decoder on its own first, since parse_urlencoded reaches it only through
	// input that already looks like a form.
	const std::string decoded = coroute::form::url_decode(body);

	// Decoding cannot produce more than it consumed: every escape is three octets in
	// and one out, and everything else is one for one. Growth would mean the decoder
	// invented output, which is how a length calculation ends up wrong somewhere else.
	if (decoded.size() > body.size())
	{
		__builtin_trap();
	}

	auto parsed = coroute::form::parse_urlencoded(body);
	if (!parsed)
	{
		return 0;
	}

	std::size_t total = 0;
	for (const auto& field : parsed->fields())
	{
		total += field.name.size() + field.value.size();
	}
	if (total > body.size())
	{
		__builtin_trap();
	}

	return 0;
}
