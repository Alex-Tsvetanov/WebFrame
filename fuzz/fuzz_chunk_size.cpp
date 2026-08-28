// Chunked transfer-encoding chunk-size lines.
//
// The line before every chunk of a chunked request body, taken verbatim from the
// client and turned into a length to read. Historically this is where request
// smuggling lives: two parties disagreeing about how many octets the next chunk is,
// or one of them computing a length that is not the number it looks like.
//
// The property under test is that the result is either a rejection or a length the
// caller can act on. A negative size, or one that overflowed on the way, is neither.

#include <coroute/core/chunked.hpp>

#include <cstddef>
#include <cstdint>
#include <string_view>

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size)
{
	const std::string_view line(reinterpret_cast<const char*>(data), size);

	const std::int64_t parsed = coroute::chunked::parse_chunk_size(line);

	// -1 is the documented rejection. Anything else is a length, and a length that the
	// caller will hand to read_bytes as a size_t, so it has to be a number of octets
	// this process could actually be asked for.
	if (parsed < -1)
	{
		__builtin_trap();
	}

	// The caller checks only for negative, so a size that survives has to be one the
	// rest of the server can hold. The bound here is the same one app.cpp applies to a
	// Content-Length body: the two framings describe the same quantity and disagreeing
	// about it is the shape request smuggling takes.
	constexpr std::int64_t max_body = 10 * 1024 * 1024;
	if (parsed > max_body)
	{
		__builtin_trap();
	}

	return 0;
}
