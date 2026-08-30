#include "coroute/http3/endpoint.hpp"

#ifdef COROUTE_HAS_HTTP3

#include <algorithm>
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
	                             std::size_t worker_index, std::size_t worker_count) noexcept
	    : io_(io),
	      tls_(std::move(tls)),
	      handler_(std::move(handler)),
	      worker_index_(worker_index),
	      worker_count_(worker_count > 0 ? worker_count : 1)
	{
	}

	Http3Stats Http3Endpoint::stats() const noexcept
	{
		return Http3Stats{
			.received = received_.load(std::memory_order_relaxed),
			.forwarded_out = forwarded_out_.load(std::memory_order_relaxed),
			.forwarded_in = forwarded_in_.load(std::memory_order_relaxed),
			.accepted = accepted_.load(std::memory_order_relaxed),
			.version_negotiations = version_negotiations_.load(std::memory_order_relaxed),
			.stateless_resets = stateless_resets_.load(std::memory_order_relaxed),
			.dropped = dropped_.load(std::memory_order_relaxed),
		};
	}

	Task<void> Http3Endpoint::deliver(OwnedDatagram datagram)
	{
		forwarded_in_.fetch_add(1, std::memory_order_relaxed);

		const net::Datagram view{
			.data = {datagram.data.data(), datagram.data.size()},
			.peer = datagram.peer,
			.local = datagram.local,
			.ecn = datagram.ecn,
		};
		co_await handle_datagram(view);
	}

	expected<void, Error> Http3Endpoint::bind(std::uint16_t port, bool reuse_port)
	{
		// The worker index goes to the socket too, so its completions are processed by
		// the same thread that owns the connections it will carry.
		socket_ = net::DatagramSocket::create(io_, worker_index_);
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
		received_.fetch_add(1, std::memory_order_relaxed);

		const PacketInfo info = classify_packet(datagram.data);
		if (info.kind == PacketKind::Malformed)
		{
			// Nothing can be said about a packet whose header will not parse, including
			// who to say it to. Dropping is the only correct answer.
			dropped_.fetch_add(1, std::memory_order_relaxed);
			co_return;
		}

		// A version this server does not speak has to be answered before anything else:
		// the connection ID in such a packet was chosen under rules that may not be the
		// ones in force here, so it is not safe to look up.
		if (info.kind == PacketKind::LongHeader && info.version != quic_version_v1)
		{
			version_negotiations_.fetch_add(1, std::memory_order_relaxed);
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

		// Not ours, and short-header, so it belongs to a connection that already exists
		// somewhere. The connection ID says where: this server chose it and put the
		// owning worker in byte 0.
		//
		// Short headers only. A client's first Initial carries a destination connection
		// ID the client invented, so decoding a worker index out of it would be decoding
		// random bytes and bouncing most new connections to an arbitrary thread.
		if (info.kind == PacketKind::ShortHeader && forward_)
		{
			const std::size_t owner = cid_worker(info.dcid.view(), worker_count_);
			if (owner != worker_index_)
			{
				forwarded_out_.fetch_add(1, std::memory_order_relaxed);
				// Copied, because the span points into socket scratch that the next
				// receive will overwrite, and this packet is about to cross a thread.
				forward_(owner, OwnedDatagram{
									.data = {datagram.data.begin(), datagram.data.end()},
									.peer = datagram.peer,
									.local = datagram.local,
									.ecn = datagram.ecn,
								});
				co_return;
			}
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
			// Additional IDs issued during the handshake must resolve to the same
			// connection: a migrating client is required to pick a fresh destination
			// connection ID, and without the alias that packet looks unknown here.
			stored->set_cid_alias_tracker(
				[this](Http3Connection* connection, const CidKey& cid) { track_additional_cid(connection, cid); });
			accepted_.fetch_add(1, std::memory_order_relaxed);
			(void)co_await stored->flush();
			co_return;
		}

		// A short header for a connection ID that is not here. Either this server
		// restarted, or the connection was already dropped. Either way the peer is
		// talking to something that no longer exists and should be told now rather
		// than waiting out its idle timeout.
		stateless_resets_.fetch_add(1, std::memory_order_relaxed);
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
		std::vector<std::shared_ptr<Http3Connection>> due;

		for (auto& [cid, connection] : connections_)
		{
			(void)cid;
			if (!connection->is_closed() && connection->expiry() <= now)
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
		}

		// Drop every map entry whose connection has closed, including aliases created
		// by track_additional_cid: leaving those behind would keep the shared_ptr alive
		// and would also answer a late packet for a dead connection as if it were live.
		std::vector<CidKey> expired;
		for (auto& [cid, connection] : connections_)
		{
			if (connection->is_closed())
			{
				expired.push_back(cid);
			}
		}
		for (const auto& cid : expired)
		{
			connections_.erase(cid);
		}
	}

	void Http3Endpoint::track_additional_cid(Http3Connection* connection, const CidKey& cid)
	{
		// Find the shared_ptr first, finishing the scan before any insert. ngtcp2 issues
		// a burst of NEW_CONNECTION_ID frames right after handshake completion; inserting
		// via operator[] mid-range-for can rehash and invalidate the iterator.
		std::shared_ptr<Http3Connection> found;
		for (auto& [existing_cid, existing] : connections_)
		{
			(void)existing_cid;
			if (existing.get() == connection)
			{
				found = existing;
				break;
			}
		}
		if (found)
		{
			connections_[cid] = std::move(found);
		}
	}

	// ========================================================================
	// Http3EndpointGroup
	// ========================================================================

	Http3EndpointGroup::Http3EndpointGroup(net::IoContext& io, const net::TlsConfig& tls_config,
	                                       RequestHandler handler)
	    : io_(io)
	{
		// One endpoint per worker, but only where a callback can be aimed at a named
		// worker. Without that a forwarded packet would be handled by whichever thread
		// happened to pick it up, and two threads inside one ngtcp2_conn is a data race
		// rather than a slow path. Falling back to a single endpoint is slower and
		// correct, which is the right way round.
		const std::size_t workers =
			io.supports_worker_affinity() ? std::max<std::size_t>(io.worker_count(), 1) : 1;

		endpoints_.reserve(workers);
		for (std::size_t index = 0; index < workers; ++index)
		{
			// Each endpoint gets its own TLS context rather than sharing one. An SSL_CTX
			// is reference counted and shareable, but sessions are created from it on
			// every worker, and giving each thread its own removes the contention without
			// costing anything but a little memory at startup.
			auto tls = net::TlsContext::create_quic(tls_config);
			if (!tls)
			{
				// Reported at bind() rather than thrown from a constructor, so the caller
				// sees one failure path instead of two.
				endpoints_.clear();
				return;
			}
			endpoints_.push_back(
				std::make_unique<Http3Endpoint>(io, std::move(*tls), handler, index, workers));
		}

		if (endpoints_.size() > 1)
		{
			for (auto& endpoint : endpoints_)
			{
				endpoint->set_forwarder(
					[this](std::size_t worker, OwnedDatagram datagram)
					{
						Http3Endpoint* target = endpoints_[worker % endpoints_.size()].get();
						io_.run_on_worker(worker,
						                  [target, datagram = std::move(datagram)]() mutable
						                  { target->deliver(std::move(datagram)).start_detached(); });
					});
			}
		}
	}

	expected<void, Error> Http3EndpointGroup::bind(std::uint16_t port)
	{
		if (endpoints_.empty())
		{
			return unexpected(Error::io(IoError::Unknown, "HTTP/3 endpoint group has no endpoints"));
		}

		// reuse_port only when there is more than one socket to share the port. Asking
		// for it with a single socket would work but would also let an unrelated process
		// quietly bind the same port alongside this one.
		const bool share = endpoints_.size() > 1;

		for (auto& endpoint : endpoints_)
		{
			if (auto bound = endpoint->bind(port, share); !bound)
			{
				return bound;
			}
		}

		// Port 0 asks the kernel to choose, and it would choose a different port for
		// each socket. Every worker has to be on the same one or the shared-port design
		// does not exist.
		if (port == 0 && share)
		{
			return unexpected(Error::io(IoError::InvalidArgument,
			                            "HTTP/3 needs an explicit port when sharing it across workers"));
		}

		return {};
	}

	void Http3EndpointGroup::start()
	{
		for (std::size_t index = 0; index < endpoints_.size(); ++index)
		{
			Http3Endpoint* endpoint = endpoints_[index].get();
			// Started on the worker that owns it, so the receive loop and the connections
			// it creates live on the same thread from the first packet onward.
			io_.run_on_worker(index, [endpoint] { endpoint->run().start_detached(); });
		}
	}

	void Http3EndpointGroup::stop() noexcept
	{
		for (auto& endpoint : endpoints_)
		{
			endpoint->stop();
		}
	}

	std::uint16_t Http3EndpointGroup::local_port() const noexcept
	{
		return endpoints_.empty() ? 0 : endpoints_.front()->local_port();
	}

	Http3Stats Http3EndpointGroup::stats() const noexcept
	{
		Http3Stats total;
		for (const auto& endpoint : endpoints_)
		{
			const Http3Stats one = endpoint->stats();
			total.received += one.received;
			total.forwarded_out += one.forwarded_out;
			total.forwarded_in += one.forwarded_in;
			total.accepted += one.accepted;
			total.version_negotiations += one.version_negotiations;
			total.stateless_resets += one.stateless_resets;
			total.dropped += one.dropped;
		}
		return total;
	}

}  // namespace coroute::http3

#endif  // COROUTE_HAS_HTTP3
