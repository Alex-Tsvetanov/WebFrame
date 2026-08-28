// HTTP/2 frame header parsing.
//
// Nine octets that every HTTP/2 peer sends before anything else, parsed by hand rather
// than by nghttp2: only HPACK is delegated. So this is the server's own code standing
// between an arbitrary client and the rest of the connection state machine.
//
// The property under test is that parse either succeeds with a header that agrees with
// the octets it came from, or fails. Never reads past the span, never returns a length
// or stream id that the wire format cannot express.

#include <coroute/http2/frame.hpp>

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size)
{
	// Copied into an exactly-sized buffer rather than spanning the caller's, so that a
	// read one octet past the end is a heap overflow ASan reports rather than a read of
	// whatever libFuzzer happens to have next in memory.
	const std::vector<std::uint8_t> input(data, data + size);

	auto parsed = coroute::http2::FrameHeader::parse(std::span<const std::uint8_t>(input));
	if (!parsed)
	{
		return 0;
	}

	// The wire format is 24 bits of length and 31 bits of stream id. A parse that
	// returns more than that has read the reserved bit or run off the end of a field,
	// and every later size calculation inherits the mistake.
	if (parsed->length > 0x00FFFFFFU)
	{
		__builtin_trap();
	}
	if (parsed->stream_id > 0x7FFFFFFFU)
	{
		__builtin_trap();
	}

	// Round trip, against a re-parse rather than against the input.
	//
	// Comparing with the raw octets is wrong: the high bit of the stream id field is
	// reserved and RFC 9113 says a receiver ignores it, so serialize deliberately does
	// not reproduce it. What has to hold is that parsing is stable once normalised, so
	// a frame this server forwards or echoes means the same thing it read.
	if (input.size() >= 9)
	{
		const auto again = parsed->serialize();
		auto reparsed = coroute::http2::FrameHeader::parse(std::span<const std::uint8_t>(again));
		if (!reparsed)
		{
			__builtin_trap();  // serialize produced nine octets its own parser rejects
		}
		if (reparsed->length != parsed->length || reparsed->type != parsed->type ||
		    reparsed->flags != parsed->flags || reparsed->stream_id != parsed->stream_id)
		{
			__builtin_trap();
		}
	}

	return 0;
}
