// QUIC + HTTP/3 in-process test client implementation.
//
// Adapted patterns from the upstream ngtcp2 example client (MIT licensed):
//   https://github.com/ngtcp2/ngtcp2/blob/v1.21.0/examples/client.cc
// Heavily trimmed to the unit-test subset: no 0-RTT, session resumption,
// migration, path validation, preferred_address, or retry.

#include "quic_test_client.hpp"

#include "coroute/net/io_context.hpp"
#include "coroute/coro/task.hpp"
#include "coroute/core/error.hpp"
#include "coroute/util/expected.hpp"

// Keep platform / openssl / ngtcp2 includes OUT of the public header to avoid
// winsock2's #define DELETE colliding with coroute::HttpMethod::DELETE.
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
#  include <sys/socket.h>
#  include <netinet/in.h>
#  include <arpa/inet.h>
#endif

#include <openssl/err.h>
#include <openssl/ssl.h>

#include <ngtcp2/ngtcp2.h>
#include <ngtcp2/ngtcp2_crypto.h>
#include <ngtcp2/ngtcp2_crypto_ossl.h>
#include <nghttp3/nghttp3.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <random>
#include <string>
#include <vector>

// Restore any macro collisions winsock2 may have inflicted. winsock2.h
// #defines DELETE to a numeric value that would poison anything included
// AFTER this translation unit within the same header cascade — but since
// this is a .cpp (final TU boundary) we only need to be careful to never
// include coroute headers that define HttpMethod::DELETE BELOW this point.
#ifdef DELETE
#  undef DELETE
#endif
#ifdef ERROR
#  undef ERROR
#endif

namespace coroute::http3::test
{

namespace {

// ---------------------------------------------------------------------------
// Debug tracing — gated on COROUTE_HTTP3_DEBUG env var.
// ---------------------------------------------------------------------------
bool dbg_enabled()
{
    static const bool v = []{
        const char* s = std::getenv("COROUTE_HTTP3_DEBUG");
        return s && *s != '\0' && *s != '0';
    }();
    return v;
}

void ngtcp2_client_log_cb(void* /*user_data*/, const char* fmt, ...)
{
    if (!dbg_enabled()) return;
    std::va_list ap;
    va_start(ap, fmt);
    std::fprintf(stderr, "[ngtcp2-cli] ");
    std::vfprintf(stderr, fmt, ap);
    std::fprintf(stderr, "\n");
    va_end(ap);
}

// ---------------------------------------------------------------------------
// One-time ngtcp2_crypto_ossl / OpenSSL init.
// ---------------------------------------------------------------------------
std::once_flag g_ossl_init_once;
bool g_ossl_init_ok = false;

void ensure_ossl_init()
{
    std::call_once(g_ossl_init_once, [] {
        if (ngtcp2_crypto_ossl_init() == 0) {
            g_ossl_init_ok = true;
        }
    });
}

// ---------------------------------------------------------------------------
// Random helpers
// ---------------------------------------------------------------------------
void fill_random(uint8_t* out, size_t len)
{
    static thread_local std::mt19937_64 rng{std::random_device{}()};
    size_t i = 0;
    while (i + sizeof(uint64_t) <= len) {
        uint64_t v = rng();
        std::memcpy(out + i, &v, sizeof(uint64_t));
        i += sizeof(uint64_t);
    }
    while (i < len) {
        out[i++] = static_cast<uint8_t>(rng());
    }
}

// Monotonic timestamp in nanoseconds (ngtcp2 uses nanosecond timestamps).
ngtcp2_tstamp now_ns()
{
    using namespace std::chrono;
    return static_cast<ngtcp2_tstamp>(
        duration_cast<nanoseconds>(steady_clock::now().time_since_epoch()).count());
}

// IPv4 string -> sockaddr_in. We only support 127.0.0.1 style literals for
// the in-process test client.
bool parse_ipv4(const std::string& host, uint16_t port, sockaddr_in& out)
{
    std::memset(&out, 0, sizeof(out));
    out.sin_family = AF_INET;
    out.sin_port = htons(port);
    if (::inet_pton(AF_INET, host.c_str(), &out.sin_addr) != 1) {
        return false;
    }
    return true;
}

}  // namespace

// ---------------------------------------------------------------------------
// Impl
// ---------------------------------------------------------------------------
struct QuicTestClient::Impl
{
    net::IoContext& ctx;
    std::string peer_host;
    uint16_t peer_port{0};

    std::unique_ptr<net::UdpSocket> socket;

    SSL_CTX* ssl_ctx{nullptr};
    SSL* ssl{nullptr};
    ngtcp2_crypto_ossl_ctx* ossl_ctx{nullptr};
    ngtcp2_crypto_conn_ref conn_ref{};

    ngtcp2_conn* qconn{nullptr};
    nghttp3_conn* h3conn{nullptr};

    // sockaddr storage
    sockaddr_in remote_addr{};
    sockaddr_in local_addr{};
    bool local_addr_known{false};

    // Connection state
    std::atomic<bool> handshake_confirmed{false};
    std::atomic<bool> h3_settings_sent{false};
    std::atomic<bool> closed{false};
    std::atomic<bool> stopped{false};

