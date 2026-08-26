// HTTP/3 unit + setup tests.
//
// All tests are gated on COROUTE_HAS_HTTP3. The Http3Stream tests construct
// the stream with a null parent connection — the header/body accumulation
// callbacks only touch the Request field and are safe to call in isolation.
//
// Tests that require real network traffic (TLS handshake, stream
// request/response, etc.) live in test_http3_e2e.cpp and use the in-process
// QuicTestClient.
//
// NOTE: unlike the upstream org repo this was ported from, this port is
// server-only — no HTTP/3 client (Http3ClientConnection) exists here, so
// the client-dispatch 0-RTT tests from the upstream file are not included.

#include <catch2/catch_test_macros.hpp>

#ifdef COROUTE_HAS_HTTP3

#include "coroute/http3/quic_server.hpp"
#include "coroute/http3/zero_rtt_replay_cache.hpp"
#include "coroute/core/request.hpp"
#include "coroute/net/io_context.hpp"

#include <cstdint>
#include <cstring>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <vector>

using namespace coroute;
using namespace coroute::http3;

// ============================================================================
// Http3Stream — header / body accumulation (no network)
// ============================================================================

TEST_CASE("HTTP/3 Http3Stream construction exposes id and empty request",
          "[http3][stream][unit]")
{
    Http3Stream s{nullptr, 42};
    REQUIRE(s.id() == 42);
    REQUIRE(s.request().body().empty());
    REQUIRE(s.request().path().empty());
    REQUIRE_FALSE(s.request().header("X-Not-There").has_value());
}

TEST_CASE("HTTP/3 :method pseudo-header sets request method",
          "[http3][stream][unit]")
{
    Http3Stream s{nullptr, 1};
    s.on_header(":method", "POST");

    REQUIRE(s.request().method() == HttpMethod::POST);
}

TEST_CASE("HTTP/3 pseudo-headers :method/:path/:authority populate Request",
          "[http3][stream][unit]")
{
    Http3Stream s{nullptr, 3};
    s.on_header(":method", "GET");
    s.on_header(":path", "/api/items");
    s.on_header(":authority", "example.com:8443");
    s.on_header(":scheme", "https");

    REQUIRE(s.request().method() == HttpMethod::GET);
    REQUIRE(s.request().path() == "/api/items");

    // :authority is mapped to the Host header.
    auto host = s.request().header("Host");
    REQUIRE(host.has_value());
    REQUIRE(*host == "example.com:8443");
}

TEST_CASE("HTTP/3 :path with query string is split", "[http3][stream][unit]")
{
    Http3Stream s{nullptr, 5};
    s.on_header(":path", "/search?q=hello&limit=10");

    REQUIRE(s.request().path() == "/search");
    REQUIRE(s.request().query_string() == "q=hello&limit=10");
}

TEST_CASE("HTTP/3 regular headers appear in request headers map",
          "[http3][stream][unit]")
{
    Http3Stream s{nullptr, 7};
    s.on_header("content-type", "application/json");
    s.on_header("authorization", "Bearer abc123");
    s.on_header("x-custom", "yes");

    REQUIRE(s.request().header("content-type").value_or("") == "application/json");
    REQUIRE(s.request().header("authorization").value_or("") == "Bearer abc123");
    REQUIRE(s.request().header("x-custom").value_or("") == "yes");
}

TEST_CASE("HTTP/3 empty header value is preserved", "[http3][stream][unit]")
{
    Http3Stream s{nullptr, 9};
    s.on_header("x-empty", "");

    auto v = s.request().header("x-empty");
    REQUIRE(v.has_value());
    REQUIRE(v->empty());
}

TEST_CASE("HTTP/3 large header value (4 KB) round-trips intact",
          "[http3][stream][unit]")
{
    constexpr std::size_t kBig = 4096;
    std::string big_value(kBig, 'X');

    Http3Stream s{nullptr, 11};
    s.on_header("x-big", big_value);

    auto got = s.request().header("x-big");
    REQUIRE(got.has_value());
    REQUIRE(got->size() == kBig);
    REQUIRE(std::string(*got) == big_value);
}

TEST_CASE("HTTP/3 on_body stores the payload bytes exactly once",
          "[http3][stream][unit]")
{
    Http3Stream s{nullptr, 13};
    const std::string payload = "hello http/3";
    s.on_body(reinterpret_cast<const std::uint8_t*>(payload.data()), payload.size());

    REQUIRE(s.request().body() == payload);
}

TEST_CASE("HTTP/3 on_body concatenates across multiple fragmented deliveries",
          "[http3][stream][unit]")
{
    Http3Stream s{nullptr, 15};
    const std::string first = "hello ";
    const std::string second = "world";

    s.on_body(reinterpret_cast<const std::uint8_t*>(first.data()), first.size());
    s.on_body(reinterpret_cast<const std::uint8_t*>(second.data()), second.size());

    REQUIRE(s.request().body() == "hello world");
    REQUIRE(s.request().body().size() == first.size() + second.size());
}

