// multipart/form-data parsing.
//
// A body and a boundary, both taken from the request, walked by hand looking for
// delimiters. The failures on this surface are historically about the boundary rather
// than the body: an empty boundary, a boundary that appears inside a part, a final
// delimiter that never arrives, a boundary longer than the body. Each of those is a
// search that either runs off the end or never terminates.
//
// The first octet of the input picks the boundary length, so the fuzzer can vary the
// boundary and the body independently instead of having to guess a matching pair.

#include <coroute/core/form.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string_view>

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size)
{
	if (size < 2)
	{
		return 0;
	}

	const std::size_t boundary_len = std::min<std::size_t>(data[0], size - 1);
	const std::string_view boundary(reinterpret_cast<const char*>(data) + 1, boundary_len);
	const std::string_view body(reinterpret_cast<const char*>(data) + 1 + boundary_len,
	                            size - 1 - boundary_len);

	auto parsed = coroute::form::parse_multipart(body, boundary);
	if (!parsed)
	{
		return 0;
	}

	// Every field that came out has to be a view of something. The parser builds
	// strings, so the check that matters is that it did not report a field whose
	// content it never actually saw: the sum of what it returns cannot exceed what it
	// was given.
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
