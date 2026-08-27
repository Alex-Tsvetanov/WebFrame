#include "coroute/http3/endpoint.hpp"

#ifdef COROUTE_HAS_HTTP3

#include <array>
#include <vector>

#include "coroute/http3/packet.hpp"
#include "coroute/http3/stateless.hpp"

namespace coroute::http3
{

	namespace
	{
		// Enough for any datagram this server will send in reply to an unknown packet.
		// Both Version Negotiation and Stateless Reset are far smaller than this.
		constexpr std::size_t reply_buffer_size = 1500;
	}  // namespace

	Http3Endpoint::Http3Endpoint(net::IoContext& io, net::TlsContext tls, RequestHandler handler,
	                             std::size_t worker_index) noexcept
	    : io_(io), tls_(std::move(tls)), handler_(std::move(handler)), worker_index_(worker_index)
	{
	}

	expected<void, Error> Http3Endpoint::bind(std::uint16_t port, bool reuse_port)
	{
		socket_ = net::DatagramSocket::create(io_);
		if (!socket_)
		{
			return unexpected(Error::io(IoError::Unknown, "could not create a datagram socket"));
		}
		return socket_->bind(port, reuse_port);
	}

	std::uint16_t Http3Endpoint::local_port() const noexcept
	{
		return socket_ ? socket_->local_port() : 0;
	}

	Task<void> Http3Endpoint::run()
	{
		if (!socket_ || !socket_->is_open())
		{
			co_return;
		}

		running_ = true;
		while (running_)
		{
			auto batch = co_await socket_->async_recv_batch();
			if (!batch)
			{
				break;
			}

			// The batch points into buffers the socket owns and will reuse on the next
			// receive, so every datagram has to be dealt with before looping.
			for (const auto& datagram : *batch)
			{
				co_await handle_datagram(datagram);
			}

			co_await service_timers();
		}

		running_ = false;
	}

	Task<void> Http3Endpoint::handle_datagram(const net::Datagram& datagram)
	{
		const PacketInfo info = classify_packet(datagram.data);
		if (info.kind == PacketKind::Malformed)
		{
			// Nothing can be said about a packet whose header will not parse, including
			// who to say it to. Dropping is the only correct answer.
			co_return;
		}

		// A version this server does not speak has to be answered before anything else:
		// the connection ID in such a packet was chosen under rules that may not be the
		// ones in force here, so it is not safe to look up.
		if (info.kind == PacketKind::LongHeader && info.version != quic_version_v1)
		{
			co_await send_version_negotiation(datagram, info);
			co_return;
		}

		// Before accepting. A retransmitted Initial arrives with the same connection ID
		// as the one that created the connection, so looking up first is what stops it
		// from creating a second.
		if (const auto it = connections_.find(info.dcid); it != connections_.end())
		{
			auto connection = it->second;
			if (connection->read_packet(datagram))
			{
				(void)co_await connection->flush();
			}
			co_return;
		}

		if (info.kind == PacketKind::LongHeader)
		{
			auto connection = Http3Connection::accept(*socket_, tls_, datagram, info.dcid, info.scid, info.version,
			                                          worker_index_, handler_);
			if (!connection)
			{
				// A handshake that will not start is not worth a reply: the client has
				// its own timeout, and answering would only tell an unauthenticated
				// peer that something is listening.
				co_return;
			}
			// Held by value, not as a reference into the map. Rehashing would not
			// invalidate the reference, but erasing the element would, and the flush
			// below suspends: anything that sweeps closed connections while it is
			// suspended would leave the reference dangling. A shared_ptr copy costs an
			// increment and removes the question.
			auto stored = std::move(*connection);
			connections_[stored->scid()] = stored;
			(void)co_await stored->flush();
			co_return;
		}

		// A short header for a connection ID that is not here. Either this server
		// restarted, or the connection was already dropped. Either way the peer is
		// talking to something that no longer exists and should be told now rather
		// than waiting out its idle timeout.
		co_await send_stateless_reset(datagram, info);
	}

	Task<void> Http3Endpoint::send_version_negotiation(const net::Datagram& datagram, const PacketInfo& info)
	{
		std::array<std::uint8_t, reply_buffer_size> buffer{};
		const std::size_t len = write_version_negotiation(buffer, info.dcid, info.scid);
		if (len == 0)
		{
			co_return;
		}
		(void)co_await socket_->async_send({buffer.data(), len}, datagram.peer, datagram.local);
	}

	Task<void> Http3Endpoint::send_stateless_reset(const net::Datagram& datagram, const PacketInfo& info)
	{
		std::array<std::uint8_t, reply_buffer_size> buffer{};
		const StatelessResetToken token = derive_reset_token(server_reset_secret(), info.dcid);

		// The reply is deliberately smaller than what provoked it. Returns 0 when no
		// compliant reset fits, which is the right outcome for a tiny packet: an
		// attacker spoofing a victim's address must not be able to aim amplified
		// traffic at them (RFC 9000 section 10.3).
		const std::size_t len = write_stateless_reset(buffer, token, datagram.data.size());
		if (len == 0)
		{
			co_return;
		}
		(void)co_await socket_->async_send({buffer.data(), len}, datagram.peer, datagram.local);
	}

	Task<void> Http3Endpoint::service_timers()
	{
		const ngtcp2_tstamp now = now_ts();

		// Collected first because flushing a connection can close it, and erasing from
		// the map while iterating it would invalidate the iterator.
		std::vector<CidKey> expired;
		std::vector<std::shared_ptr<Http3Connection>> due;

		for (auto& [cid, connection] : connections_)
		{
			if (connection->is_closed())
			{
				expired.push_back(cid);
			}
			else if (connection->expiry() <= now)
			{
				due.push_back(connection);
			}
		}

		for (auto& connection : due)
		{
			if (connection->handle_expiry())
			{
				(void)co_await connection->flush();
			}
			if (connection->is_closed())
			{
				expired.push_back(connection->scid());
			}
		}

		for (const auto& cid : expired)
		{
			connections_.erase(cid);
		}
	}

}  // namespace coroute::http3

#endif  // COROUTE_HAS_HTTP3