TEST_CASE("HTTP/3 on_body with zero length is a no-op", "[http3][stream][unit]")
{
    Http3Stream s{nullptr, 17};
    s.on_header(":method", "PUT");

    s.on_body(nullptr, 0);
    s.on_body(reinterpret_cast<const std::uint8_t*>("data"), 0);

    REQUIRE(s.request().body().empty());
    REQUIRE(s.request().method() == HttpMethod::PUT);
}

TEST_CASE("HTTP/3 on_body preserves non-UTF-8 / binary payload byte-for-byte",
          "[http3][stream][unit]")
{
    std::vector<std::uint8_t> bytes;
    bytes.reserve(256);
    for (int i = 0; i < 256; ++i)
    {
        bytes.push_back(static_cast<std::uint8_t>(i));
    }

    Http3Stream s{nullptr, 19};
    s.on_body(bytes.data(), bytes.size());

    auto view = s.request().body();
    REQUIRE(view.size() == bytes.size());
    REQUIRE(std::memcmp(view.data(), bytes.data(), bytes.size()) == 0);
}

TEST_CASE("HTTP/3 on_body correctly accumulates 1000 small chunks (regression: was O(n²))",
          "[http3][stream][unit]")
{
    // This is the regression test for the O(n²) body accumulation bug where
    // each call to on_body() copied the entire accumulated body.
    Http3Stream s{nullptr, 18};
    constexpr int kChunks = 1000;
    const std::string chunk = "ABCDE";  // 5 bytes

    for (int i = 0; i < kChunks; ++i)
        s.on_body(reinterpret_cast<const std::uint8_t*>(chunk.data()), chunk.size());

    REQUIRE(s.request().body().size() == static_cast<size_t>(kChunks) * chunk.size());
    // Verify byte-exact contents
    const auto& body = s.request().body();
    for (int i = 0; i < kChunks; ++i)
        REQUIRE(body.substr(static_cast<size_t>(i) * 5, 5) == chunk);
}

TEST_CASE("HTTP/3 on_end_headers is an observable no-op",
          "[http3][stream][unit]")
{
    Http3Stream s{nullptr, 21};
    s.on_header(":method", "GET");
    s.on_header(":path", "/");
    s.on_header("x-trace", "42");

    // Snapshot state.
    auto before_method = s.request().method();
    auto before_path = s.request().path();
    auto before_trace = s.request().header("x-trace").value_or("");

    s.on_end_headers();

    REQUIRE(s.request().method() == before_method);
    REQUIRE(s.request().path() == before_path);
    REQUIRE(s.request().header("x-trace").value_or("") == std::string(before_trace));
}

TEST_CASE("HTTP/3 full callback sequence (headers -> end_headers -> body) "
          "produces a fully-populated Request",
          "[http3][stream][unit]")
{
    Http3Stream s{nullptr, 23};
    s.on_header(":method", "POST");
    s.on_header(":path", "/upload?id=7");
    s.on_header(":authority", "api.example.com");
    s.on_header("content-type", "application/octet-stream");
    s.on_header("content-length", "11");
    s.on_end_headers();

    const std::string chunk1 = "abcde";
    const std::string chunk2 = "fghijk";
    s.on_body(reinterpret_cast<const std::uint8_t*>(chunk1.data()), chunk1.size());
    s.on_body(reinterpret_cast<const std::uint8_t*>(chunk2.data()), chunk2.size());

    const auto& req = s.request();
    REQUIRE(req.method() == HttpMethod::POST);
    REQUIRE(req.path() == "/upload");
    REQUIRE(req.query_string() == "id=7");
    REQUIRE(req.header("Host").value_or("") == "api.example.com");
    REQUIRE(req.header("content-type").value_or("") == "application/octet-stream");
    REQUIRE(req.header("content-length").value_or("") == "11");
    REQUIRE(req.body() == "abcdefghijk");
}

// ============================================================================
// QuicServer — lifecycle, binding, handler registration (no TLS / no traffic)
// ============================================================================

TEST_CASE("HTTP/3 QuicServer constructs without a TLS context",
          "[http3][server][unit]")
{
    auto ctx = net::IoContext::create(1);
    // tls_ctx defaults to nullptr.
    QuicServer server(*ctx);

    REQUIRE_FALSE(server.has_handler());
    REQUIRE(server.tls_context() == nullptr);
    REQUIRE(server.port() == 0);
}

