#pragma once

// Server-side QUIC + HTTP/3 wiring for Coroute.
//
// NOTE: This header intentionally avoids including <winsock2.h>, <ws2tcpip.h>
// and <openssl/ssl.h>. winsock2 defines `DELETE` as a macro which collides
// with HttpMethod::DELETE, and openssl drags in a lot of transitive includes.
// All platform-specific / third-party types are forward-declared here, and
// their concrete definitions are used only inside the .cpp translation
// units.

#include "coroute/coro/task.hpp"
#include "coroute/core/error.hpp"
#include "coroute/core/request.hpp"
#include "coroute/core/response.hpp"
#include "coroute/http3/quic_server_settings.hpp"
#include "coroute/http3/retry_token.hpp"
#include "coroute/net/io_context.hpp"
#include "coroute/util/expected.hpp"

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <coroutine>
#include <deque>
#include <functional>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>

// Forward declarations for opaque third-party types.
struct ngtcp2_conn;
struct ngtcp2_crypto_ossl_ctx;
struct ngtcp2_crypto_conn_ref;
struct ssl_st;
typedef struct ssl_st SSL;
struct nghttp3_conn;

namespace coroute::net
{
	class TlsContext;
}

namespace coroute::http3
{

class QuicServer;
class QuicConnection;
class Http3Connection;
class Http3Stream;

// ---------------------------------------------------------------------------
// SockAddrStorage - opaque fixed-size buffer holding a sockaddr_storage.
//
// The .cpp has a static_assert(sizeof(sockaddr_storage) <= 128) to guard
// against platform changes silently corrupting memory.
// ---------------------------------------------------------------------------
struct alignas(16) SockAddrStorage
{
	std::array<std::byte, 128> bytes{};
	int len = 0;  // actual length (socklen_t)
};

// ---------------------------------------------------------------------------
// Http3Stream
// ---------------------------------------------------------------------------
class Http3Stream
{
public:
	Http3Stream(Http3Connection* conn, int64_t stream_id,
	            bool received_in_0rtt = false);
	~Http3Stream() = default;

	Http3Stream(const Http3Stream&) = delete;
	Http3Stream& operator=(const Http3Stream&) = delete;

	int64_t id() const noexcept { return stream_id_; }

	// Read-only access to the Request being accumulated via callbacks. Used by
	// tests to verify header and body accumulation without needing a live
	// QUIC handshake.
	const coroute::Request& request() const noexcept { return req_; }

	// nghttp3 -> Http3Stream callbacks (invoked from trampolines in the .cpp).
	void on_header(std::string name, std::string value);
	void on_body(const uint8_t* data, size_t len);
	void on_end_headers();
	void on_end_stream();

	// Queue an HTTP/3 response. Does not await; wakes the write loop.
	void send_response(const coroute::Response& res);

	// Called by nghttp3's data reader callback. Returns a pointer into the
	// owned body buffer (valid for the stream's lifetime) and sets *remaining
	// to the number of bytes available from that pointer. Sets *eof once the
	// buffer is fully drained.
	uint8_t* pull_body_range(size_t* remaining, bool* eof);

	Http3Connection* connection() const noexcept { return conn_; }

	// True if this stream arrived during the 0-RTT early-data phase.
	// The server's on_end_stream path checks the replay cache when this
	// is set, and responds 425 Too Early if a replay is detected.
	bool received_in_0rtt() const noexcept { return received_in_0rtt_; }
	void mark_received_in_0rtt() noexcept { received_in_0rtt_ = true; }

private:
	Http3Connection* conn_;
	int64_t stream_id_;

	// Request being accumulated from incoming HEADERS + DATA frames.
	coroute::Request req_;

