// HPACK header block decoding.
//
// The compression itself is nghttp2's, so this is not fuzzing a third party's
// inflater. What is under test is this codebase's use of it: the loop that drives
// nghttp2_hd_inflate_hd2 and accumulates every emitted field into a vector of strings.
//
// HPACK is the one place in HTTP/2 where a small input produces a large output by
// design, which makes the accumulation the interesting part rather than the parsing.
// An indexed field is one octet and emits a header of up to the dynamic table entry
// size, so the ratio between what a peer sends and what the server allocates is the
// property worth watching.
//
// The input is split into chunks so one decoder sees several blocks in sequence, which
// is what a real connection does and what builds up dynamic table state. Feeding each
// input to a fresh decoder would test only the empty-table case.

#include <coroute/http2/hpack.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace
{
	// A block a peer could actually send in one go, bounded by the default
	// SETTINGS_MAX_FRAME_SIZE. Inputs above it are still decoded, just in pieces.
	constexpr std::size_t max_block = 16384;

	// Where amplification stops being compression and starts being a bomb. Generous on
	// purpose: HPACK is supposed to amplify, and a threshold set at the real ratio
	// would fire on correct behaviour. One megabyte out of a single frame-sized block
	// is not compression working, it is the absence of a limit.
	constexpr std::size_t bomb_threshold = 1U << 20;
}  // namespace

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size)
{
	coroute::http2::HpackDecoder decoder;

	std::size_t consumed = 0;
	std::size_t emitted = 0;

	while (consumed < size)
	{
		const std::size_t chunk = std::min(max_block, size - consumed);
		const std::vector<std::uint8_t> block(data + consumed, data + consumed + chunk);
		consumed += chunk;

		auto headers = decoder.decode(std::span<const std::uint8_t>(block));
		if (!headers)
		{
			// A rejected block is the expected outcome for most inputs. The decoder
			// keeps its table, and a real connection would close, so stop here.
			break;
		}

		for (const auto& header : *headers)
		{
			emitted += header.name.size() + header.value.size();
		}

		if (emitted > bomb_threshold)
		{
			// Reached only if a block of at most 16 KB expanded past a megabyte, which
			// means nothing between the wire and the allocation is counting.
			__builtin_trap();
		}
	}

	return 0;
}