TEST_CASE("HTTP/3 QuicServer::listen fails without a TLS context",
          "[http3][server][unit]")
{
    // QUIC mandates TLS; ALPN + keying material come from the shared
    // OpenSSL SSL_CTX exposed by TlsContext::native_handle(). A server
    // constructed without TLS should refuse to bind.
    auto ctx = net::IoContext::create(1);
    QuicServer server(*ctx);  // tls_ctx == nullptr

    auto res = server.listen(0);
    REQUIRE_FALSE(res.has_value());
    REQUIRE(res.error().is_io());
    REQUIRE(server.port() == 0);
}

TEST_CASE("HTTP/3 set_handler / has_handler round-trip",
          "[http3][server][unit]")
{
    auto ctx = net::IoContext::create(1);
    QuicServer server(*ctx);

    REQUIRE_FALSE(server.has_handler());

    server.set_handler([](Request) -> Task<Response> {
        Response r;
        r.set_status(200);
        r.set_body("ok");
        co_return r;
    });

    REQUIRE(server.has_handler());
}

// ============================================================================
// QuicConnection — datagram queue behaviour (no TLS / no traffic)
//
// We construct QuicConnection with client_chosen_version = 0, which is an
// invalid QUIC version. ngtcp2_conn_server_new rejects it immediately and the
// constructor returns with conn_ == nullptr. The datagram queue and waiter
// state are independent of conn_, so they work correctly without a live QUIC
// session.
// ============================================================================

// Helper: build a QuicConnection where ngtcp2 init fails gracefully.
static QuicConnection make_test_conn(QuicServer& server)
{
    const uint8_t zero_cid[8] = {};
    net::UdpEndpoint peer{"127.0.0.1", 1234};
    // version 0 is invalid → ngtcp2_conn_server_new fails → conn_ == nullptr.
    return QuicConnection(&server, peer, zero_cid, 8, zero_cid, 8, 0);
}

TEST_CASE("HTTP/3 QuicConnection::on_recv_datagram enqueues a single datagram",
          "[http3][datagram][unit]")
{
    auto ctx = net::IoContext::create(1);
    QuicServer server(*ctx);
    auto conn = make_test_conn(server);

    const uint8_t payload[] = {0xDE, 0xAD, 0xBE, 0xEF};
    conn.on_recv_datagram(payload, sizeof(payload));

    auto t = conn.next_datagram();
    auto result = t.sync_wait();

    REQUIRE(result.has_value());
    REQUIRE(result->size() == 4);
    REQUIRE((*result)[0] == 0xDE);
    REQUIRE((*result)[1] == 0xAD);
    REQUIRE((*result)[2] == 0xBE);
    REQUIRE((*result)[3] == 0xEF);
}

TEST_CASE("HTTP/3 QuicConnection datagram queue preserves FIFO order",
          "[http3][datagram][unit]")
{
    auto ctx = net::IoContext::create(1);
    QuicServer server(*ctx);
    auto conn = make_test_conn(server);

    for (uint8_t i = 0; i < 5; ++i)
        conn.on_recv_datagram(&i, 1);

    for (uint8_t expected = 0; expected < 5; ++expected)
    {
        auto result = conn.next_datagram().sync_wait();
        REQUIRE(result.has_value());
        REQUIRE(result->size() == 1);
        REQUIRE((*result)[0] == expected);
    }
}

TEST_CASE("HTTP/3 QuicConnection datagram queue drops oldest when full",
          "[http3][datagram][unit]")
{
    auto ctx = net::IoContext::create(1);
    QuicServer server(*ctx);
    auto conn = make_test_conn(server);

    // kMaxInboundDatagramQueue == 256. Push one extra (257 total).
    // The first item (value 0) should be evicted to make room for the 257th (value 256).
    constexpr int kOverflow = 257;
    for (int i = 0; i < kOverflow; ++i)
    {
        uint8_t byte = static_cast<uint8_t>(i & 0xFF);
        conn.on_recv_datagram(&byte, 1);
    }

    // Front of the queue should now be the second datagram pushed (value 1).
    auto result = conn.next_datagram().sync_wait();
    REQUIRE(result.has_value());
    REQUIRE(result->size() == 1);
    REQUIRE((*result)[0] == 1);  // 0 was evicted; 1 is now the oldest
}

TEST_CASE("HTTP/3 QuicConnection::send_datagram fails when conn_ is not initialised",
          "[http3][datagram][unit]")
{
    auto ctx = net::IoContext::create(1);
    QuicServer server(*ctx);
    auto conn = make_test_conn(server);

    // conn_ == nullptr because ngtcp2 init failed with version 0.
    const uint8_t payload[] = {0x01, 0x02, 0x03};
    auto res = conn.send_datagram(std::span<const uint8_t>{payload, sizeof(payload)});

    REQUIRE_FALSE(res.has_value());
    REQUIRE(res.error().is_io());
}

// ============================================================================
// ZeroRttReplayCache — unit tests (no TLS / no traffic)
// ============================================================================