	// Incoming request-body arena.
	//
	// Using a chunked deque of fixed-size pages instead of a single
	// growing std::string eliminates O(n²) copy behaviour on large uploads:
	// std::string::append can trigger a full-buffer copy when capacity is
	// exceeded; the chunked layout never moves existing data.
	//
	// Each page holds kBodyChunkSize bytes. The last page is partially
	// filled; `body_last_chunk_used_` tracks how many bytes of it are live.
	// At on_end_stream time, all pages are coalesced into req_ exactly once.
	static constexpr std::size_t kBodyChunkSize = 8192;
	std::deque<std::array<uint8_t, kBodyChunkSize>> body_chunks_;
	std::size_t body_last_chunk_used_ = 0;

	// Append `len` bytes from `data` to the chunked request-body arena.
	// O(n) total over all calls for a given stream — no full-buffer copies.
	void append_body_chunk(const uint8_t* data, size_t len);

	// Coalesce the chunked arena into req_.body() once (called from
	// on_end_stream). After this call the arena is no longer needed.
	void coalesce_body_to_request();

	// Response state. Kept alive for the duration of the stream (destroyed
	// only when Http3Connection removes the stream after stream_close).
	// hdr_storage_ holds owned name/value strings as alternating entries
	// (even indices = names, odd indices = values).
	std::vector<std::string> hdr_storage_;
	std::string body_buffer_;
	size_t body_offset_ = 0;
	bool response_submitted_  = false;
	bool received_in_0rtt_    = false;
};

// ---------------------------------------------------------------------------
// Http3Connection
// ---------------------------------------------------------------------------
class Http3Connection
{
public:
	explicit Http3Connection(QuicConnection* quic_conn);
	~Http3Connection();

	Http3Connection(const Http3Connection&) = delete;
	Http3Connection& operator=(const Http3Connection&) = delete;

	// Open control + QPACK streams and send SETTINGS. Called once, after the
	// QUIC handshake completes.
	int setup_server_streams();

	// Feed bytes received on a QUIC stream. Returns the number of bytes the
	// app consumed (for flow control credit), or a negative nghttp3 error.
	// `received_in_0rtt` should be true when the packet had
	// NGTCP2_STREAM_DATA_FLAG_0RTT set (data arrived in the early-data phase).
	long long on_stream_data(int64_t stream_id, const uint8_t* data, size_t datalen,
	                          bool fin, bool received_in_0rtt = false);

	// Stream lifecycle from the QUIC layer.
	int on_stream_close(int64_t stream_id, uint64_t app_error_code);
	int on_stream_reset(int64_t stream_id);
	int on_stream_stop_sending(int64_t stream_id);
	int on_acked_stream_data(int64_t stream_id, uint64_t datalen);

	nghttp3_conn* http3_conn() const noexcept { return http3_conn_; }
	QuicConnection* quic_conn() const noexcept { return quic_conn_; }

	Http3Stream* get_or_create_stream(int64_t stream_id,
	                                   bool received_in_0rtt = false);
	Http3Stream* find_stream(int64_t stream_id);
	void remove_stream(int64_t stream_id);

private:
	QuicConnection* quic_conn_;
	nghttp3_conn* http3_conn_ = nullptr;
	std::unordered_map<int64_t, std::unique_ptr<Http3Stream>> streams_;
};

// ---------------------------------------------------------------------------
// QuicConnection
// ---------------------------------------------------------------------------
class QuicConnection
{
public:
	QuicConnection(QuicServer* server,
	               const net::UdpEndpoint& peer,
	               const uint8_t* dcid, size_t dcid_len,
	               const uint8_t* scid, size_t scid_len,
	               uint32_t client_chosen_version);

	~QuicConnection();

	QuicConnection(const QuicConnection&) = delete;
	QuicConnection& operator=(const QuicConnection&) = delete;

	bool initialised() const noexcept { return conn_ != nullptr; }
	bool closed() const noexcept { return closed_; }
	void mark_closed() { closed_ = true; }

	// Feed a UDP datagram to ngtcp2.
	void on_datagram(const uint8_t* data, size_t datalen, const net::UdpEndpoint& peer);