    // Last ngtcp2 error (set from callbacks)
    int last_error{0};

    // Per-request in-flight state. For the test client we only ever expect
    // one outstanding request at a time but using a map keeps stream
    // ownership clean.
    struct PendingRequest {
        int64_t stream_id{-1};
        std::vector<uint8_t> body;        // request body (owned)
        size_t body_offset{0};            // bytes already handed to nghttp3
        bool body_eof_sent{false};
        bool end_stream_seen{false};
        TestResponse response;
    };
    std::unordered_map<int64_t, PendingRequest> pending;

    // Header-field scratch staging used by nghttp3 read callback.
    // nghttp3 gives headers as name/value pairs.
    //
    // We also buffer the request headers to own the memory during
    // nghttp3_conn_submit_request (it takes pointers, not copies).
    struct ReqHeaders {
        std::vector<std::string> names;
        std::vector<std::string> values;
        std::vector<nghttp3_nv> nvs;
    };
    std::unordered_map<int64_t, std::shared_ptr<ReqHeaders>> req_hdr_owners;

    // RFC 9221 inbound datagrams (server → client direction).
    std::deque<std::vector<uint8_t>> inbound_datagrams;

    Impl(net::IoContext& c, std::string h, uint16_t p)
        : ctx(c), peer_host(std::move(h)), peer_port(p) {}

    ~Impl() { teardown(); }

    void teardown()
    {
        stopped = true;
        if (h3conn) {
            nghttp3_conn_del(h3conn);
            h3conn = nullptr;
        }
        if (qconn) {
            ngtcp2_conn_del(qconn);
            qconn = nullptr;
        }
        if (ossl_ctx) {
            ngtcp2_crypto_ossl_ctx_del(ossl_ctx);
            ossl_ctx = nullptr;
        }
        if (ssl) {
            // Clear app_data before SSL_free (per ngtcp2_crypto_ossl docs).
            SSL_set_app_data(ssl, nullptr);
            SSL_free(ssl);
            ssl = nullptr;
        }
        if (ssl_ctx) {
            SSL_CTX_free(ssl_ctx);
            ssl_ctx = nullptr;
        }
        if (socket) {
            socket->close();
            socket.reset();
        }
    }

    // -----------------------------------------------------------------------
    // TLS / SSL context
    // -----------------------------------------------------------------------
    expected<void, Error> init_ssl()
    {
        ensure_ossl_init();
        if (!g_ossl_init_ok) {
            return unexpected(Error::io(IoError::Unknown,
                                         "ngtcp2_crypto_ossl_init failed"));
        }
        ssl_ctx = SSL_CTX_new(TLS_client_method());
        if (!ssl_ctx) {
            return unexpected(Error::io(IoError::Unknown,
                                         "SSL_CTX_new failed"));
        }
        // No peer verification in the test client.
        SSL_CTX_set_verify(ssl_ctx, SSL_VERIFY_NONE, nullptr);
        SSL_CTX_set_min_proto_version(ssl_ctx, TLS1_3_VERSION);
        SSL_CTX_set_max_proto_version(ssl_ctx, TLS1_3_VERSION);

        ssl = SSL_new(ssl_ctx);
        if (!ssl) {
            return unexpected(Error::io(IoError::Unknown,
                                         "SSL_new failed"));
        }
        if (ngtcp2_crypto_ossl_configure_client_session(ssl) != 0) {
            return unexpected(Error::io(IoError::Unknown,
                                         "configure_client_session failed"));
        }
        SSL_set_connect_state(ssl);
        // ALPN h3
        static const unsigned char alpn[] = {0x02, 'h', '3'};
        SSL_set_alpn_protos(ssl, alpn, sizeof(alpn));
        // SNI
        SSL_set_tlsext_host_name(ssl, peer_host.c_str());

        // Per-connection crypto_ossl_ctx wrapping the SSL. This is what
        // ngtcp2_conn_set_tls_native_handle expects (NOT the raw SSL*).
        if (ngtcp2_crypto_ossl_ctx_new(&ossl_ctx, ssl) != 0) {
            return unexpected(Error::io(IoError::Unknown,
                                         "ngtcp2_crypto_ossl_ctx_new failed"));
        }

        // Install conn_ref on the SSL so crypto callbacks can resolve the
        // ngtcp2_conn via SSL_get_app_data.
        conn_ref.get_conn = [](ngtcp2_crypto_conn_ref* r) -> ngtcp2_conn* {
            return static_cast<Impl*>(r->user_data)->qconn;
        };
        conn_ref.user_data = this;
        SSL_set_app_data(ssl, &conn_ref);
        return {};
    }

    // -----------------------------------------------------------------------
    // ngtcp2 callbacks (client-side)
    // -----------------------------------------------------------------------
    static int client_initial_cb(ngtcp2_conn* conn, void* user_data)
    {
        return ngtcp2_crypto_client_initial_cb(conn, user_data);
    }

    static int recv_crypto_data_cb(ngtcp2_conn* conn,
                                    ngtcp2_encryption_level enc_level,
                                    uint64_t offset, const uint8_t* data,
                                    size_t datalen, void* user_data)
    {
        return ngtcp2_crypto_recv_crypto_data_cb(conn, enc_level, offset,
                                                 data, datalen, user_data);
    }

