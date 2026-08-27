#pragma once

#ifdef COROUTE_HAS_HTTP3

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>

namespace coroute::http3
{

	// ============================================================================
	// QUIC connection IDs, and why the worker index lives inside them
	// ============================================================================
	//
	// SO_REUSEPORT spreads UDP datagrams across sockets by hashing the 4-tuple. That
	// works for a transport where a connection is the 4-tuple, and TCP is such a
	// transport. QUIC is not: a connection survives its client changing address, by
	// design, so that a phone moving from wifi to cellular keeps its session. The
	// moment that happens the 4-tuple changes, the hash changes, and the datagrams
	// land on a worker that knows nothing about the connection.
	//
	// The connection ID is what actually identifies a QUIC connection, and the server
	// chooses its own. So the server can put the answer in the question: byte 0 of
	// every connection ID this server issues carries the index of the worker that owns
	// it. Recovering the owner from a misdirected packet is then a single array
	// access, and the packet can be handed across without a lookup or a lock.
	//
	// nginx and Angie solve the same problem with an eBPF program attached via
	// SO_ATTACH_REUSEPORT_EBPF, which reads the connection ID in the kernel and steers
	// the datagram before userspace sees it. That is faster when it is available and
	// unavailable everywhere else: there is no eBPF on macOS or Windows, and loading a
	// program needs privileges a server often does not have. This encoding is the
	// portable half of that idea, and how much the kernel-side steering is actually
	// worth is a measurement rather than an assumption.

	// RFC 9000 section 5.1: a connection ID is at most 20 bytes.
	inline constexpr std::size_t max_cid_length = 20;

	// The server issues connection IDs of this length. Long enough for the worker byte
	// plus 15 bytes of entropy, which is well past guessing range, and short enough
	// that it costs little on every packet header.
	inline constexpr std::size_t server_cid_length = 16;

	// A connection ID, usable as a hash map key.
	struct CidKey
	{
		std::uint8_t len = 0;
		std::array<std::uint8_t, max_cid_length> bytes{};

		CidKey() = default;
		CidKey(const std::uint8_t* data, std::size_t length) noexcept;

		[[nodiscard]] std::span<const std::uint8_t> view() const noexcept { return {bytes.data(), len}; }

		bool operator==(const CidKey& other) const noexcept;
	};

	// Fills `out` with a fresh server connection ID: the worker index in byte 0, then
	// cryptographically random bytes.
	//
	// The random part is not decoration. RFC 9000 section 5.1 requires connection IDs
	// an off-path observer cannot guess, because guessing one lets an attacker inject
	// packets into someone else's connection. A predictable generator here would be a
	// security hole, not a performance detail.
	//
	// Returns false if randomness could not be obtained, in which case `out` is not
	// usable and the connection must be refused rather than given a weak ID.
	[[nodiscard]] bool cid_fill(std::span<std::uint8_t> out, std::size_t worker_index) noexcept;

	// Recovers the owning worker from a connection ID this server issued.
	//
	// Returns 0 for anything too short to carry the marker, which is the safe answer:
	// worker 0 exists in every configuration, and a packet for an unknown connection
	// is dropped there just as it would be anywhere else.
	//
	// The index is stored modulo the worker count, so it stays valid even if a peer
	// echoes a connection ID from a differently sized run of the server.
	[[nodiscard]] std::size_t cid_worker(std::span<const std::uint8_t> dcid, std::size_t worker_count) noexcept;

}  // namespace coroute::http3

namespace std
{
	template <>
	struct hash<coroute::http3::CidKey>
	{
		std::size_t operator()(const coroute::http3::CidKey& cid) const noexcept;
	};
}  // namespace std

#endif  // COROUTE_HAS_HTTP3
