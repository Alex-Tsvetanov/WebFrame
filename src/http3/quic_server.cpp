// Server-side QUIC wiring.
//
// This implementation is inspired by the canonical ngtcp2 server example
// (https://github.com/ngtcp2/ngtcp2/blob/main/examples/server.cc, MIT)
// but is an independent adaptation: we drive ngtcp2 from an IoContext-based
// coroutine event loop instead of libev.

// ---------------------------------------------------------------------------
// Platform / crypto headers - kept out of public headers (see note in
// quic_server.hpp: winsock2's #define DELETE collides with HttpMethod::DELETE).
// ---------------------------------------------------------------------------
#ifdef _WIN32
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  ifndef NOMINMAX
#    define NOMINMAX
#  endif
#  include <winsock2.h>
#  include <ws2tcpip.h>
#else
#  include <arpa/inet.h>
#  include <netinet/in.h>
#  include <sys/socket.h>
#endif

// Belt-and-braces: prevent any leaked DELETE macro from winsock2 propagating.
#ifdef DELETE
#  undef DELETE
#endif

#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/hmac.h>
#include <openssl/rand.h>

#include <ngtcp2/ngtcp2.h>
#include <ngtcp2/ngtcp2_crypto.h>
#include <ngtcp2/ngtcp2_crypto_ossl.h>

#include <nghttp3/nghttp3.h>

#include "coroute/http3/quic_server.hpp"
#include "coroute/http3/quic_server_settings.hpp"
#include "coroute/http3/retry_token.hpp"
#include "coroute/net/tls.hpp"

#include <algorithm>
#include <chrono>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <mutex>
#include <utility>

static_assert(sizeof(sockaddr_storage) <= 128,
              "SockAddrStorage buffer too small for this platform's sockaddr_storage");