    static int handshake_completed_cb(ngtcp2_conn* conn, void* user_data)
    {
        (void)conn;
        auto* self = static_cast<Impl*>(user_data);
        self->handshake_confirmed = true;
        // Set up nghttp3 once the handshake completes.
        self->setup_h3();
        return 0;
    }

    static int encrypt_cb(uint8_t* dest, const ngtcp2_crypto_aead* aead,
                           const ngtcp2_crypto_aead_ctx* aead_ctx,
                           const uint8_t* plaintext, size_t plaintextlen,
                           const uint8_t* nonce, size_t noncelen,
                           const uint8_t* aad, size_t aadlen)
    {
        return ngtcp2_crypto_encrypt_cb(dest, aead, aead_ctx, plaintext,
                                        plaintextlen, nonce, noncelen, aad,
                                        aadlen);
    }

    static int decrypt_cb(uint8_t* dest, const ngtcp2_crypto_aead* aead,
                           const ngtcp2_crypto_aead_ctx* aead_ctx,
                           const uint8_t* ciphertext, size_t ciphertextlen,
                           const uint8_t* nonce, size_t noncelen,
                           const uint8_t* aad, size_t aadlen)
    {
        return ngtcp2_crypto_decrypt_cb(dest, aead, aead_ctx, ciphertext,
                                        ciphertextlen, nonce, noncelen, aad,
                                        aadlen);
    }

    static int hp_mask_cb(uint8_t* dest, const ngtcp2_crypto_cipher* hp,
                          const ngtcp2_crypto_cipher_ctx* hp_ctx,
                          const uint8_t* sample)
    {
        return ngtcp2_crypto_hp_mask_cb(dest, hp, hp_ctx, sample);
    }

    static int recv_stream_data_cb(ngtcp2_conn* conn, uint32_t flags,
                                    int64_t stream_id, uint64_t offset,
                                    const uint8_t* data, size_t datalen,
                                    void* user_data, void* stream_user_data)
    {
        (void)conn;
        (void)offset;
        (void)stream_user_data;
        auto* self = static_cast<Impl*>(user_data);
        if (!self->h3conn) return 0;
        auto n = nghttp3_conn_read_stream(self->h3conn, stream_id, data,
                                          datalen,
                                          (flags & NGTCP2_STREAM_DATA_FLAG_FIN) ? 1 : 0);
        if (n < 0) {
            self->last_error = static_cast<int>(n);
            return NGTCP2_ERR_CALLBACK_FAILURE;
        }
        ngtcp2_conn_extend_max_stream_offset(conn, stream_id,
                                              static_cast<uint64_t>(n));
        ngtcp2_conn_extend_max_offset(conn, static_cast<uint64_t>(n));
        return 0;
    }

    static int acked_stream_data_offset_cb(ngtcp2_conn* conn, int64_t stream_id,
                                            uint64_t offset, uint64_t datalen,
                                            void* user_data,
                                            void* stream_user_data)
    {
        (void)conn;
        (void)offset;
        (void)stream_user_data;
        auto* self = static_cast<Impl*>(user_data);
        if (self->h3conn) {
            nghttp3_conn_add_ack_offset(self->h3conn, stream_id, datalen);
        }
        return 0;
    }

    static int stream_close_cb(ngtcp2_conn* conn, uint32_t flags,
                                int64_t stream_id, uint64_t app_error_code,
                                void* user_data, void* stream_user_data)
    {
        (void)conn;
        (void)stream_user_data;
        auto* self = static_cast<Impl*>(user_data);
        uint64_t aec = app_error_code;
        if (!(flags & NGTCP2_STREAM_CLOSE_FLAG_APP_ERROR_CODE_SET)) {
            aec = NGHTTP3_H3_NO_ERROR;
        }
        if (self->h3conn) {
            auto rv = nghttp3_conn_close_stream(self->h3conn, stream_id, aec);
            if (rv != 0 && rv != NGHTTP3_ERR_STREAM_NOT_FOUND) {
                self->last_error = rv;
                return NGTCP2_ERR_CALLBACK_FAILURE;
            }
        }
        return 0;
    }

    static int stream_reset_cb(ngtcp2_conn* conn, int64_t stream_id,
                                uint64_t final_size, uint64_t app_error_code,
                                void* user_data, void* stream_user_data)
    {
        (void)conn;
        (void)final_size;
        (void)app_error_code;
        (void)stream_user_data;
        auto* self = static_cast<Impl*>(user_data);
        if (self->h3conn) {
            nghttp3_conn_shutdown_stream_read(self->h3conn, stream_id);
        }
        return 0;
    }

    static int extend_max_local_streams_bidi_cb(ngtcp2_conn* conn,
                                                 uint64_t max_streams,
                                                 void* user_data)
    {
        (void)conn;
        (void)max_streams;
        (void)user_data;
        return 0;
    }

    static void rand_cb(uint8_t* dest, size_t destlen,
                         const ngtcp2_rand_ctx* rand_ctx)
    {
        (void)rand_ctx;
        fill_random(dest, destlen);
    }

