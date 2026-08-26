// HTTP/3 end-to-end tests — QUIC handshake + request/response using the
// in-process QuicTestClient against a real QuicServer (same IoContext).
//
// All tests gated on COROUTE_HAS_HTTP3 and require a pre-generated self-signed
// cert pair (path supplied via CMake: COROUTE_TEST_CERT_FILE / COROUTE_TEST_KEY_FILE).

#include <catch2/catch_test_macros.hpp>

#ifdef COROUTE_HAS_HTTP3

#include "coroute/http3/quic_server.hpp"
#include "coroute/net/tls.hpp"
#include "coroute/net/io_context.hpp"
#include "coroute/core/request.hpp"
#include "coroute/core/response.hpp"
#include "coroute/coro/task.hpp"

#include "quic_test_client.hpp"

#include <atomic>
#include <chrono>
#include <memory>
#include <optional>
#include <string>

using namespace coroute;
using namespace coroute::http3;
using namespace std::chrono_literals;

namespace
{

// Build a TlsContext from the checked-in self-signed cert/key. The cert was
// originally minted with: `openssl req -x509 -newkey rsa:4096 -keyout key.pem
// -out cert.pem -days 365 -nodes`. Suitable for loopback test traffic only.
std::unique_ptr<net::TlsContext> make_test_tls_context()
{
    net::TlsConfig cfg;
    cfg.cert_file = COROUTE_TEST_CERT_FILE;
    cfg.key_file = COROUTE_TEST_KEY_FILE;
    cfg.min_version = net::TlsConfig::MinVersion::TLS_1_3;
    cfg.alpn_protocols = {"h3"};

    auto res = net::TlsContext::create(cfg);
    if (!res) return nullptr;
    return std::make_unique<net::TlsContext>(std::move(*res));
}

}  // namespace

// ============================================================================
// E2E — the QUIC handshake path
// ============================================================================

TEST_CASE("HTTP/3 E2E: client handshake completes against a real QuicServer",
          "[http3][e2e][integration]")
{
    auto tls_ctx = make_test_tls_context();
    if (!tls_ctx)
    {
        WARN("Test TLS cert not available at " COROUTE_TEST_CERT_FILE " — skipping");
        SUCCEED();
        return;
    }

    auto ctx = net::IoContext::create(1);

    QuicServer server(*ctx, tls_ctx.get());
    server.set_handler([](Request req) -> Task<Response> {
        Response r;
        r.set_status(200);
        r.set_body("ok:" + std::string{req.path()});
        co_return r;
    });

    auto listen_res = server.listen(0);
    REQUIRE(listen_res.has_value());
    const uint16_t server_port = server.port();
    REQUIRE(server_port > 0);

    std::atomic<bool> got_response{false};
    std::optional<test::TestResponse> received;
    std::optional<Error> error;
    std::atomic<bool> watchdog_fired{false};

    auto driver = [&]() -> Task<void> {
        server.run().start_detached();

        test::QuicTestClient client(*ctx, "127.0.0.1", server_port);

        auto connect_res = co_await client.connect(5s);
        if (!connect_res)
        {
            error = connect_res.error();
            if (!watchdog_fired) ctx->stop();
            co_return;
        }

        auto resp = co_await client.get("/hello");
        if (!resp)
        {
            error = resp.error();
        }
        else
        {
            received = std::move(*resp);
            got_response = true;
        }

        co_await client.close();
        if (!watchdog_fired) ctx->stop();
    };

    // Hard watchdog: guarantee the ctx loop unblocks within 10s no matter
    // what so a hung exchange cannot block the whole test suite.
    ctx->schedule(10s, [&]() {
        watchdog_fired = true;
        ctx->stop();
    });

    ctx->post([&]() { driver().start_detached(); });
    ctx->run();

    INFO("watchdog_fired=" << watchdog_fired.load()
         << " got_response=" << got_response.load()
         << (error ? (" error=" + error->to_string()) : std::string{}));
    REQUIRE(got_response.load());
    REQUIRE(received.has_value());
    CHECK(received->status == 200);
    CHECK(received->body == "ok:/hello");
}

#endif  // COROUTE_HAS_HTTP3
