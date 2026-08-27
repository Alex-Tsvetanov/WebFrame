#pragma once

#ifdef COROUTE_HAS_HTTP3

#include <cstdint>
#include <memory>
#include <unordered_map>

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

	class Http3Endpoint
	{
	public:
		using RequestHandler = Http3Connection::RequestHandler;

		Http3Endpoint(net::IoContext& io, net::TlsContext tls, RequestHandler handler,
		              std::size_t worker_index = 0) noexcept;

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

		std::unique_ptr<net::DatagramSocket> socket_;
		std::unordered_map<CidKey, std::shared_ptr<Http3Connection>> connections_;
		bool running_ = false;
	};

}  // namespace coroute::http3

#endif  // COROUTE_HAS_HTTP3