    static int get_new_connection_id_cb(ngtcp2_conn* conn, ngtcp2_cid* cid,
                                         uint8_t* token, size_t cidlen,
                                         void* user_data)
    {
        (void)conn;
        (void)user_data;
        fill_random(cid->data, cidlen);
        cid->datalen = cidlen;
        fill_random(token, NGTCP2_STATELESS_RESET_TOKENLEN);
        return 0;
    }

    static int update_key_cb(ngtcp2_conn* conn, uint8_t* rx_secret,
                              uint8_t* tx_secret,
                              ngtcp2_crypto_aead_ctx* rx_aead_ctx,
                              uint8_t* rx_iv,
                              ngtcp2_crypto_aead_ctx* tx_aead_ctx,
                              uint8_t* tx_iv,
                              const uint8_t* current_rx_secret,
                              const uint8_t* current_tx_secret,
                              size_t secretlen, void* user_data)
    {
        (void)user_data;
        return ngtcp2_crypto_update_key_cb(
            conn, rx_secret, tx_secret, rx_aead_ctx, rx_iv, tx_aead_ctx, tx_iv,
            current_rx_secret, current_tx_secret, secretlen, user_data);
    }

    static void delete_crypto_aead_ctx_cb(ngtcp2_conn* conn,
                                           ngtcp2_crypto_aead_ctx* aead_ctx,
                                           void* user_data)
    {
        ngtcp2_crypto_delete_crypto_aead_ctx_cb(conn, aead_ctx, user_data);
    }

    static void delete_crypto_cipher_ctx_cb(ngtcp2_conn* conn,
                                             ngtcp2_crypto_cipher_ctx* cipher_ctx,
                                             void* user_data)
    {
        ngtcp2_crypto_delete_crypto_cipher_ctx_cb(conn, cipher_ctx, user_data);
    }

    static int get_path_challenge_data_cb(ngtcp2_conn* conn, uint8_t* data,
                                           void* user_data)
    {
        return ngtcp2_crypto_get_path_challenge_data_cb(conn, data, user_data);
    }

    static int version_negotiation_cb(ngtcp2_conn* conn, uint32_t version,
                                       const ngtcp2_cid* client_dcid,
                                       void* user_data)
    {
        return ngtcp2_crypto_version_negotiation_cb(conn, version, client_dcid,
                                                    user_data);
    }

    // RFC 9221 — ngtcp2 fires this when a DATAGRAM frame arrives from the server.
    static int recv_datagram_cb(ngtcp2_conn* /*conn*/, uint32_t /*flags*/,
                                const uint8_t* data, size_t datalen,
                                void* user_data)
    {
        auto* self = static_cast<Impl*>(user_data);
        self->inbound_datagrams.emplace_back(data, data + datalen);
        return 0;
    }

    // -----------------------------------------------------------------------
    // nghttp3 callbacks
    // -----------------------------------------------------------------------
    static int h3_recv_header(nghttp3_conn* conn, int64_t stream_id,
                               int32_t token, nghttp3_rcbuf* name,
                               nghttp3_rcbuf* value, uint8_t flags,
                               void* user_data, void* stream_user_data)
    {
        (void)conn;
        (void)flags;
        (void)stream_user_data;
        auto* self = static_cast<Impl*>(user_data);
        auto it = self->pending.find(stream_id);
        if (it == self->pending.end()) return 0;
        auto nv = nghttp3_rcbuf_get_buf(name);
        auto vv = nghttp3_rcbuf_get_buf(value);
        std::string n(reinterpret_cast<const char*>(nv.base), nv.len);
        std::string v(reinterpret_cast<const char*>(vv.base), vv.len);
        if (token == NGHTTP3_QPACK_TOKEN__STATUS) {
            try { it->second.response.status = std::stoi(v); } catch (...) {}
        } else if (!n.empty() && n[0] != ':') {
            it->second.response.headers[std::move(n)] = std::move(v);
        }
        return 0;
    }

    static int h3_recv_data(nghttp3_conn* conn, int64_t stream_id,
                             const uint8_t* data, size_t datalen,
                             void* user_data, void* stream_user_data)
    {
        (void)conn;
        (void)stream_user_data;
        auto* self = static_cast<Impl*>(user_data);
        auto it = self->pending.find(stream_id);
        if (it != self->pending.end()) {
            it->second.response.body.append(reinterpret_cast<const char*>(data),
                                            datalen);
        }
        return 0;
    }

    static int h3_deferred_consume(nghttp3_conn* conn, int64_t stream_id,
                                    size_t consumed, void* user_data,
                                    void* stream_user_data)
    {
        (void)conn;
        (void)stream_id;
        (void)stream_user_data;
        auto* self = static_cast<Impl*>(user_data);
        if (self->qconn) {
            ngtcp2_conn_extend_max_offset(self->qconn, consumed);
        }
        return 0;
    }

