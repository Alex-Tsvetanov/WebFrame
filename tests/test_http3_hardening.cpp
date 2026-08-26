// HTTP/3 production-hardening unit tests.
//
// Covers: Http3ServerSettings defaults, preferred-address construction,
// keepalive-timeout range validation, and stateless-retry settings.
//
// All tests are gated on COROUTE_HAS_HTTP3.

#include <catch2/catch_test_macros.hpp>

#ifdef COROUTE_HAS_HTTP3

#include "coroute/http3/quic_server_settings.hpp"

#include <chrono>
#include <optional>
#include <string>

using namespace coroute::http3;

// ============================================================================
// Http3ServerSettings — defaults
// ============================================================================

TEST_CASE("Http3ServerSettings default keep_alive_timeout is 25 s",
          "[http3][hardening][unit]")
{
    Http3ServerSettings s;
    REQUIRE(s.keep_alive_timeout == std::chrono::seconds{25});
}

TEST_CASE("Http3ServerSettings default stateless retry is disabled",
          "[http3][hardening][unit]")
{
    Http3ServerSettings s;
    REQUIRE_FALSE(s.enable_stateless_retry);
}

TEST_CASE("Http3ServerSettings default stateless retry threshold is 10",
          "[http3][hardening][unit]")
{
    Http3ServerSettings s;
    REQUIRE(s.stateless_retry_threshold == 10u);
}

TEST_CASE("Http3ServerSettings default preferred_address is absent",
          "[http3][hardening][unit]")
{
    Http3ServerSettings s;
    REQUIRE_FALSE(s.preferred_address.has_value());
}

// ============================================================================
// Http3ServerSettings — keepalive range
// ============================================================================

TEST_CASE("Http3ServerSettings keep_alive_timeout is positive",
          "[http3][hardening][unit]")
{
    Http3ServerSettings s;
    REQUIRE(s.keep_alive_timeout.count() > 0);
}

TEST_CASE("Http3ServerSettings keep_alive_timeout is below 60 s",
          "[http3][hardening][unit]")
{
    Http3ServerSettings s;
    REQUIRE(s.keep_alive_timeout < std::chrono::seconds{60});
}

TEST_CASE("Http3ServerSettings keep_alive_timeout below carrier NAT idle window (30 s)",
          "[http3][hardening][unit]")
{
    // The default of 25 s is deliberately below the typical 30 s carrier
    // NAT idle-port mapping timeout so PINGs keep the path alive.
    Http3ServerSettings s;
    REQUIRE(s.keep_alive_timeout < std::chrono::seconds{30});
}

TEST_CASE("Http3ServerSettings keep_alive_timeout can be disabled with 0",
          "[http3][hardening][unit]")
{
    Http3ServerSettings s;
    s.keep_alive_timeout = std::chrono::seconds{0};
    REQUIRE(s.keep_alive_timeout.count() == 0);
}

// ============================================================================
// Http3ServerSettings — preferred-address construction
// ============================================================================

TEST_CASE("Http3ServerSettings preferred_address can be set with IPv4",
          "[http3][hardening][unit]")
{
    Http3ServerSettings s;
    s.preferred_address = Http3ServerSettings::PreferredAddress{
        .ipv4_addr = "192.0.2.1",
        .ipv4_port = 4433,
        .ipv6_addr = {},
        .ipv6_port = 0,
    };

    REQUIRE(s.preferred_address.has_value());
    REQUIRE(s.preferred_address->ipv4_addr == "192.0.2.1");
    REQUIRE(s.preferred_address->ipv4_port == 4433u);
    REQUIRE(s.preferred_address->ipv6_addr.empty());
}

TEST_CASE("Http3ServerSettings preferred_address can be set with IPv6",
          "[http3][hardening][unit]")
{
    Http3ServerSettings s;
    s.preferred_address = Http3ServerSettings::PreferredAddress{
        .ipv4_addr = {},
        .ipv4_port = 0,
        .ipv6_addr = "2001:db8::1",
        .ipv6_port = 4433,
    };

    REQUIRE(s.preferred_address.has_value());
    REQUIRE(s.preferred_address->ipv6_addr == "2001:db8::1");
    REQUIRE(s.preferred_address->ipv6_port == 4433u);
}

