#pragma once

#ifdef COROUTE_HAS_HTTP3

#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>

#include <nghttp3/nghttp3.h>
#include <ngtcp2/ngtcp2.h>
#include <ngtcp2/ngtcp2_crypto.h>
#include <ngtcp2/ngtcp2_crypto_ossl.h>
#include <openssl/ssl.h>

#include "coroute/core/error.hpp"
#include "coroute/core/request.hpp"
#include "coroute/core/response.hpp"
#include "coroute/coro/task.hpp"
#include "coroute/http3/cid.hpp"
#include "coroute/http3/headers.hpp"
#include "coroute/net/datagram.hpp"
#include "coroute/net/tls.hpp"
#include "coroute/util/expected.hpp"

namespace coroute::http3
{

	// ============================================================================
	// One QUIC connection, serving HTTP/3
	// ============================================================================
	//
	// Three state machines stacked on each other, none of which owns a socket:
	//
	//   ngtcp2_conn   QUIC transport: packet protection, loss recovery, flow control
	//   SSL           the TLS 1.3 handshake, driven by ngtcp2 rather than by a BIO
	//   nghttp3_conn  HTTP/3 framing and QPACK, riding on QUIC streams
	//
	// That none of them owns the socket is the reason this project can serve HTTP/3
	// from the same UDP descriptor as everything else. A library that ran its own
	// event loop would force a second one, and the single-descriptor design would be
	// over before it started.
	//
	// Requests leave here as an ordinary Request and come back as an ordinary
	// Response, through the same handler the other protocols use.

	// A timestamp in the form ngtcp2 wants: monotonic nanoseconds.
	[[nodiscard]] ngtcp2_tstamp now_ts() noexcept;

	class Http3Connection : public std::enable_shared_from_this<Http3Connection>
	{
	public:
		using RequestHandler = std::function<Task<Response>(Request&)>;

		Http3Connection(const Http3Connection&) = delete;
		Http3Connection& operator=(const Http3Connection&) = delete;
		~Http3Connection();

		// Builds a connection from a client's Initial packet.
		//
		// `socket` must outlive the connection: replies, including the ones a handler
		// produces long after the request packet was read, are written through it.
		[[nodiscard]] static expected<std::shared_ptr<Http3Connection>, Error> accept(
			net::DatagramSocket& socket, const net::TlsContext& tls, const net::Datagram& initial,
			const CidKey& client_dcid, const CidKey& client_scid, std::uint32_t version, std::size_t worker_index,
			RequestHandler handler);

		// Feeds one received packet in. An error here is connection-fatal.
		[[nodiscard]] expected<void, Error> read_packet(const net::Datagram& datagram);

		// Writes out whatever the three state machines have queued.
		[[nodiscard]] Task<expected<void, Error>> flush();

		// When ngtcp2 next needs attention. UINT64_MAX means no timer is pending.
		[[nodiscard]] ngtcp2_tstamp expiry() const noexcept;

		// Runs the loss-detection and idle timers. Call once expiry() has passed.
		[[nodiscard]] expected<void, Error> handle_expiry();

		// The connection ID this server chose, which is what the endpoint keys on.
		[[nodiscard]] const CidKey& scid() const noexcept { return scid_; }

		[[nodiscard]] bool is_closed() const noexcept { return closed_; }

		// ---- callback bodies
		//
		// Public because ngtcp2 and nghttp3 reach them through C function pointers,
		// which cannot be members. They are not part of the interface: the free
		// functions in the .cpp are the only callers, and a bridge layer to keep them
		// private would be more code than the encapsulation is worth here.
		int on_recv_stream_data(std::uint32_t flags, std::int64_t stream_id, const std::uint8_t* data, std::size_t len);
		int on_stream_close(std::int64_t stream_id, std::uint64_t app_error_code);
		int on_acked_stream_data(std::int64_t stream_id, std::uint64_t datalen);
		int on_handshake_completed();
		int on_new_connection_id(ngtcp2_cid* cid, ngtcp2_stateless_reset_token* token, std::size_t cidlen);

		int on_h3_recv_header(std::int64_t stream_id, std::span<const std::uint8_t> name,
		                      std::span<const std::uint8_t> value);
		int on_h3_end_stream(std::int64_t stream_id);
		int on_h3_stream_close(std::int64_t stream_id);
		int on_h3_recv_data(std::int64_t stream_id, std::span<const std::uint8_t> data);

	private:
		Http3Connection(net::DatagramSocket& socket, std::size_t worker_index, RequestHandler handler) noexcept;

		// Per-stream state. A request arrives in pieces, so the parts accumulate here
		// until the stream ends and the handler can run.
		struct Stream
		{
			RequestBuilder builder;
			std::string body;

			// The response, kept alive because nghttp3 reads the body out of it lazily,
			// after submit_response has already returned.
			Response response{200, {}, ""};
			std::vector<HeaderField> fields;
			std::vector<nghttp3_nv> nva;
		};

		[[nodiscard]] expected<void, Error> setup_tls(const net::TlsContext& tls);
		[[nodiscard]] expected<void, Error> setup_http3();

		// Runs the application handler and submits what it returns.
		Task<void> dispatch(std::int64_t stream_id);

		[[nodiscard]] Stream* find_stream(std::int64_t stream_id) noexcept;

		net::DatagramSocket& socket_;
		std::size_t worker_index_ = 0;
		RequestHandler handler_;

		ngtcp2_conn* conn_ = nullptr;
		ngtcp2_crypto_conn_ref conn_ref_{};
		SSL* ssl_ = nullptr;
		ngtcp2_crypto_ossl_ctx* ossl_ctx_ = nullptr;
		nghttp3_conn* h3_ = nullptr;

		CidKey scid_;
		net::Endpoint peer_;
		net::Endpoint local_;

		std::unordered_map<std::int64_t, Stream> streams_;
		bool closed_ = false;
		bool handshake_done_ = false;
	};

}  // namespace coroute::http3

#endif  // COROUTE_HAS_HTTP3