    static int h3_end_stream(nghttp3_conn* conn, int64_t stream_id,
                              void* user_data, void* stream_user_data)
    {
        (void)conn;
        (void)stream_user_data;
        auto* self = static_cast<Impl*>(user_data);
        auto it = self->pending.find(stream_id);
        if (it != self->pending.end()) {
            it->second.end_stream_seen = true;
        }
        return 0;
    }

    static int h3_stop_sending(nghttp3_conn* conn, int64_t stream_id,
                                uint64_t app_error_code, void* user_data,
                                void* stream_user_data)
    {
        (void)conn;
        (void)user_data;
        (void)stream_user_data;
        (void)stream_id;
        (void)app_error_code;
        return 0;
    }

    static int h3_reset_stream(nghttp3_conn* conn, int64_t stream_id,
                                uint64_t app_error_code, void* user_data,
                                void* stream_user_data)
    {
        (void)conn;
        (void)stream_id;
        (void)app_error_code;
        (void)user_data;
        (void)stream_user_data;
        return 0;
    }

    // Data-source read callback: streams request body to nghttp3.
    static nghttp3_ssize h3_read_data(nghttp3_conn* conn, int64_t stream_id,
                                       nghttp3_vec* vec, size_t veccnt,
                                       uint32_t* pflags, void* user_data,
                                       void* stream_user_data)
    {
        (void)conn;
        (void)veccnt;
        (void)stream_user_data;
        auto* self = static_cast<Impl*>(user_data);
        auto it = self->pending.find(stream_id);
        if (it == self->pending.end()) {
            *pflags |= NGHTTP3_DATA_FLAG_EOF;
            return 0;
        }
        auto& req = it->second;
        if (req.body_offset >= req.body.size()) {
            *pflags |= NGHTTP3_DATA_FLAG_EOF;
            req.body_eof_sent = true;
            return 0;
        }
        vec[0].base = req.body.data() + req.body_offset;
        vec[0].len = req.body.size() - req.body_offset;
        req.body_offset = req.body.size();
        *pflags |= NGHTTP3_DATA_FLAG_EOF;
        req.body_eof_sent = true;
        return 1;
    }

    // -----------------------------------------------------------------------
    // HTTP/3 setup once handshake completes
    // -----------------------------------------------------------------------
    void setup_h3()
    {
        if (h3conn) return;

        nghttp3_callbacks h3cb{};
        h3cb.recv_header          = h3_recv_header;
        h3cb.recv_data            = h3_recv_data;
        h3cb.deferred_consume     = h3_deferred_consume;
        h3cb.end_stream           = h3_end_stream;
        h3cb.stop_sending         = h3_stop_sending;
        h3cb.reset_stream         = h3_reset_stream;

        nghttp3_settings h3settings;
        nghttp3_settings_default(&h3settings);
        h3settings.qpack_max_dtable_capacity = 4096;
        h3settings.qpack_blocked_streams     = 100;

        if (nghttp3_conn_client_new(&h3conn, &h3cb, &h3settings, nullptr, this) != 0) {
            last_error = -1;
            return;
        }

        // Open and bind the uni streams required by HTTP/3.
        int64_t ctrl_stream_id = -1;
        if (ngtcp2_conn_open_uni_stream(qconn, &ctrl_stream_id, nullptr) != 0) return;
        if (nghttp3_conn_bind_control_stream(h3conn, ctrl_stream_id) != 0) return;

        int64_t qpack_enc_stream_id = -1;
        int64_t qpack_dec_stream_id = -1;
        if (ngtcp2_conn_open_uni_stream(qconn, &qpack_enc_stream_id, nullptr) != 0) return;
        if (ngtcp2_conn_open_uni_stream(qconn, &qpack_dec_stream_id, nullptr) != 0) return;
        if (nghttp3_conn_bind_qpack_streams(h3conn, qpack_enc_stream_id,
                                             qpack_dec_stream_id) != 0) {
            return;
        }
        h3_settings_sent = true;
    }

    // -----------------------------------------------------------------------
    // Bind UDP socket & build ngtcp2 path
    // -----------------------------------------------------------------------
    expected<void, Error> open_socket()
    {
        auto bind_res = ctx.bind_udp(0);
        if (!bind_res) return unexpected(bind_res.error());
        socket = std::move(*bind_res);
        if (!parse_ipv4(peer_host, peer_port, remote_addr)) {
            return unexpected(Error::io(IoError::InvalidArgument,
                                         "peer host must be an IPv4 literal"));
        }
        // local addr = 127.0.0.1:socket->local_port(). We fill it in for the
        // ngtcp2_path even though ngtcp2 rarely needs the exact local sockaddr
        // when the platform doesn't tell us. Good enough for loopback tests.
        std::memset(&local_addr, 0, sizeof(local_addr));
        local_addr.sin_family = AF_INET;
        local_addr.sin_port = htons(socket->local_port());
        ::inet_pton(AF_INET, "127.0.0.1", &local_addr.sin_addr);
        local_addr_known = true;
        return {};
    }

    ngtcp2_path make_path()
    {
        ngtcp2_path p{};
        p.local.addr = reinterpret_cast<ngtcp2_sockaddr*>(&local_addr);
        p.local.addrlen = sizeof(local_addr);
        p.remote.addr = reinterpret_cast<ngtcp2_sockaddr*>(&remote_addr);
        p.remote.addrlen = sizeof(remote_addr);
        p.user_data = nullptr;
        return p;
    }

