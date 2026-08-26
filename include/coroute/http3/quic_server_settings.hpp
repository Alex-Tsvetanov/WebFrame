#pragma once

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>

namespace coroute::http3
{

// ---------------------------------------------------------------------------
// Http3ServerSettings
//
// Runtime tunables for the QUIC / HTTP/3 server layer.  Pass an instance to
// QuicServer::set_settings() before calling listen().  All fields have
// production-safe defaults so zero-configuration use is correct out of the box.
// ---------------------------------------------------------------------------
struct Http3ServerSettings
{
    // -----------------------------------------------------------------------
    // Keepalive PING
    //
    // ngtcp2 emits a PING frame after the connection has been idle for this
    // long.  25 s keeps the connection alive through the majority of carrier
    // NAT tables (most idle-out at 30 s).  Set to 0 to disable keepalive.
    // -----------------------------------------------------------------------
    std::chrono::seconds keep_alive_timeout{25};

    // -----------------------------------------------------------------------
    // Stateless Retry (RFC 9000 §8.1)
    //
    // Protects against source-address spoofing: the server challenges any
    // source IP that sends more than `stateless_retry_threshold` Initial
    // packets within a one-second window by responding with a Retry packet
    // instead of allocating connection state.  Disabled by default; enable
    // for Internet-facing deployments that may face Initial-packet floods.
    // -----------------------------------------------------------------------
    bool     enable_stateless_retry{false};
    uint32_t stateless_retry_threshold{10};  // max Initial pkts per IP per second

    // -----------------------------------------------------------------------
    // Preferred Address (RFC 9000 §9.6)
    //
    // When present, the server advertises an alternative address in its
    // transport parameters.  After the handshake the client MAY migrate to
    // the preferred address; the ngtcp2 library handles the client-side path
    // validation transparently.
    //
    // Leave std::nullopt (the default) to omit the preferred_address
    // transport parameter entirely.
    // -----------------------------------------------------------------------
    struct PreferredAddress
    {
        std::string  ipv4_addr;
        uint16_t     ipv4_port{0};
        std::string  ipv6_addr;
        uint16_t     ipv6_port{0};
    };
    std::optional<PreferredAddress> preferred_address;
};

}  // namespace coroute::http3