	// Drain all pending outbound QUIC packets via ngtcp2_conn_writev_stream.
	void do_write();
	void schedule_write();

	QuicServer* server() const noexcept { return server_; }
	ngtcp2_conn* quic_conn() const noexcept { return conn_; }
	Http3Connection* http3() const noexcept { return http3_.get(); }

	// Trampoline targets (public so free-function callbacks in the .cpp can
	// dispatch to them after resolving void* user_data).
	void on_handshake_completed();
	int on_recv_stream_data(int64_t stream_id, const uint8_t* data, size_t len,
	                        bool fin, uint32_t ngtcp2_flags);
	int on_stream_close(int64_t stream_id, uint64_t app_error_code, bool app);
	int on_stream_reset(int64_t stream_id);
	int on_stream_stop_sending(int64_t stream_id);
	int on_acked_stream_data_offset(int64_t stream_id, uint64_t offset, uint64_t datalen);
	int on_extend_max_stream_data(int64_t stream_id, uint64_t max_data);

	// QUIC datagram support (RFC 9221).
	// Called by cb_recv_datagram when a DATAGRAM frame arrives.
	void on_recv_datagram(const uint8_t* data, size_t datalen);

	// Write a datagram frame to the peer. Returns an error if the peer did not
	// advertise `max_datagram_frame_size` in their transport parameters or if
	// the connection is closed.
	[[nodiscard]] expected<void, Error> send_datagram(std::span<const uint8_t> data);

	// Suspend the caller until an inbound QUIC datagram arrives.
	// Returns std::nullopt if the connection closes before any datagram arrives.
	Task<std::optional<std::vector<uint8_t>>> next_datagram();

	// Restart the idle/retransmit timer from the ngtcp2 expiry.
	void arm_timer();

	// Local / peer addresses are populated on the first received datagram.
	SockAddrStorage& local_addr() noexcept { return local_addr_; }
	SockAddrStorage& peer_addr() noexcept { return peer_addr_; }

	const net::UdpEndpoint& peer_endpoint() const noexcept { return peer_endpoint_; }

	// Connection ID (server-chosen SCID) used to key QuicServer::connections_.
	const std::string& cid_key() const noexcept { return cid_key_; }
	void set_cid_key(std::string key) { cid_key_ = std::move(key); }

private:
	QuicServer* server_;
	net::UdpEndpoint peer_endpoint_;
	std::string cid_key_;

	SockAddrStorage local_addr_;
	SockAddrStorage peer_addr_;

	ngtcp2_conn* conn_ = nullptr;
	SSL* ssl_ = nullptr;
	ngtcp2_crypto_ossl_ctx* ossl_ctx_ = nullptr;

	// conn_ref_ wraps a pointer back to us so the OSSL backend can find the
	// ngtcp2_conn from the SSL object. Held via an opaque PImpl to avoid
	// leaking ngtcp2 types into this header.
	struct ConnRefHolder;
	std::unique_ptr<ConnRefHolder> conn_ref_holder_;

	std::unique_ptr<Http3Connection> http3_;

	bool closed_ = false;
	bool write_scheduled_ = false;
	bool handshake_completed_ = false;
	uint64_t timer_generation_ = 0;

	// Inbound QUIC datagrams (RFC 9221). Bounded FIFO; oldest dropped on overflow.
	static constexpr std::size_t kMaxInboundDatagramQueue = 256;
	std::deque<std::vector<uint8_t>> inbound_datagrams_;
	// Coroutine suspended in next_datagram() waiting for the next inbound frame.
	std::coroutine_handle<> datagram_waiter_;
};

// ---------------------------------------------------------------------------
// QuicServer
// ---------------------------------------------------------------------------
class QuicServer
{
public:
	using Handler = std::function<Task<Response>(Request)>;

	QuicServer(net::IoContext& ctx, net::TlsContext* tls_ctx = nullptr);
	~QuicServer();

	QuicServer(const QuicServer&) = delete;
	QuicServer& operator=(const QuicServer&) = delete;