TEST_CASE("ZeroRttReplayCache::make_token returns non-empty hex string",
          "[http3][zero_rtt][unit]")
{
    std::string token = ZeroRttReplayCache::make_token("test-scid", "GET", "/api/v1/items", "");
    REQUIRE_FALSE(token.empty());
    // SHA-256 hex is exactly 64 characters.
    REQUIRE(token.size() == 64);
    // All characters must be hex digits.
    for (char c : token)
        REQUIRE(((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f')));
}

TEST_CASE("ZeroRttReplayCache::make_token is deterministic",
          "[http3][zero_rtt][unit]")
{
    auto t1 = ZeroRttReplayCache::make_token("test-scid", "GET", "/search", "");
    auto t2 = ZeroRttReplayCache::make_token("test-scid", "GET", "/search", "");
    REQUIRE(t1 == t2);
}

TEST_CASE("ZeroRttReplayCache::make_token differs across method / path / body",
          "[http3][zero_rtt][unit]")
{
    auto t_get  = ZeroRttReplayCache::make_token("test-scid", "GET",  "/page", "");
    auto t_head = ZeroRttReplayCache::make_token("test-scid", "HEAD", "/page", "");
    auto t_path = ZeroRttReplayCache::make_token("test-scid", "GET",  "/other", "");
    auto t_body = ZeroRttReplayCache::make_token("test-scid", "GET",  "/page", "extra");

    REQUIRE(t_get  != t_head);
    REQUIRE(t_get  != t_path);
    REQUIRE(t_get  != t_body);
    REQUIRE(t_head != t_path);
}

TEST_CASE("ZeroRttReplayCache: first check_and_mark returns false (fresh request)",
          "[http3][zero_rtt][unit]")
{
    auto& cache = ZeroRttReplayCache::instance();
    cache.clear();

    std::string token = ZeroRttReplayCache::make_token("test-scid", "GET", "/fresh", "");
    REQUIRE_FALSE(cache.check_and_mark(token));
}

TEST_CASE("ZeroRttReplayCache: second check_and_mark with same token returns true (replay)",
          "[http3][zero_rtt][unit]")
{
    auto& cache = ZeroRttReplayCache::instance();
    cache.clear();

    std::string token = ZeroRttReplayCache::make_token("test-scid", "GET", "/duplicate", "");
    REQUIRE_FALSE(cache.check_and_mark(token));  // first call: fresh
    REQUIRE(cache.check_and_mark(token));          // second call: replay detected
}

TEST_CASE("ZeroRttReplayCache: different tokens do not collide",
          "[http3][zero_rtt][unit]")
{
    auto& cache = ZeroRttReplayCache::instance();
    cache.clear();

    auto t1 = ZeroRttReplayCache::make_token("test-scid", "GET",  "/alpha", "");
    auto t2 = ZeroRttReplayCache::make_token("test-scid", "GET",  "/beta",  "");
    auto t3 = ZeroRttReplayCache::make_token("test-scid", "HEAD", "/alpha", "");

    REQUIRE_FALSE(cache.check_and_mark(t1));
    REQUIRE_FALSE(cache.check_and_mark(t2));
    REQUIRE_FALSE(cache.check_and_mark(t3));

    // Each is fresh, so re-checking each should now report replay.
    REQUIRE(cache.check_and_mark(t1));
    REQUIRE(cache.check_and_mark(t2));
    REQUIRE(cache.check_and_mark(t3));
}

TEST_CASE("ZeroRttReplayCache: empty token is treated as fresh (no-op guard)",
          "[http3][zero_rtt][unit]")
{
    auto& cache = ZeroRttReplayCache::instance();
    cache.clear();

    // An empty token means hashing failed; per spec we treat it as fresh
    // (can't detect replay) rather than blocking legitimate traffic.
    REQUIRE_FALSE(cache.check_and_mark(""));
    REQUIRE_FALSE(cache.check_and_mark(""));  // still fresh — not tracked
}

TEST_CASE("ZeroRttReplayCache: size increments after fresh tokens",
          "[http3][zero_rtt][unit]")
{
    auto& cache = ZeroRttReplayCache::instance();
    cache.clear();
    REQUIRE(cache.size() == 0);

    (void) cache.check_and_mark(ZeroRttReplayCache::make_token("test-scid", "GET", "/a", ""));
    REQUIRE(cache.size() == 1);
    (void) cache.check_and_mark(ZeroRttReplayCache::make_token("test-scid", "GET", "/b", ""));
    REQUIRE(cache.size() == 2);
    // Replay of /a does NOT increase size.
    (void) cache.check_and_mark(ZeroRttReplayCache::make_token("test-scid", "GET", "/a", ""));
    REQUIRE(cache.size() == 2);
}

#endif  // COROUTE_HAS_HTTP3
