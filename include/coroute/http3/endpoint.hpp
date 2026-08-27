#pragma once

#ifdef COROUTE_HAS_HTTP3

#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <unordered_map>
#include <vector>

#include "coroute/http3/cid.hpp"
#include "coroute/http3/connection.hpp"
#include "coroute/http3/packet.hpp"
#include "coroute/net/datagram.hpp"
#include "coroute/net/io_context.hpp"
#include "coroute/net/tls.hpp"

namespace coroute::http3
{

	// ============================================================================
	// The UDP side of the server
	// ============================================================================
	//
	// One datagram socket, however many QUIC connections. That ratio is the whole
	// point: TCP gets a descriptor per accepted connection because the kernel
	// demultiplexes by 4-tuple, but QUIC is demultiplexed by connection ID in
	// userspace, so every connection here shares one descriptor.
	//
	// It also means this class has to do what accept() does for TCP. An arriving
	// datagram is checked in a fixed order, and the order matters:
	//
	//   1. a version this server does not speak     -> Version Negotiation
	//   2. a connection ID it recognises            -> hand to that connection
	//   3. a long header on a version it does speak -> a new connection
	//   4. anything else                            -> Stateless Reset
	//
	// Putting the lookup before the accept is what stops a retransmitted Initial from
	// creating a second connection for a client that already has one.

	// A datagram lifted out of the socket's own buffers so it can outlive them.
	//
	// The receive path hands back spans into scratch that is reused on the next call,
	// which is right for the common case and useless for forwarding: a packet destined
	// for another worker has to survive the trip. Copying is the price of the handoff,
	// and the measurement below is what says whether that price is being paid often
	// enough to care about.
	struct OwnedDatagram
	{
		std::vector<std::uint8_t> data;
		net::Endpoint peer;
		net::Endpoint local;
		std::uint8_t ecn = 0;
	};

	// What the endpoint did with what arrived.
	//
	// forwarded_in over received is the number this design exists to produce. nginx and
	// Angie steer QUIC packets in the kernel with an eBPF program attached through
	// SO_ATTACH_REUSEPORT_EBPF, which is faster than forwarding in userspace and needs
	// privileges, a toolchain and a kernel that has it. If the fraction of packets that
	// ever need forwarding is small, the kernel-side version is optimising something
	// that barely happens, and that is a result worth reporting rather than a gap worth
	// filling.
	struct Http3Stats
	{
		std::uint64_t received = 0;
		std::uint64_t forwarded_out = 0;
		std::uint64_t forwarded_in = 0;
		std::uint64_t accepted = 0;
		std::uint64_t version_negotiations = 0;
		std::uint64_t stateless_resets = 0;
		std::uint64_t dropped = 0;
	};

	class Http3Endpoint
	{
	public:
		using RequestHandler = Http3Connection::RequestHandler;

		// Hands a packet to the worker that owns the connection it belongs to.
		using Forwarder = std::function<void(std::size_t worker, OwnedDatagram)>;

		Http3Endpoint(net::IoContext& io, net::TlsContext tls, RequestHandler handler,
		              std::size_t worker_index = 0, std::size_t worker_count = 1) noexcept;

		Http3Endpoint(const Http3Endpoint&) = delete;
		Http3Endpoint& operator=(const Http3Endpoint&) = delete;

		// Binds the UDP socket. `reuse_port` is what lets several workers share a port
		// where the platform load-balances across them.
		[[nodiscard]] expected<void, Error> bind(std::uint16_t port, bool reuse_port = false);

		// Reads and dispatches until stop() is called or the socket fails.
		Task<void> run();

		void stop() noexcept { running_ = false; }

		[[nodiscard]] std::uint16_t local_port() const noexcept;

		[[nodiscard]] std::size_t connection_count() const noexcept { return connections_.size(); }