    expected<void, Error> create_conn()
    {
        ngtcp2_callbacks cb{};
        cb.client_initial                = client_initial_cb;
        cb.recv_retry                    = ngtcp2_crypto_recv_retry_cb;
        cb.recv_crypto_data              = recv_crypto_data_cb;
        cb.handshake_completed           = handshake_completed_cb;
        cb.encrypt                       = encrypt_cb;
        cb.decrypt                       = decrypt_cb;
        cb.hp_mask                        = hp_mask_cb;
        cb.recv_stream_data               = recv_stream_data_cb;
        cb.acked_stream_data_offset       = acked_stream_data_offset_cb;
        cb.stream_close                   = stream_close_cb;
        cb.stream_reset                   = stream_reset_cb;
        cb.extend_max_local_streams_bidi  = extend_max_local_streams_bidi_cb;
        cb.rand                           = rand_cb;
        cb.get_new_connection_id          = get_new_connection_id_cb;
        cb.update_key                     = update_key_cb;
        cb.delete_crypto_aead_ctx         = delete_crypto_aead_ctx_cb;
        cb.delete_crypto_cipher_ctx       = delete_crypto_cipher_ctx_cb;
        cb.get_path_challenge_data        = get_path_challenge_data_cb;
        cb.version_negotiation            = version_negotiation_cb;
        cb.recv_datagram                  = recv_datagram_cb;

        ngtcp2_cid scid{};
        ngtcp2_cid dcid{};
        scid.datalen = 8;
        fill_random(scid.data, scid.datalen);
        dcid.datalen = 8;
        fill_random(dcid.data, dcid.datalen);

        ngtcp2_settings settings;
        ngtcp2_settings_default(&settings);
        settings.initial_ts = now_ns();
        settings.log_printf = ngtcp2_client_log_cb;

        ngtcp2_transport_params params;
        ngtcp2_transport_params_default(&params);
        params.initial_max_stream_data_bidi_local  = 256 * 1024;
        params.initial_max_stream_data_bidi_remote = 256 * 1024;
        params.initial_max_stream_data_uni         = 256 * 1024;
        params.initial_max_data                    = 1024 * 1024;
        params.initial_max_streams_bidi            = 100;
        params.initial_max_streams_uni             = 3;
        params.max_idle_timeout                    = 30ULL * NGTCP2_SECONDS;
        params.active_connection_id_limit          = 7;
        params.max_datagram_frame_size             = 65535;  // Advertise RFC 9221 support

        auto path = make_path();
        int rv = ngtcp2_conn_client_new(&qconn, &dcid, &scid, &path,
                                         NGTCP2_PROTO_VER_V1, &cb, &settings,
                                         &params, nullptr, this);
        if (rv != 0) {
            return unexpected(Error::io(IoError::Unknown,
                                         "ngtcp2_conn_client_new failed"));
        }
        ngtcp2_conn_set_tls_native_handle(qconn, ossl_ctx);
        return {};
    }

    // -----------------------------------------------------------------------
    // I/O pump
    // -----------------------------------------------------------------------

    // Drive writev_stream until nothing to send / blocked, sending each
    // resulting datagram via async_send_to.
    Task<void> write_loop_once()
    {
        if (!qconn || closed) co_return;
        std::vector<uint8_t> buf(1500);
        for (;;) {
            if (!qconn || closed) co_return;

            int64_t stream_id = -1;
            int fin = 0;
            nghttp3_vec vec[16];
            nghttp3_ssize veccnt = 0;
            if (h3conn) {
                auto sv = nghttp3_conn_writev_stream(h3conn, &stream_id, &fin,
                                                     vec, 16);
                if (sv < 0) {
                    last_error = static_cast<int>(sv);
                    co_return;
                }
                veccnt = sv;
            }

            ngtcp2_path_storage ps;
            ngtcp2_path_storage_zero(&ps);
            ngtcp2_pkt_info pi{};

            uint32_t flags = NGTCP2_WRITE_STREAM_FLAG_MORE;
            if (fin) flags |= NGTCP2_WRITE_STREAM_FLAG_FIN;

            ngtcp2_ssize wdatalen = 0;
            auto nwrite = ngtcp2_conn_writev_stream(
                qconn, &ps.path, &pi, buf.data(), buf.size(), &wdatalen, flags,
                stream_id, reinterpret_cast<const ngtcp2_vec*>(vec),
                static_cast<size_t>(veccnt), now_ns());

            if (nwrite < 0) {
                if (nwrite == NGTCP2_ERR_WRITE_MORE) {
                    // Tell nghttp3 how much of the stream data got buffered so
                    // it can advance its internal offsets.
                    if (h3conn && wdatalen >= 0) {
                        nghttp3_conn_add_write_offset(h3conn, stream_id, wdatalen);
                    }
                    continue;
                }
                if (nwrite == NGTCP2_ERR_STREAM_DATA_BLOCKED ||
                    nwrite == NGTCP2_ERR_STREAM_SHUT_WR) {
                    if (h3conn && stream_id >= 0) {
                        nghttp3_conn_block_stream(h3conn, stream_id);
                    }
                    continue;
                }
                // Fatal — bail.
                last_error = static_cast<int>(nwrite);
                co_return;
            }

            if (h3conn && wdatalen >= 0 && stream_id >= 0) {
                nghttp3_conn_add_write_offset(h3conn, stream_id, wdatalen);
            }

            if (nwrite == 0) co_return;

            net::UdpEndpoint peer_ep{peer_host, peer_port};
            auto sres = co_await socket->async_send_to(buf.data(),
                                                       static_cast<size_t>(nwrite),
                                                       peer_ep);
            if (!sres) {
                last_error = -1;
                co_return;
            }
        }
    }