	expected<void, Error> listen(uint16_t port);
	Task<void> run();

	void set_handler(Handler h) { handler_ = std::move(h); }
	const Handler& handler() const noexcept { return handler_; }
	bool has_handler() const noexcept { return static_cast<bool>(handler_); }

	// Apply server-wide hardening settings. Must be called before listen().
	void set_settings(Http3ServerSettings s) { settings_ = std::move(s); }
	const Http3ServerSettings& settings() const noexcept { return settings_; }

	// Enqueue a UDP datagram for sending (owns `buffer`).
	void send_datagram(std::vector<uint8_t> buffer, const net::UdpEndpoint& peer);

	net::IoContext& io_context() noexcept { return ctx_; }
	net::TlsContext* tls_context() noexcept { return tls_ctx_; }
	uint16_t port() const noexcept { return port_; }

	void drop_connection(const std::string& cid_key);
	void register_connection(std::string cid_key, std::shared_ptr<QuicConnection> c);
	// Register an alias CID that maps to an already-registered connection.
	void add_cid_alias(std::string cid_key, const std::shared_ptr<QuicConnection>& c);
	// Called from ngtcp2's get_new_connection_id callback: locate the
	// existing shared_ptr for `conn` (by scanning) and register it under the
	// newly-minted CID.
	void track_additional_cid(QuicConnection* conn, std::string cid_key);

	// Returns true when stateless retry is enabled and the given source address
	// has exceeded the threshold of Initial packets in the current window.
	// Advances the per-IP counter; resets the window once per second.
	[[nodiscard]] bool should_send_retry(const net::UdpEndpoint& peer);

private:
	net::IoContext& ctx_;
	net::TlsContext* tls_ctx_ = nullptr;
	std::unique_ptr<net::UdpSocket> socket_;
	bool stopped_ = false;
	uint16_t port_ = 0;

	Http3ServerSettings settings_;
	Handler handler_;

	// Stateless-retry rate tracking: per-source-IP Initial-packet counter.
	// Counters reset every second (epoch_start_ marks the start of the window).
	std::unordered_map<std::string, uint32_t> initial_pkt_count_;
	std::chrono::steady_clock::time_point epoch_start_{std::chrono::steady_clock::now()};

	// HMAC-SHA256 retry-token issuer (secret rotation, constant-time compare).
	// One per server instance so all tokens share the same secret pair.
	RetryTokenIssuer retry_token_issuer_;

	// Connections keyed by any active CID (raw bytes in a std::string). The
	// same QuicConnection may be registered under multiple keys — notably the
	// client's chosen original DCID and the server-generated SCID — so that
	// Initial-phase retransmits and post-handshake packets both route correctly.
	std::unordered_map<std::string, std::shared_ptr<QuicConnection>> connections_;

	// Outbound datagram queue.
	//
	// `do_write` (in QuicConnection) emits multiple datagrams in tight loops;
	// each one used to be sent via a detached coroutine that did
	// `co_await async_send_to`. That pattern collides with the kqueue / epoll
	// EVFILT_WRITE EV_ADD|EV_ONESHOT registration model: when several
	// `register_write_op` calls happen on the same fd before the first event
	// is delivered, the later EV_ADD overwrites the earlier ones in the
	// kernel's kevent table, orphaning the earlier coroutines and dropping
	// their datagrams. Symptom: server appears to send (ngtcp2 logs the
	// `pkt tx` events) but the client never receives anything past the first
	// flight.
	//
	// Fix: serialize sends through one drainer coroutine. `send_datagram`
	// pushes onto `pending_sends_`; if no drainer is running, it spawns one
	// that pops + `co_await`s `async_send_to` one at a time until empty.
	struct PendingSend
	{
		std::vector<uint8_t> buffer;
		net::UdpEndpoint     dest;
	};
	std::deque<PendingSend>          pending_sends_;
	bool                              send_drainer_running_ = false;
};

}  // namespace coroute::http3