		// Installed by the group. Without one, every packet is handled here, which is
		// the correct behaviour for a single-worker server.
		void set_forwarder(Forwarder forwarder) { forward_ = std::move(forwarder); }

		// Entry point for a packet another worker forwarded. Runs on this endpoint's
		// worker thread, which is the entire point of the exercise.
		Task<void> deliver(OwnedDatagram datagram);

		[[nodiscard]] Http3Stats stats() const noexcept;

	private:
		Task<void> handle_datagram(const net::Datagram& datagram);

		// Answers a datagram this endpoint has no connection for.
		Task<void> send_version_negotiation(const net::Datagram& datagram, const PacketInfo& info);
		Task<void> send_stateless_reset(const net::Datagram& datagram, const PacketInfo& info);

		// Runs the loss-detection and idle timers, and drops what has closed.
		Task<void> service_timers();

		net::IoContext& io_;
		net::TlsContext tls_;
		RequestHandler handler_;
		std::size_t worker_index_ = 0;
		std::size_t worker_count_ = 1;
		Forwarder forward_;

		// Atomic only because stats() is read from another thread. Every increment
		// happens on this endpoint's own worker, so relaxed ordering is all that is
		// needed: these are counters to report, not flags anything synchronises on.
		std::atomic<std::uint64_t> received_{0};
		std::atomic<std::uint64_t> forwarded_out_{0};
		std::atomic<std::uint64_t> forwarded_in_{0};
		std::atomic<std::uint64_t> accepted_{0};
		std::atomic<std::uint64_t> version_negotiations_{0};
		std::atomic<std::uint64_t> stateless_resets_{0};
		std::atomic<std::uint64_t> dropped_{0};

		std::unique_ptr<net::DatagramSocket> socket_;
		std::unordered_map<CidKey, std::shared_ptr<Http3Connection>> connections_;
		bool running_ = false;
	};

	// ============================================================================
	// Several endpoints, one UDP port
	// ============================================================================
	//
	// SO_REUSEPORT gives each worker its own socket on the same port and the kernel
	// spreads arriving datagrams across them by hashing the 4-tuple. For TCP that is
	// the end of the story, because a TCP connection is its 4-tuple.
	//
	// QUIC is not. A connection survives its client changing address, by design, so
	// that a phone moving from wifi to cellular keeps its session. The moment that
	// happens the hash changes and the datagrams arrive at a worker that knows nothing
	// about the connection. The connection ID is what actually identifies it, and the
	// server chose that ID, so the answer is already inside the question: byte 0 carries
	// the owning worker, and the packet is handed there.
	//
	// Only short-header packets are forwarded. A client's first Initial carries a
	// destination connection ID the client invented, so reading a worker index out of it
	// would be reading random bytes, and most new connections would be bounced to an
	// arbitrary thread for no reason. Long-header packets for an unknown connection are
	// therefore accepted where they land.
	class Http3EndpointGroup
	{
	public:
		using RequestHandler = Http3Connection::RequestHandler;

		Http3EndpointGroup(net::IoContext& io, const net::TlsConfig& tls_config, RequestHandler handler);

		Http3EndpointGroup(const Http3EndpointGroup&) = delete;
		Http3EndpointGroup& operator=(const Http3EndpointGroup&) = delete;

		// Binds every endpoint to the same port with SO_REUSEPORT.
		[[nodiscard]] expected<void, Error> bind(std::uint16_t port);

		// Starts each endpoint on the worker that owns it.
		void start();

		void stop() noexcept;

		[[nodiscard]] std::uint16_t local_port() const noexcept;

		[[nodiscard]] std::size_t worker_count() const noexcept { return endpoints_.size(); }

		// Summed across workers.
		[[nodiscard]] Http3Stats stats() const noexcept;

	private:
		net::IoContext& io_;
		std::vector<std::unique_ptr<Http3Endpoint>> endpoints_;
	};

}  // namespace coroute::http3

#endif  // COROUTE_HAS_HTTP3
