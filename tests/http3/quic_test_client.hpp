// QUIC + HTTP/3 in-process TEST CLIENT for coroute.
//
// Scope: unit-test helper ONLY. Speaks just enough QUIC (RFC 9000) and HTTP/3
// (RFC 9114) to perform a TLS handshake, issue one or more requests against a
// coroute::http3::QuicServer in the SAME process / SAME IoContext, and read
// the responses. Not fit for production, not a public API.
//
// Header hygiene (critical):
//   * This header MUST NOT include <winsock2.h> or any <openssl/*>. winsock2
//     #define DELETE collides with coroute::HttpMethod::DELETE. All platform
//     and OpenSSL/ngtcp2/nghttp3 includes live in the .cpp.
//   * SSL / SSL_CTX forward declared as opaque struct types.
//   * Peer sockaddr stored as an opaque 128-byte aligned byte array.
//
// Adapted patterns (not copied wholesale) from ngtcp2 upstream example client:
//   https://raw.githubusercontent.com/ngtcp2/ngtcp2/v1.21.0/examples/client.cc
//   (MIT licensed).

#pragma once

#include "coroute/net/io_context.hpp"
#include "coroute/coro/task.hpp"
#include "coroute/core/error.hpp"
#include "coroute/util/expected.hpp"

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>

// Forward declarations for ngtcp2 / nghttp3 / OpenSSL (kept out of the header
// per the header hygiene rule above).
struct ngtcp2_conn;
struct nghttp3_conn;
struct ssl_st;
struct ssl_ctx_st;
typedef struct ssl_st SSL;           // NOLINT(readability-identifier-naming)
typedef struct ssl_ctx_st SSL_CTX;   // NOLINT(readability-identifier-naming)

namespace coroute::http3::test {

struct TestResponse
{
    int status{0};
    std::unordered_map<std::string, std::string> headers;
    std::string body;
};

class QuicTestClient
{
public:
    QuicTestClient(net::IoContext& ctx, const std::string& peer_host, uint16_t peer_port);
    ~QuicTestClient();

    QuicTestClient(const QuicTestClient&) = delete;
    QuicTestClient& operator=(const QuicTestClient&) = delete;
    QuicTestClient(QuicTestClient&&) = delete;
    QuicTestClient& operator=(QuicTestClient&&) = delete;

    // Perform UDP bind, QUIC handshake, and HTTP/3 SETTINGS exchange.
    // Suspends until handshake_confirmed or timeout.
    Task<expected<void, Error>> connect(std::chrono::milliseconds timeout = std::chrono::seconds(2));

    // Issue an HTTP/3 request over a new bidi stream and await the response.
    Task<expected<TestResponse, Error>> request(std::string method,
                                                std::string path,
                                                std::unordered_map<std::string, std::string> headers = {},
                                                std::string body = {});

    // Convenience wrapper.
    Task<expected<TestResponse, Error>> get(std::string path)
    {
        return request("GET", std::move(path), {}, {});
    }

    // Graceful close: sends CONNECTION_CLOSE and drains the last packet.
    Task<void> close();

    // Send a QUIC DATAGRAM frame (RFC 9221) to the peer.
    // Returns error if the connection is not open, the peer did not advertise
    // max_datagram_frame_size, or ngtcp2 rejects the frame.
    Task<expected<void, Error>> send_datagram(std::vector<uint8_t> payload);

    // Suspend until the next inbound DATAGRAM frame arrives from the server,
    // or until `timeout` elapses. Returns nullopt on timeout or connection close.
    Task<std::optional<std::vector<uint8_t>>> next_datagram(
        std::chrono::milliseconds timeout = std::chrono::seconds(2));

private:
    // Opaque storage for sockaddr_storage — we cast to sockaddr_storage inside
    // the .cpp. 128 bytes + 16-byte alignment is a safe over-approximation on
    // every platform coroute targets.
    using SockAddrBuf = std::array<std::byte, 128>;

    // Forward-declared impl lives in the .cpp.
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace coroute::http3::test