TEST_CASE("Http3ServerSettings preferred_address can be set with both families",
          "[http3][hardening][unit]")
{
    Http3ServerSettings s;
    s.preferred_address = Http3ServerSettings::PreferredAddress{
        .ipv4_addr = "198.51.100.5",
        .ipv4_port = 443,
        .ipv6_addr = "2001:db8::dead:beef",
        .ipv6_port = 443,
    };

    REQUIRE(s.preferred_address.has_value());
    REQUIRE(s.preferred_address->ipv4_addr == "198.51.100.5");
    REQUIRE(s.preferred_address->ipv4_port == 443u);
    REQUIRE(s.preferred_address->ipv6_addr == "2001:db8::dead:beef");
    REQUIRE(s.preferred_address->ipv6_port == 443u);
}

TEST_CASE("Http3ServerSettings preferred_address can be cleared",
          "[http3][hardening][unit]")
{
    Http3ServerSettings s;
    s.preferred_address = Http3ServerSettings::PreferredAddress{
        .ipv4_addr = "192.0.2.1",
        .ipv4_port = 4433,
    };
    REQUIRE(s.preferred_address.has_value());

    s.preferred_address = std::nullopt;
    REQUIRE_FALSE(s.preferred_address.has_value());
}

// ============================================================================
// Http3ServerSettings — stateless retry tuning
// ============================================================================

TEST_CASE("Http3ServerSettings stateless retry can be enabled",
          "[http3][hardening][unit]")
{
    Http3ServerSettings s;
    s.enable_stateless_retry = true;
    REQUIRE(s.enable_stateless_retry);
}

TEST_CASE("Http3ServerSettings stateless retry threshold can be raised",
          "[http3][hardening][unit]")
{
    Http3ServerSettings s;
    s.stateless_retry_threshold = 100u;
    REQUIRE(s.stateless_retry_threshold == 100u);
}

TEST_CASE("Http3ServerSettings stateless retry threshold can be set to 1 for strict mode",
          "[http3][hardening][unit]")
{
    Http3ServerSettings s;
    s.enable_stateless_retry       = true;
    s.stateless_retry_threshold    = 1u;
    REQUIRE(s.enable_stateless_retry);
    REQUIRE(s.stateless_retry_threshold == 1u);
}

// ============================================================================
// Http3ServerSettings — copy / move semantics
// ============================================================================

TEST_CASE("Http3ServerSettings is copy-constructible",
          "[http3][hardening][unit]")
{
    Http3ServerSettings a;
    a.keep_alive_timeout       = std::chrono::seconds{15};
    a.enable_stateless_retry   = true;
    a.stateless_retry_threshold = 5u;
    a.preferred_address = Http3ServerSettings::PreferredAddress{
        .ipv4_addr = "10.0.0.1",
        .ipv4_port = 8443,
    };

    Http3ServerSettings b = a;

    REQUIRE(b.keep_alive_timeout == std::chrono::seconds{15});
    REQUIRE(b.enable_stateless_retry);
    REQUIRE(b.stateless_retry_threshold == 5u);
    REQUIRE(b.preferred_address.has_value());
    REQUIRE(b.preferred_address->ipv4_addr == "10.0.0.1");
}

TEST_CASE("Http3ServerSettings is move-constructible",
          "[http3][hardening][unit]")
{
    Http3ServerSettings a;
    a.keep_alive_timeout = std::chrono::seconds{10};
    a.preferred_address  = Http3ServerSettings::PreferredAddress{
        .ipv4_addr = "10.0.0.2",
        .ipv4_port = 8444,
    };

    Http3ServerSettings b = std::move(a);

    REQUIRE(b.keep_alive_timeout == std::chrono::seconds{10});
    REQUIRE(b.preferred_address.has_value());
    REQUIRE(b.preferred_address->ipv4_addr == "10.0.0.2");
}

#endif  // COROUTE_HAS_HTTP3