namespace coroute::http3
{

// ===========================================================================
// Internal helpers
// ===========================================================================

namespace
{

// ngtcp2 log_printf shim — gated on COROUTE_HTTP3_DEBUG env var so the
// server is quiet in normal runs but can be traced for integration debugging.
void ngtcp2_server_log_cb(void* /*user_data*/, const char* fmt, ...)
{
    static const bool enabled = []{
        const char* v = std::getenv("COROUTE_HTTP3_DEBUG");
        return v && *v != '\0' && *v != '0';
    }();
    if (!enabled) return;
    std::va_list ap;
    va_start(ap, fmt);
    std::fprintf(stderr, "[ngtcp2-srv] ");
    std::vfprintf(stderr, fmt, ap);
    std::fprintf(stderr, "\n");
    va_end(ap);
}

}  // namespace
namespace
{

std::once_flag g_ossl_init_once;
void ensure_crypto_initialised()
{
	std::call_once(g_ossl_init_once, [] { (void)ngtcp2_crypto_ossl_init(); });
}

ngtcp2_tstamp monotonic_nanos()
{
	using namespace std::chrono;
	return static_cast<ngtcp2_tstamp>(
	    duration_cast<nanoseconds>(steady_clock::now().time_since_epoch()).count());
}

void endpoint_to_sockaddr(const net::UdpEndpoint& ep, SockAddrStorage& out)
{
	std::memset(out.bytes.data(), 0, out.bytes.size());
	auto* sin = reinterpret_cast<sockaddr_in*>(out.bytes.data());
	sin->sin_family = AF_INET;
	sin->sin_port = htons(ep.port);
	if (!ep.address.empty() && inet_pton(AF_INET, ep.address.c_str(), &sin->sin_addr) != 1)
	{
		sin->sin_addr.s_addr = 0;
	}
	out.len = static_cast<int>(sizeof(sockaddr_in));
}

void init_wildcard_local(SockAddrStorage& out, uint16_t port)
{
	std::memset(out.bytes.data(), 0, out.bytes.size());
	auto* sin = reinterpret_cast<sockaddr_in*>(out.bytes.data());
	sin->sin_family = AF_INET;
	sin->sin_port = htons(port);
	sin->sin_addr.s_addr = htonl(INADDR_ANY);
	out.len = static_cast<int>(sizeof(sockaddr_in));
}

// Populate an ngtcp2_addr that points into a SockAddrStorage.
void addr_init_from_storage(ngtcp2_addr* addr, SockAddrStorage& s)
{
	addr->addr = reinterpret_cast<ngtcp2_sockaddr*>(s.bytes.data());
	addr->addrlen = static_cast<ngtcp2_socklen>(s.len);
}

// ALPN selector - accept only "h3".
int alpn_select_h3_cb(SSL* /*ssl*/, const unsigned char** out, unsigned char* outlen,
                      const unsigned char* in, unsigned int inlen, void* /*arg*/)
{
	// Walk the length-prefixed list and pick "h3".
	unsigned int i = 0;
	while (i + 1 <= inlen)
	{
		unsigned int n = in[i];
		if (i + 1 + n > inlen) break;
		if (n == 2 && in[i + 1] == 'h' && in[i + 2] == '3')
		{
			*out = &in[i + 1];
			*outlen = 2;
			return SSL_TLSEXT_ERR_OK;
		}
		i += 1 + n;
	}
	return SSL_TLSEXT_ERR_ALERT_FATAL;
}

// ---- ngtcp2 callback trampolines -----------------------------------------

QuicConnection* conn_from_ud(void* user_data)
{
	return static_cast<QuicConnection*>(user_data);
}

int cb_handshake_completed(ngtcp2_conn*, void* user_data)
{
	conn_from_ud(user_data)->on_handshake_completed();
	return 0;
}

int cb_recv_stream_data(ngtcp2_conn*, uint32_t flags, int64_t stream_id,
                        uint64_t /*offset*/, const uint8_t* data, size_t datalen,
                        void* user_data, void* /*stream_user_data*/)
{
	bool fin = (flags & NGTCP2_STREAM_DATA_FLAG_FIN) != 0;
	return conn_from_ud(user_data)->on_recv_stream_data(stream_id, data, datalen, fin, flags);
}

int cb_stream_open(ngtcp2_conn*, int64_t /*stream_id*/, void* /*user_data*/)
{
	return 0;
}

int cb_stream_close(ngtcp2_conn*, uint32_t flags, int64_t stream_id,
                    uint64_t app_error_code, void* user_data, void* /*stream_user_data*/)
{
	bool app = (flags & NGTCP2_STREAM_CLOSE_FLAG_APP_ERROR_CODE_SET) != 0;
	return conn_from_ud(user_data)->on_stream_close(stream_id, app_error_code, app);
}

int cb_stream_reset(ngtcp2_conn*, int64_t stream_id, uint64_t /*final_size*/,
                    uint64_t /*app_error_code*/, void* user_data, void*)
{
	return conn_from_ud(user_data)->on_stream_reset(stream_id);
}

int cb_stream_stop_sending(ngtcp2_conn*, int64_t stream_id, uint64_t /*app_error_code*/,
                           void* user_data, void*)
{
	return conn_from_ud(user_data)->on_stream_stop_sending(stream_id);
}

int cb_acked_stream_data_offset(ngtcp2_conn*, int64_t stream_id, uint64_t offset,
                                uint64_t datalen, void* user_data, void*)
{
	return conn_from_ud(user_data)->on_acked_stream_data_offset(stream_id, offset, datalen);
}

int cb_extend_max_stream_data(ngtcp2_conn*, int64_t stream_id, uint64_t max_data,
                              void* user_data, void*)
{
	return conn_from_ud(user_data)->on_extend_max_stream_data(stream_id, max_data);
}

void cb_rand(uint8_t* dest, size_t destlen, const ngtcp2_rand_ctx*)
{
	if (RAND_bytes(dest, static_cast<int>(destlen)) != 1)
	{
		for (size_t i = 0; i < destlen; ++i)
		{
			dest[i] = static_cast<uint8_t>(std::rand() & 0xFF);
		}
	}
}

int cb_get_new_connection_id(ngtcp2_conn*, ngtcp2_cid* cid, uint8_t* token,
                             size_t cidlen, void* user_data)
{
	cid->datalen = cidlen;
	if (RAND_bytes(cid->data, static_cast<int>(cidlen)) != 1)
		return NGTCP2_ERR_CALLBACK_FAILURE;
	if (RAND_bytes(token, NGTCP2_STATELESS_RESET_TOKENLEN) != 1)
		return NGTCP2_ERR_CALLBACK_FAILURE;

	// Register the new CID as an alias in the server's routing map so that
	// any subsequent packets from the peer addressed to this CID land at the
	// same QuicConnection. Without this, ngtcp2 advertises a NEW_CONNECTION_ID
	// the peer may start using, but packets to it would miss and get
	// rejected as spurious Initials.
	if (user_data)
	{
		auto* self = static_cast<QuicConnection*>(user_data);
		if (auto* server = self->server())
		{
			std::string key(reinterpret_cast<const char*>(cid->data), cid->datalen);
			// Find the existing shared_ptr for this connection so we can add
			// another entry to the routing map pointing at it.
			server->track_additional_cid(self, std::move(key));
		}
	}
	return 0;
}

// ---- QUIC datagram callbacks (RFC 9221) -----------------------------------

int cb_recv_datagram(ngtcp2_conn*, uint32_t /*flags*/,
                     const uint8_t* data, size_t datalen, void* user_data)
{
	conn_from_ud(user_data)->on_recv_datagram(data, datalen);
	return 0;
}

int cb_ack_datagram(ngtcp2_conn*, uint64_t /*dgram_id*/, void* /*user_data*/)
{
	return 0;  // Datagrams are unreliable; nothing to do on ack.
}

int cb_lost_datagram(ngtcp2_conn*, uint64_t /*dgram_id*/, void* /*user_data*/)
{
	return 0;  // Datagrams are unreliable; nothing to do on loss.
}

}  // anonymous namespace

// ===========================================================================
// QuicConnection::ConnRefHolder
// ===========================================================================
struct QuicConnection::ConnRefHolder
{
	ngtcp2_crypto_conn_ref ref{};
};

// ===========================================================================
// QuicServer
// ===========================================================================

QuicServer::QuicServer(net::IoContext& ctx, net::TlsContext* tls_ctx)
    : ctx_(ctx), tls_ctx_(tls_ctx)
{
	ensure_crypto_initialised();
}

QuicServer::~QuicServer()
{
	stopped_ = true;
}

expected<void, Error> QuicServer::listen(uint16_t port)
{
	if (!tls_ctx_)
	{
		return unexpected(Error::io(IoError::InvalidArgument,
		                            "HTTP/3 requires a TLS context"));
	}

	auto sock_result = ctx_.bind_udp(port);
	if (!sock_result) return unexpected(sock_result.error());
	socket_ = std::move(*sock_result);
	// Capture the actual bound port (matters when caller passes 0 for ephemeral).
	port_ = socket_->local_port();

	// Configure ALPN "h3" selection on the shared SSL_CTX.
	SSL_CTX* sslctx = tls_ctx_->native_handle();
	if (sslctx)
	{
		SSL_CTX_set_alpn_select_cb(sslctx, alpn_select_h3_cb, nullptr);
	}

	return {};
}

Task<void> QuicServer::run()
{
	if (!socket_) co_return;

	std::vector<uint8_t> buffer(65535);
	while (!stopped_)
	{
		auto res = co_await socket_->async_recv_from(buffer.data(), buffer.size());
		if (!res) continue;  // transient UDP errors are non-fatal

		auto [len, peer] = *res;
		if (len == 0) continue;

		ngtcp2_version_cid vc{};
		int rv = ngtcp2_pkt_decode_version_cid(&vc, buffer.data(), len,
		                                       NGTCP2_MIN_INITIAL_DCIDLEN);
		if (rv < 0) continue;  // unparseable QUIC packet

		// Routing key: the packet's DCID. For client Initial-phase packets
		// this is the client's chosen value; after the client switches to
		// the server's advertised SCID it becomes that. We register the
		// connection under both — see register_connection / add_cid_alias.
		std::string key(reinterpret_cast<const char*>(vc.dcid), vc.dcidlen);
		auto it = connections_.find(key);
		if (it == connections_.end())
		{
			// New connection: validate as Initial.
			ngtcp2_pkt_hd hd{};
			if (ngtcp2_accept(&hd, buffer.data(), len) != 0) continue;

			// Stateless retry (RFC 9000 §8.1): if this source IP has sent
			// more Initial packets than the threshold within the current
			// one-second window, challenge it with a Retry packet instead
			// of allocating connection state.  ngtcp2_pkt_write_retry
			// writes a syntactically valid Retry packet containing a
			// server-minted token; the client must loop it back in a
			// subsequent Initial before we proceed.
			if (should_send_retry(peer))
			{
				// Build an HMAC-SHA256 retry token (73 bytes) that binds the
				// client source address and the original DCID to a rotating
				// server secret. The client must echo this token back in its
				// subsequent Initial; RetryTokenIssuer validates it statelessly.
				const std::string client_ep =
				    peer.address + ":" + std::to_string(peer.port);
				const auto hmac_token =
				    retry_token_issuer_.issue(vc.dcid, vc.dcidlen, client_ep);

				// An all-zero token indicates HMAC failure (OpenSSL / RAND_bytes
				// error). Skip the Retry rather than send an unverifiable token.
				const bool token_ok =
				    std::any_of(hmac_token.begin(), hmac_token.end(),
				                [](uint8_t b){ return b != 0u; });
				if (!token_ok) continue;

				// Server-chosen SCID for the Retry packet.
				ngtcp2_cid retry_scid{};
				retry_scid.datalen = 8;
				RAND_bytes(retry_scid.data, static_cast<int>(retry_scid.datalen));

				ngtcp2_cid orig_dcid{};
				orig_dcid.datalen = vc.dcidlen;
				std::memcpy(orig_dcid.data, vc.dcid, vc.dcidlen);

				ngtcp2_cid client_scid_cid{};
				if (vc.scid && vc.scidlen)
				{
					client_scid_cid.datalen = vc.scidlen;
					std::memcpy(client_scid_cid.data, vc.scid, vc.scidlen);
				}

				std::vector<uint8_t> retry_buf(NGTCP2_MAX_UDP_PAYLOAD_SIZE);
				// ngtcp2_crypto_write_retry handles the AEAD envelope
				// (AEAD_AES_128_GCM with NGTCP2_RETRY_KEY) required by
				// RFC 9000 §8.1.4, wrapping ngtcp2_pkt_write_retry.
				ngtcp2_ssize rlen = ngtcp2_crypto_write_retry(
				    retry_buf.data(), retry_buf.size(),
				    vc.version,
				    &client_scid_cid,        // destination (client's SCID)
				    &retry_scid,              // source (new server CID)
				    &orig_dcid,               // original DCID
				    hmac_token.data(), hmac_token.size());
				if (rlen > 0)
				{
					retry_buf.resize(static_cast<std::size_t>(rlen));
					send_datagram(std::move(retry_buf), peer);
				}
				continue;
			}

			auto conn = std::make_shared<QuicConnection>(
			    this, peer, vc.dcid, vc.dcidlen, vc.scid, vc.scidlen, vc.version);
			if (!conn->initialised())
			{
				std::cerr << "HTTP/3: failed to create QuicConnection" << std::endl;
				continue;
			}

			// Register under both CIDs — the client keeps using the original
			// DCID on Initial-phase retransmits, then switches to the
			// server-generated SCID after its first server-reply packet.
			std::string scid_key = conn->cid_key();
			register_connection(key, conn);
			if (scid_key != key) add_cid_alias(scid_key, conn);
			conn->on_datagram(buffer.data(), len, peer);
		}
		else
		{
			auto conn = it->second;  // copy shared_ptr to keep alive across drop_connection
			conn->on_datagram(buffer.data(), len, peer);
			if (conn->closed())
			{
				drop_connection(key);
			}
		}
	}
}

void QuicServer::send_datagram(std::vector<uint8_t> buffer, const net::UdpEndpoint& peer)
{
	if (!socket_ || buffer.empty()) return;

	// Push onto the per-server send queue and ensure exactly one drainer
	// coroutine is running. Multiple parallel detached sends would race on
	// the kqueue / epoll EVFILT_WRITE `EV_ADD | EV_ONESHOT` registration
	// table: the second EV_ADD overwrites the first, orphaning the first
	// coroutine and dropping its datagram. The previous design (a fresh
	// detached coroutine per datagram) hit this directly and surfaced as
	// "server's first flight gets ACKd but ServerHello + Handshake CRYPTO
	// never reach the client" — the in-process E2E test stalled at the
	// QUIC anti-amplification limit. Serializing through one drainer keeps
	// at most one async_send_to in flight at a time.
	pending_sends_.push_back({std::move(buffer), peer});

	if (send_drainer_running_) return;
	send_drainer_running_ = true;

	[this]() -> Task<void>
	{
		// Drain until empty. New send_datagram calls during a co_await will
		// just append to pending_sends_ and rely on this drainer to keep going.
		while (!pending_sends_.empty())
		{
			PendingSend ps = std::move(pending_sends_.front());
			pending_sends_.pop_front();
			if (!socket_) break;
			(void) co_await socket_->async_send_to(ps.buffer.data(),
			                                         ps.buffer.size(), ps.dest);
		}
		send_drainer_running_ = false;
		co_return;
	}()
	    .start_detached();
}

void QuicServer::drop_connection(const std::string& cid_key)
{
	// A single QuicConnection may be registered under multiple CIDs (original
	// DCID + server-generated SCID). When one closes, erase every map entry
	// that points at the same underlying connection.
	auto it = connections_.find(cid_key);
	if (it == connections_.end()) return;
	QuicConnection* target = it->second.get();
	for (auto i = connections_.begin(); i != connections_.end(); )
	{
		if (i->second.get() == target)
			i = connections_.erase(i);
		else
			++i;
	}
}

void QuicServer::register_connection(std::string cid_key,
                                     std::shared_ptr<QuicConnection> c)
{
	connections_[std::move(cid_key)] = std::move(c);
}

void QuicServer::add_cid_alias(std::string cid_key,
                               const std::shared_ptr<QuicConnection>& c)
{
	connections_[std::move(cid_key)] = c;
}

void QuicServer::track_additional_cid(QuicConnection* conn, std::string cid_key)
{
	// Find any existing shared_ptr whose raw pointer matches `conn`, and
	// register the same shared_ptr under the new cid_key.
	for (auto& [k, v] : connections_)
	{
		if (v.get() == conn)
		{
			connections_[std::move(cid_key)] = v;
			return;
		}
	}
}

bool QuicServer::should_send_retry(const net::UdpEndpoint& peer)
{
	if (!settings_.enable_stateless_retry) return false;

	const auto now = std::chrono::steady_clock::now();
	// Reset counters once per second.
	if (now - epoch_start_ >= std::chrono::seconds{1})
	{
		initial_pkt_count_.clear();
		epoch_start_ = now;
	}

	uint32_t& count = initial_pkt_count_[peer.address];
	++count;
	return count > settings_.stateless_retry_threshold;
}

// ===========================================================================
// QuicConnection
// ===========================================================================

QuicConnection::QuicConnection(QuicServer* server,
                               const net::UdpEndpoint& peer,
                               const uint8_t* dcid, size_t dcid_len,
                               const uint8_t* scid, size_t scid_len,
                               uint32_t client_chosen_version)
    : server_(server), peer_endpoint_(peer),
      conn_ref_holder_(std::make_unique<ConnRefHolder>())
{
	// Defaults in case we fail early.
	init_wildcard_local(local_addr_, server_->port());
	endpoint_to_sockaddr(peer, peer_addr_);

	// ---- Server-chosen SCID (random) ---------------------------------------
	// 8-byte server CIDs: matches NGTCP2_MIN_INITIAL_DCIDLEN used by the
	// server's recv loop when decoding short-header packets. If this length
	// changes, the second argument to ngtcp2_pkt_decode_version_cid in
	// QuicServer::run() must change in lockstep.
	ngtcp2_cid server_scid{};
	server_scid.datalen = 8;
	if (RAND_bytes(server_scid.data, static_cast<int>(server_scid.datalen)) != 1)
	{
		std::cerr << "HTTP/3: RAND_bytes failed for SCID" << std::endl;
		return;
	}
	cid_key_.assign(reinterpret_cast<const char*>(server_scid.data),
	                server_scid.datalen);

	ngtcp2_cid orig_dcid{};
	orig_dcid.datalen = dcid_len;
	std::memcpy(orig_dcid.data, dcid, dcid_len);

	ngtcp2_cid client_scid{};
	client_scid.datalen = scid_len;
	if (scid && scid_len) std::memcpy(client_scid.data, scid, scid_len);

	// ---- ngtcp2_callbacks --------------------------------------------------
	ngtcp2_callbacks callbacks{};
	// Crypto callbacks provided by the ngtcp2 OSSL helper library.
	callbacks.recv_client_initial = ngtcp2_crypto_recv_client_initial_cb;
	callbacks.recv_crypto_data = ngtcp2_crypto_recv_crypto_data_cb;
	callbacks.encrypt = ngtcp2_crypto_encrypt_cb;
	callbacks.decrypt = ngtcp2_crypto_decrypt_cb;
	callbacks.hp_mask = ngtcp2_crypto_hp_mask_cb;
	callbacks.update_key = ngtcp2_crypto_update_key_cb;
	callbacks.delete_crypto_aead_ctx = ngtcp2_crypto_delete_crypto_aead_ctx_cb;
	callbacks.delete_crypto_cipher_ctx = ngtcp2_crypto_delete_crypto_cipher_ctx_cb;
	callbacks.get_path_challenge_data = ngtcp2_crypto_get_path_challenge_data_cb;
	callbacks.version_negotiation = ngtcp2_crypto_version_negotiation_cb;

	// App callbacks.
	callbacks.handshake_completed = cb_handshake_completed;
	callbacks.recv_stream_data = cb_recv_stream_data;
	callbacks.stream_open = cb_stream_open;
	callbacks.stream_close = cb_stream_close;
	callbacks.stream_reset = cb_stream_reset;
	callbacks.stream_stop_sending = cb_stream_stop_sending;
	callbacks.acked_stream_data_offset = cb_acked_stream_data_offset;
	callbacks.extend_max_stream_data = cb_extend_max_stream_data;
	callbacks.rand = cb_rand;
	callbacks.get_new_connection_id = cb_get_new_connection_id;
	callbacks.recv_datagram = cb_recv_datagram;
	callbacks.ack_datagram  = cb_ack_datagram;
	callbacks.lost_datagram = cb_lost_datagram;

	// Reject the connection immediately if there's no TLS context. The server's
	// listen() already gates on this, but QuicConnection can be constructed
	// directly in tests — guard defensively so conn_ stays nullptr.
	if (!server_->tls_context())
	{
		return;
	}

	// ---- Settings + transport params --------------------------------------
	ngtcp2_settings settings{};
	ngtcp2_settings_default(&settings);
	settings.initial_ts = monotonic_nanos();
	settings.max_tx_udp_payload_size = NGTCP2_MAX_UDP_PAYLOAD_SIZE;
	settings.log_printf = ngtcp2_server_log_cb;

	ngtcp2_transport_params params{};
	ngtcp2_transport_params_default(&params);
	params.initial_max_stream_data_bidi_local = 256 * 1024;
	params.initial_max_stream_data_bidi_remote = 256 * 1024;
	params.initial_max_stream_data_uni = 256 * 1024;
	params.initial_max_data = 1024 * 1024;
	params.initial_max_streams_bidi = 100;
	params.initial_max_streams_uni = 3;
	params.max_idle_timeout = 30ULL * NGTCP2_SECONDS;
	params.max_datagram_frame_size = 65535;  // Enable RFC 9221 QUIC datagrams.
	params.original_dcid = orig_dcid;
	params.original_dcid_present = 1;

	// Preferred address (RFC 9000 §9.6): advertise an alternative server
	// address in the transport parameters.  After the handshake the client
	// MAY migrate to this address; ngtcp2 handles the client-side path
	// validation transparently.  Only populated when the caller has configured
	// one via Http3ServerSettings::preferred_address.
	if (server_)
	{
		const auto& srv_settings = server_->settings();
		if (srv_settings.preferred_address.has_value())
		{
			const auto& pa = *srv_settings.preferred_address;
			params.preferred_addr_present = 1;
			std::memset(&params.preferred_addr, 0, sizeof(params.preferred_addr));

			if (!pa.ipv4_addr.empty() && pa.ipv4_port != 0)
			{
				struct in_addr in4{};
				if (inet_pton(AF_INET, pa.ipv4_addr.c_str(), &in4) == 1)
				{
					params.preferred_addr.ipv4.sin_family = AF_INET;
					params.preferred_addr.ipv4.sin_addr   = in4;
					params.preferred_addr.ipv4.sin_port   = htons(pa.ipv4_port);
					params.preferred_addr.ipv4_present     = 1;
				}
			}

			if (!pa.ipv6_addr.empty() && pa.ipv6_port != 0)
			{
				struct in6_addr in6{};
				if (inet_pton(AF_INET6, pa.ipv6_addr.c_str(), &in6) == 1)
				{
					params.preferred_addr.ipv6.sin6_family = AF_INET6;
					params.preferred_addr.ipv6.sin6_addr   = in6;
					params.preferred_addr.ipv6.sin6_port   = htons(pa.ipv6_port);
					params.preferred_addr.ipv6_present      = 1;
				}
			}

			// Preferred-address CID: a freshly-minted random CID the client
			// will use when sending to the preferred address.
			params.preferred_addr.cid.datalen = 8;
			RAND_bytes(params.preferred_addr.cid.data,
			           static_cast<int>(params.preferred_addr.cid.datalen));
			RAND_bytes(params.preferred_addr.stateless_reset_token,
			           NGTCP2_STATELESS_RESET_TOKENLEN);
		}
	}

	// ---- Path (local + remote sockaddr pointers) --------------------------
	ngtcp2_path path{};
	addr_init_from_storage(&path.local, local_addr_);
	addr_init_from_storage(&path.remote, peer_addr_);

	// ---- Create ngtcp2_conn -----------------------------------------------
	int rv = ngtcp2_conn_server_new(&conn_, &client_scid, &server_scid, &path,
	                                 client_chosen_version, &callbacks, &settings,
	                                 &params, nullptr, this);
	if (rv != 0)
	{
		std::cerr << "HTTP/3: ngtcp2_conn_server_new failed: "
		          << ngtcp2_strerror(rv) << std::endl;
		conn_ = nullptr;
		return;
	}

	// ---- TLS / SSL setup --------------------------------------------------
	SSL_CTX* ssl_ctx = server_->tls_context()->native_handle();
	ssl_ = SSL_new(ssl_ctx);
	if (!ssl_)
	{
		std::cerr << "HTTP/3: SSL_new failed" << std::endl;
		ngtcp2_conn_del(conn_);
		conn_ = nullptr;
		return;
	}

	// Hook up the OSSL crypto backend.
	if (ngtcp2_crypto_ossl_ctx_new(&ossl_ctx_, ssl_) != 0)
	{
		std::cerr << "HTTP/3: ngtcp2_crypto_ossl_ctx_new failed" << std::endl;
		SSL_free(ssl_);
		ssl_ = nullptr;
		ngtcp2_conn_del(conn_);
		conn_ = nullptr;
		return;
	}

	// Mark the SSL as server side BEFORE configure_server_session so the QUIC
	// callbacks installed by ngtcp2 operate on a session in the correct role.
	SSL_set_accept_state(ssl_);

	if (ngtcp2_crypto_ossl_configure_server_session(ssl_) != 0)
	{
		std::cerr << "HTTP/3: configure_server_session failed" << std::endl;
		ngtcp2_crypto_ossl_ctx_del(ossl_ctx_);
		ossl_ctx_ = nullptr;
		SSL_free(ssl_);
		ssl_ = nullptr;
		ngtcp2_conn_del(conn_);
		conn_ = nullptr;
		return;
	}

	// Wire the conn_ref so the OSSL callbacks can walk SSL -> ngtcp2_conn.
	conn_ref_holder_->ref.get_conn = [](ngtcp2_crypto_conn_ref* ref) -> ngtcp2_conn*
	{
		auto* self = static_cast<QuicConnection*>(ref->user_data);
		return self ? self->conn_ : nullptr;
	};
	conn_ref_holder_->ref.user_data = this;
	SSL_set_app_data(ssl_, &conn_ref_holder_->ref);

	// Give ngtcp2 ownership of the TLS handle.
	ngtcp2_conn_set_tls_native_handle(conn_, ossl_ctx_);
}

QuicConnection::~QuicConnection()
{
	if (ssl_)
	{
		// Per ngtcp2 docs: must clear app_data before SSL_free if the
		// ngtcp2_conn might be gone.
		SSL_set_app_data(ssl_, nullptr);
	}
	if (ossl_ctx_)
	{
		ngtcp2_crypto_ossl_ctx_del(ossl_ctx_);
		ossl_ctx_ = nullptr;
	}
	if (ssl_)
	{
		SSL_free(ssl_);
		ssl_ = nullptr;
	}
	if (conn_)
	{
		ngtcp2_conn_del(conn_);
		conn_ = nullptr;
	}
}

void QuicConnection::on_datagram(const uint8_t* data, size_t datalen,
                                 const net::UdpEndpoint& peer)
{
	if (!conn_ || closed_) return;

	// Refresh peer addr in case it changed (migration — we tolerate it).
	peer_endpoint_ = peer;
	endpoint_to_sockaddr(peer, peer_addr_);

	ngtcp2_path path{};
	addr_init_from_storage(&path.local, local_addr_);
	addr_init_from_storage(&path.remote, peer_addr_);

	ngtcp2_pkt_info pi{};
	int rv = ngtcp2_conn_read_pkt(conn_, &path, &pi, data, datalen, monotonic_nanos());
	if (rv != 0)
	{
		if (rv == NGTCP2_ERR_DRAINING || rv == NGTCP2_ERR_CLOSING)
		{
			closed_ = true;
		}
		else
		{
			std::cerr << "HTTP/3: ngtcp2_conn_read_pkt error: "
			          << ngtcp2_strerror(rv) << std::endl;
			if (rv == NGTCP2_ERR_CRYPTO)
			{
				unsigned long err;
				while ((err = ERR_get_error()) != 0)
				{
					char buf[256];
					ERR_error_string_n(err, buf, sizeof(buf));
					std::cerr << "  OpenSSL: " << buf << std::endl;
				}
			}
			closed_ = true;
		}
		return;
	}
	do_write();
}

void QuicConnection::schedule_write()
{
	if (!conn_ || closed_ || write_scheduled_) return;
	write_scheduled_ = true;
	server_->io_context().post([this]
	{
		write_scheduled_ = false;
		if (!closed_) do_write();
	});
}

void QuicConnection::do_write()
{
	if (!conn_ || closed_) return;

	std::vector<uint8_t> pkt(NGTCP2_MAX_UDP_PAYLOAD_SIZE);
	nghttp3_conn* h3 = http3_ ? http3_->http3_conn() : nullptr;

	ngtcp2_path_storage ps;
	ngtcp2_path_storage_zero(&ps);
	ngtcp2_pkt_info pi{};

	for (;;)
	{
		int64_t stream_id = -1;
		int fin = 0;
		nghttp3_vec h3vec[16]{};
		nghttp3_ssize h3cnt = 0;

		if (h3)
		{
			h3cnt = nghttp3_conn_writev_stream(h3, &stream_id, &fin, h3vec, 16);
			if (h3cnt < 0)
			{
				std::cerr << "HTTP/3: nghttp3_conn_writev_stream error: "
				          << nghttp3_strerror(static_cast<int>(h3cnt)) << std::endl;
				closed_ = true;
				return;
			}
		}

		ngtcp2_ssize pdatalen = 0;
		uint32_t flags = NGTCP2_WRITE_STREAM_FLAG_MORE;
		if (fin) flags |= NGTCP2_WRITE_STREAM_FLAG_FIN;

		ngtcp2_ssize nwrite = ngtcp2_conn_writev_stream(
		    conn_, &ps.path, &pi, pkt.data(), pkt.size(), &pdatalen, flags,
		    stream_id,
		    reinterpret_cast<const ngtcp2_vec*>(h3vec),
		    static_cast<size_t>(h3cnt), monotonic_nanos());

		if (nwrite < 0)
		{
			switch (nwrite)
			{
				case NGTCP2_ERR_WRITE_MORE:
					// More room in packet; inform nghttp3 of bytes packed so far.
					if (h3 && pdatalen >= 0 && stream_id >= 0)
					{
						nghttp3_conn_add_write_offset(h3, stream_id,
						                              static_cast<size_t>(pdatalen));
					}
					continue;
				case NGTCP2_ERR_STREAM_DATA_BLOCKED:
				case NGTCP2_ERR_STREAM_SHUT_WR:
					if (h3 && stream_id >= 0)
					{
						// Tell nghttp3 we accepted 0 bytes so it can move on.
						nghttp3_conn_add_write_offset(h3, stream_id, 0);
					}
					continue;
				case NGTCP2_ERR_STREAM_NOT_FOUND:
					if (h3 && stream_id >= 0)
					{
						nghttp3_conn_add_write_offset(h3, stream_id, 0);
					}
					continue;
				default:
					std::cerr << "HTTP/3: ngtcp2_conn_writev_stream error: "
					          << ngtcp2_strerror(static_cast<int>(nwrite)) << std::endl;
					closed_ = true;
					return;
			}
		}

		// nwrite >= 0 here.
		if (h3 && pdatalen >= 0 && stream_id >= 0)
		{
			int add_rv = nghttp3_conn_add_write_offset(h3, stream_id,
			                                           static_cast<size_t>(pdatalen));
			if (add_rv != 0)
			{
				std::cerr << "HTTP/3: nghttp3_conn_add_write_offset error: "
				          << nghttp3_strerror(add_rv) << std::endl;
				closed_ = true;
				return;
			}
		}

		if (nwrite == 0)
		{
			// Nothing left to send right now.
			break;
		}

		// Copy the packet out and enqueue for transmission.
		std::vector<uint8_t> out(pkt.data(), pkt.data() + nwrite);
		server_->send_datagram(std::move(out), peer_endpoint_);

		// Reset the packet_info for the next iteration.
		pi = ngtcp2_pkt_info{};
	}

	arm_timer();

	if (ngtcp2_conn_in_closing_period(conn_) || ngtcp2_conn_in_draining_period(conn_))
	{
		closed_ = true;
	}
}

void QuicConnection::arm_timer()
{
	if (!conn_ || closed_) return;
	ngtcp2_tstamp expiry = ngtcp2_conn_get_expiry(conn_);
	ngtcp2_tstamp now = monotonic_nanos();

	std::chrono::milliseconds delay{0};
	if (expiry > now)
	{
		uint64_t diff_ns = expiry - now;
		delay = std::chrono::milliseconds(diff_ns / 1000000ULL);
	}

	// Bump generation so any previously scheduled fire is ignored.
	uint64_t my_gen = ++timer_generation_;
	server_->io_context().schedule(delay, [this, my_gen]
	{
		if (closed_) return;
		if (my_gen != timer_generation_) return;  // stale
		if (!conn_) return;
		int rv = ngtcp2_conn_handle_expiry(conn_, monotonic_nanos());
		if (rv != 0)
		{
			closed_ = true;
			return;
		}
		do_write();
	});
}

// ----- ngtcp2 callback sinks ----------------------------------------------

void QuicConnection::on_handshake_completed()
{
	if (handshake_completed_) return;
	handshake_completed_ = true;

	// Apply keepalive PING timeout from server settings.  ngtcp2 will
	// automatically emit a PING frame when the connection is idle for this
	// interval, keeping the path through NAT devices alive.  A value of 0
	// disables the mechanism entirely (ngtcp2 treats 0 as "no keepalive").
	if (conn_ && server_)
	{
		const auto& srv_settings = server_->settings();
		if (srv_settings.keep_alive_timeout.count() > 0)
		{
			const ngtcp2_tstamp timeout_ns = static_cast<ngtcp2_tstamp>(
			    srv_settings.keep_alive_timeout.count()) * NGTCP2_SECONDS;
			ngtcp2_conn_set_keep_alive_timeout(conn_, timeout_ns);
		}
	}

	http3_ = std::make_unique<Http3Connection>(this);
	int rv = http3_->setup_server_streams();
	if (rv != 0)
	{
		std::cerr << "HTTP/3: Http3Connection setup failed: "
		          << nghttp3_strerror(rv) << std::endl;
		closed_ = true;
		return;
	}
}

int QuicConnection::on_recv_stream_data(int64_t stream_id, const uint8_t* data,
                                        size_t len, bool fin, uint32_t flags)
{
	if (!http3_) return 0;
	const bool in_0rtt = (flags & NGTCP2_STREAM_DATA_FLAG_0RTT) != 0;
	long long nconsumed = http3_->on_stream_data(stream_id, data, len, fin, in_0rtt);
	if (nconsumed < 0)
	{
		return NGTCP2_ERR_CALLBACK_FAILURE;
	}
	ngtcp2_conn_extend_max_stream_offset(conn_, stream_id,
	                                      static_cast<uint64_t>(nconsumed));
	ngtcp2_conn_extend_max_offset(conn_, static_cast<uint64_t>(nconsumed));
	return 0;
}

int QuicConnection::on_stream_close(int64_t stream_id, uint64_t app_error_code, bool /*app*/)
{
	if (http3_)
	{
		int rv = nghttp3_conn_close_stream(http3_->http3_conn(), stream_id, app_error_code);
		if (rv != 0 && rv != NGHTTP3_ERR_STREAM_NOT_FOUND)
		{
			return NGTCP2_ERR_CALLBACK_FAILURE;
		}
	}
	return 0;
}

int QuicConnection::on_stream_reset(int64_t stream_id)
{
	if (http3_)
	{
		int rv = nghttp3_conn_shutdown_stream_read(http3_->http3_conn(), stream_id);
		if (rv != 0 && rv != NGHTTP3_ERR_STREAM_NOT_FOUND)
		{
			return NGTCP2_ERR_CALLBACK_FAILURE;
		}
	}
	return 0;
}

int QuicConnection::on_stream_stop_sending(int64_t /*stream_id*/)
{
	return 0;
}

int QuicConnection::on_acked_stream_data_offset(int64_t /*stream_id*/, uint64_t /*offset*/,
                                                uint64_t /*datalen*/)
{
	return 0;
}

int QuicConnection::on_extend_max_stream_data(int64_t stream_id, uint64_t /*max_data*/)
{
	if (http3_)
	{
		nghttp3_conn_unblock_stream(http3_->http3_conn(), stream_id);
	}
	return 0;
}

// ---------------------------------------------------------------------------
// QUIC datagram support (RFC 9221)
// ---------------------------------------------------------------------------

void QuicConnection::on_recv_datagram(const uint8_t* data, size_t datalen)
{
	if (inbound_datagrams_.size() >= kMaxInboundDatagramQueue)
		inbound_datagrams_.pop_front();  // drop oldest — datagrams are unreliable

	inbound_datagrams_.emplace_back(data, data + datalen);

	// Resume a coroutine waiting in next_datagram(), if any.
	if (datagram_waiter_)
	{
		auto h = std::exchange(datagram_waiter_, {});
		h.resume();
	}
}

expected<void, Error> QuicConnection::send_datagram(std::span<const uint8_t> data)
{
	if (!conn_ || closed_)
		return unexpected(Error::io(IoError::InvalidArgument, "connection not open"));

	// Verify the peer advertised datagram support.
	const ngtcp2_transport_params* remote = ngtcp2_conn_get_remote_transport_params(conn_);
	if (!remote || remote->max_datagram_frame_size == 0)
		return unexpected(Error::io(IoError::InvalidArgument,
		                            "peer did not negotiate QUIC datagram support"));

	std::vector<uint8_t> pkt(NGTCP2_MAX_UDP_PAYLOAD_SIZE);
	ngtcp2_path_storage ps;
	ngtcp2_path_storage_zero(&ps);
	ngtcp2_pkt_info pi{};
	int accepted = 0;

	ngtcp2_ssize nwrite = ngtcp2_conn_write_datagram(
	    conn_, &ps.path, &pi,
	    pkt.data(), pkt.size(),
	    &accepted,
	    NGTCP2_WRITE_DATAGRAM_FLAG_NONE,
	    /*dgram_id=*/0,
	    data.data(), data.size(),
	    monotonic_nanos());

	if (nwrite < 0)
	{
		return unexpected(Error::io(IoError::Unknown,
		                            ngtcp2_strerror(static_cast<int>(nwrite))));
	}

	if (nwrite > 0 && accepted)
	{
		pkt.resize(static_cast<std::size_t>(nwrite));
		server_->send_datagram(std::move(pkt), peer_endpoint_);
	}
	return {};
}

Task<std::optional<std::vector<uint8_t>>> QuicConnection::next_datagram()
{
	// Suspend until an inbound datagram is available or the connection closes.
	// The awaitable is defined inline as a lambda-like struct so it can access
	// private members through the enclosing member function context.
	struct DatagramAwaiter
	{
		QuicConnection& self;
		bool await_ready() const noexcept
		{
			return !self.inbound_datagrams_.empty() || self.closed_;
		}
		void await_suspend(std::coroutine_handle<> h) noexcept
		{
			self.datagram_waiter_ = h;
		}
		void await_resume() const noexcept {}
	};

	if (inbound_datagrams_.empty())
	{
		if (closed_) co_return std::nullopt;
		co_await DatagramAwaiter{*this};
		if (inbound_datagrams_.empty()) co_return std::nullopt;
	}
	auto dg = std::move(inbound_datagrams_.front());
	inbound_datagrams_.pop_front();
	co_return dg;
}

}  // namespace coroute::http3