    // Single recv -> ngtcp2_conn_read_pkt.
    Task<expected<void, Error>> read_once(std::vector<uint8_t>& buf)
    {
        auto rres = co_await socket->async_recv_from(buf.data(), buf.size());
        if (!rres) {
            co_return unexpected(rres.error());
        }
        auto [n, peer] = *rres;
        if (n == 0) co_return expected<void, Error>{};

        auto path = make_path();
        ngtcp2_pkt_info pi{};
        auto rv = ngtcp2_conn_read_pkt(qconn, &path, &pi, buf.data(), n,
                                        now_ns());
        if (rv != 0) {
            co_return unexpected(Error::io(IoError::Unknown,
                                            "ngtcp2_conn_read_pkt failed"));
        }
        co_return expected<void, Error>{};
    }
};

// ---------------------------------------------------------------------------
// QuicTestClient thin wrapper
// ---------------------------------------------------------------------------
QuicTestClient::QuicTestClient(net::IoContext& ctx,
                                const std::string& peer_host,
                                uint16_t peer_port)
    : impl_(std::make_unique<Impl>(ctx, peer_host, peer_port))
{}

QuicTestClient::~QuicTestClient() = default;

Task<expected<void, Error>> QuicTestClient::connect(std::chrono::milliseconds timeout)
{
    auto& I = *impl_;

    if (auto r = I.init_ssl(); !r) co_return unexpected(r.error());
    if (auto r = I.open_socket(); !r) co_return unexpected(r.error());
    if (auto r = I.create_conn(); !r) co_return unexpected(r.error());

    // Kick off the very first flight (client Initial).
    co_await I.write_loop_once();

    using clock = std::chrono::steady_clock;
    auto deadline = clock::now() + timeout;
    std::vector<uint8_t> rbuf(65535);

    while (!I.handshake_confirmed && clock::now() < deadline) {
        // Read one datagram with an effective micro-timeout driven by the
        // outer deadline. The current UDP API has no cancellable recv, so we
        // fall back to "recv then check" — acceptable for in-process tests
        // where the server responds promptly.
        auto r = co_await I.read_once(rbuf);
        if (!r) co_return unexpected(r.error());
        co_await I.write_loop_once();
    }

    if (!I.handshake_confirmed) {
        co_return unexpected(Error::io(IoError::Timeout,
                                        "QUIC handshake timed out"));
    }

    // Make sure HTTP/3 control/QPACK streams are flushed.
    co_await I.write_loop_once();
    co_return expected<void, Error>{};
}

Task<expected<TestResponse, Error>> QuicTestClient::request(
    std::string method, std::string path,
    std::unordered_map<std::string, std::string> headers, std::string body)
{
    auto& I = *impl_;
    if (!I.handshake_confirmed || !I.h3conn) {
        co_return unexpected(Error::io(IoError::InvalidArgument,
                                        "connect() must succeed first"));
    }

    int64_t stream_id = -1;
    if (ngtcp2_conn_open_bidi_stream(I.qconn, &stream_id, nullptr) != 0) {
        co_return unexpected(Error::io(IoError::Unknown,
                                        "open_bidi_stream failed"));
    }

    auto& req = I.pending[stream_id];
    req.stream_id = stream_id;
    req.body.assign(body.begin(), body.end());

    auto owner = std::make_shared<Impl::ReqHeaders>();
    auto add_nv = [&](std::string name, std::string value) {
        owner->names.push_back(std::move(name));
        owner->values.push_back(std::move(value));
    };
    add_nv(":method", method);
    add_nv(":scheme", "https");
    add_nv(":authority", I.peer_host + ":" + std::to_string(I.peer_port));
    add_nv(":path", path);
    for (auto& [k, v] : headers) add_nv(k, v);
    if (!req.body.empty()) {
        add_nv("content-length", std::to_string(req.body.size()));
    }

    owner->nvs.reserve(owner->names.size());
    for (size_t i = 0; i < owner->names.size(); ++i) {
        nghttp3_nv nv{};
        nv.name      = reinterpret_cast<uint8_t*>(owner->names[i].data());
        nv.namelen   = owner->names[i].size();
        nv.value     = reinterpret_cast<uint8_t*>(owner->values[i].data());
        nv.valuelen  = owner->values[i].size();
        nv.flags     = NGHTTP3_NV_FLAG_NONE;
        owner->nvs.push_back(nv);
    }
    I.req_hdr_owners[stream_id] = owner;

    nghttp3_data_reader dr{};
    dr.read_data = Impl::h3_read_data;

    auto rv = nghttp3_conn_submit_request(I.h3conn, stream_id, owner->nvs.data(),
                                           owner->nvs.size(),
                                           req.body.empty() ? nullptr : &dr,
                                           nullptr);
    if (rv != 0) {
        co_return unexpected(Error::io(IoError::Unknown,
                                        "nghttp3_conn_submit_request failed"));
    }

    // Drive the exchange until the response stream sees end_stream.
    std::vector<uint8_t> rbuf(65535);
    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    co_await I.write_loop_once();

    while (!req.end_stream_seen &&
           std::chrono::steady_clock::now() < deadline &&
           !I.closed) {
        auto r = co_await I.read_once(rbuf);
        if (!r) co_return unexpected(r.error());
        co_await I.write_loop_once();
    }

    if (!req.end_stream_seen) {
        I.pending.erase(stream_id);
        I.req_hdr_owners.erase(stream_id);
        co_return unexpected(Error::io(IoError::Timeout,
                                        "HTTP/3 response timed out"));
    }

    TestResponse resp = std::move(req.response);
    I.pending.erase(stream_id);
    I.req_hdr_owners.erase(stream_id);
    co_return resp;
}

Task<expected<void, Error>> QuicTestClient::send_datagram(std::vector<uint8_t> payload)
{
    auto& I = *impl_;
    if (!I.handshake_confirmed || !I.qconn || I.closed) {
        co_return unexpected(Error::io(IoError::InvalidArgument,
                                        "not connected"));
    }

    // Verify the server advertised datagram support in its transport params.
    const ngtcp2_transport_params* remote = ngtcp2_conn_get_remote_transport_params(I.qconn);
    if (!remote || remote->max_datagram_frame_size == 0) {
        co_return unexpected(Error::io(IoError::InvalidArgument,
                                        "server did not negotiate QUIC datagram support"));
    }

    std::vector<uint8_t> pkt(NGTCP2_MAX_UDP_PAYLOAD_SIZE);
    ngtcp2_path_storage ps;
    ngtcp2_path_storage_zero(&ps);
    ngtcp2_pkt_info pi{};
    int accepted = 0;

    ngtcp2_ssize nwrite = ngtcp2_conn_write_datagram(
        I.qconn, &ps.path, &pi, pkt.data(), pkt.size(), &accepted,
        NGTCP2_WRITE_DATAGRAM_FLAG_NONE, /*dgram_id=*/0,
        payload.data(), payload.size(), now_ns());

    if (nwrite < 0) {
        co_return unexpected(Error::io(IoError::Unknown,
                                        ngtcp2_strerror(static_cast<int>(nwrite))));
    }

    if (nwrite > 0 && accepted && I.socket) {
        pkt.resize(static_cast<std::size_t>(nwrite));
        net::UdpEndpoint peer_ep{I.peer_host, I.peer_port};
        auto sres = co_await I.socket->async_send_to(pkt.data(), pkt.size(), peer_ep);
        if (!sres) {
            co_return unexpected(sres.error());
        }
    }
    co_return expected<void, Error>{};
}

Task<std::optional<std::vector<uint8_t>>>
QuicTestClient::next_datagram(std::chrono::milliseconds timeout)
{
    auto& I = *impl_;
    if (!I.inbound_datagrams.empty()) {
        auto dg = std::move(I.inbound_datagrams.front());
        I.inbound_datagrams.pop_front();
        co_return dg;
    }

    // Poll: drive the read loop until a datagram arrives or we time out.
    std::vector<uint8_t> rbuf(65535);
    auto deadline = std::chrono::steady_clock::now() + timeout;
    while (I.inbound_datagrams.empty() && !I.closed) {
        if (std::chrono::steady_clock::now() >= deadline) co_return std::nullopt;
        auto r = co_await I.read_once(rbuf);
        if (!r) co_return std::nullopt;
        co_await I.write_loop_once();
    }

    if (I.inbound_datagrams.empty()) co_return std::nullopt;
    auto dg = std::move(I.inbound_datagrams.front());
    I.inbound_datagrams.pop_front();
    co_return dg;
}

Task<void> QuicTestClient::close()
{
    auto& I = *impl_;
    if (I.closed || !I.qconn) co_return;
    I.closed = true;

    ngtcp2_path_storage ps;
    ngtcp2_path_storage_zero(&ps);
    ngtcp2_pkt_info pi{};
    ngtcp2_ccerr ccerr;
    ngtcp2_ccerr_default(&ccerr);

    std::vector<uint8_t> buf(1500);
    auto n = ngtcp2_conn_write_connection_close(I.qconn, &ps.path, &pi,
                                                 buf.data(), buf.size(),
                                                 &ccerr, now_ns());
    if (n > 0 && I.socket) {
        net::UdpEndpoint peer_ep{I.peer_host, I.peer_port};
        (void)co_await I.socket->async_send_to(buf.data(),
                                                static_cast<size_t>(n), peer_ep);
    }
    co_return;
}

}  // namespace coroute::http3::test
