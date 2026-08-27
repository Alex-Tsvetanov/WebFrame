#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>

#include "coroute/net/io_context.hpp"

namespace coroute::net
{

	// ============================================================================
	// UDP datagram I/O
	// ============================================================================
	//
	// The second half of the socket-minimal design. TCP and UDP are different
	// transports and cannot share a descriptor, so an endpoint serving HTTP/1.1,
	// HTTP/2, WebSocket and HTTP/3 needs exactly two: one stream socket, one datagram
	// socket. That is the floor, and it does not move as more application protocols
	// are added on either side.
	//
	// This exists for QUIC. It is deliberately not a general-purpose UDP API: the
	// shape is what a QUIC endpoint needs, which is batched receive with the local
	// address attached and a send that can carry a GSO segment size.

	// Storage for a socket address, sized for sockaddr_in6.
	//
	// Kept opaque so that <sys/socket.h> and <winsock2.h> stay out of a public header.
	// Winsock in particular is order-sensitive about being included before windows.h,
	// and this header is reachable from most of the tree.
	struct Endpoint
	{
		static constexpr size_t capacity = 32;  // sockaddr_in6 is 28 bytes on both

		alignas(8) std::array<std::byte, capacity> bytes{};
		std::uint32_t len = 0;

		[[nodiscard]] bool empty() const noexcept { return len == 0; }
	};

	// One received datagram.
	//
	// `data` borrows from the socket's receive buffer and stays valid only until the
	// next async_recv_batch on the same socket. QUIC copies what it needs into
	// connection state during processing, so this avoids a copy per packet on the
	// hottest path there is.
	struct Datagram
	{
		std::span<const std::uint8_t> data;

		Endpoint peer;   // who sent it
		Endpoint local;  // which local address it arrived on, from IP_PKTINFO

		std::uint8_t ecn = 0;  // ECN codepoint, for QUIC congestion response
	};

	class DatagramSocket
	{
	public:
		virtual ~DatagramSocket() = default;

		// Bind to `port` on the wildcard address.
		//
		// reuse_port shards the endpoint across workers the way the TCP listener does.
		// Note that for QUIC this is not sufficient on its own: the kernel hashes the
		// 4-tuple, but a QUIC connection survives its client changing address, so a
		// migrated packet arrives on the wrong worker. Routing it by connection ID is
		// a separate concern handled above this layer.
		virtual expected<void, Error> bind(uint16_t port, bool reuse_port = false) = 0;

		// Receive a batch of datagrams.
		//
		// The returned span, and the buffers the datagrams point into, are valid until
		// the next call on this socket. Backends normalise GRO-coalesced segments into
		// separate entries, so the caller never sees a merged datagram: QUIC must
		// process each one as its own packet.
		virtual Task<expected<std::span<const Datagram>, Error>> async_recv_batch() = 0;

		// Send one datagram, or several at once when gso_size is non-zero.
		//
		// `local` selects the source address. It matters: a wildcard-bound socket on a
		// multi-homed host will otherwise let the kernel pick, and a QUIC client that
		// receives a reply from an address it did not send to will discard it.
		//
		// gso_size > 0 asks the kernel to segment `data` into chunks of that size,
		// which turns N sends into one syscall. Backends without segmentation offload
		// loop instead, so the observable behaviour is the same either way.
		virtual Task<expected<size_t, Error>> async_send(std::span<const std::uint8_t> data, const Endpoint& peer,
		                                                 const Endpoint& local, size_t gso_size = 0) = 0;

		virtual void close() = 0;
		[[nodiscard]] virtual bool is_open() const noexcept = 0;
		[[nodiscard]] virtual uint16_t local_port() const noexcept = 0;

		// True when the backend actually offloads segmentation. Reported so a
		// measurement can distinguish a real GSO path from the emulated loop.
		[[nodiscard]] virtual bool has_segmentation_offload() const noexcept { return false; }

		// Factory. Returns nullptr on backends that do not implement datagrams yet.
		//
		// `worker_index` names the worker this socket's I/O belongs to. It matters for
		// QUIC sharding: several sockets share one UDP port through SO_REUSEPORT, one
		// per worker, and each one's completions have to be processed by its own worker
		// or the whole point of the split is lost and everything funnels through one
		// thread. Backends without per-worker queues ignore it, which is what
		// IoContext::supports_worker_affinity() reports.
		static std::unique_ptr<DatagramSocket> create(IoContext& ctx, std::size_t worker_index = 0);
	};

}  // namespace coroute::net
