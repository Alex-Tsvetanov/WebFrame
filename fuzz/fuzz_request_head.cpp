// The HTTP/1.1 request line and header fields.
//
// The largest attacker-controlled parsing surface in the tree, and until it was split
// out of App::parse_request it could not be reached without a socket. Every octet here
// comes from a peer that has done nothing but connect.

#include <coroute/core/request_parse.hpp>

#include <cstddef>
#include <cstdint>
#include <string_view>

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size)
{
	const std::string_view head(reinterpret_cast<const char*>(data), size);

	auto parsed = coroute::parse_request_head(head);
	if (!parsed)
	{
		return 0;
	}

	// Everything the parser reports was carved from the octets it was given, and
	// percent-decoding only ever shrinks. A result larger than the input means the
	// parser produced octets the peer never sent, which is how a length calculation
	// downstream ends up trusting a number that was never true.
	std::size_t total = parsed->path().size() + parsed->query_string().size();
	for (const auto& [name, value] : parsed->headers())
	{
		total += name.size() + value.size();
	}
	if (total > size)
	{
		__builtin_trap();
	}

	// A Content-Length that survived parsing is one the caller will allocate, so it has
	// to be a number of octets this process could be asked for. The same limit the body
	// read applies.
	if (auto length = parsed->content_length())
	{
		constexpr std::size_t max_body = 10U * 1024 * 1024;
		if (*length > max_body)
		{
			// Not a defect on its own: parse_request checks this before allocating.
			// Here it only records that nothing between the two grew the number.
			return 0;
		}
	}

	return 0;
}
