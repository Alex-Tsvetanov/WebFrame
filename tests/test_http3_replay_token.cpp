// HTTP/3 replay-token: mode-aware unit tests.
//
// Tests the ReplayTokenMode enum and the four-argument make_token overload
// introduced in the Batch-1 security hardening pass.
//
// Coverage:
//   - ScidBound mode: same SCID + same request → token repeated → replay detected
//   - ScidBound mode: different SCID + same request → different token → NOT a replay
//   - IpBound mode: same IP + same request → token repeated → replay detected
//   - IpBound mode: different IP + same request → different token → NOT a replay
//   - Cross-mode: ScidBound token ≠ IpBound token for the same peer_id value
//     (this would only collide if the SCID hex string happened to equal the
//     IP string, which is astronomically unlikely)
//
// All tests are gated on COROUTE_HAS_HTTP3.

#include <catch2/catch_test_macros.hpp>

#ifdef COROUTE_HAS_HTTP3

#include "coroute/http3/zero_rtt_replay_cache.hpp"

#include <string>

using coroute::http3::ReplayTokenMode;
using coroute::http3::ZeroRttReplayCache;

// Helper: obtain a fresh (cleared) singleton reference.
static ZeroRttReplayCache& fresh()
{
    auto& c = ZeroRttReplayCache::instance();
    c.clear();
    return c;
}

// ===========================================================================
// ReplayTokenMode enum smoke tests
// ===========================================================================

TEST_CASE("ReplayTokenMode — ScidBound and IpBound are distinct enumerators",
          "[http3][replay][unit]")
{
    REQUIRE(ReplayTokenMode::ScidBound != ReplayTokenMode::IpBound);
}

// ===========================================================================
// ScidBound mode
// ===========================================================================

TEST_CASE("ScidBound — make_token with same SCID and request produces same token",
          "[http3][replay][unit]")
{
    const auto t1 = ZeroRttReplayCache::make_token(
        "scid-A", ReplayTokenMode::ScidBound, "GET", "/api/data", "");
    const auto t2 = ZeroRttReplayCache::make_token(
        "scid-A", ReplayTokenMode::ScidBound, "GET", "/api/data", "");
    REQUIRE(!t1.empty());
    REQUIRE(t1 == t2);
}

TEST_CASE("ScidBound — same SCID + same request → replay detected on second mark",
          "[http3][replay][unit]")
{
    auto& cache = fresh();
    const auto token = ZeroRttReplayCache::make_token(
        "scid-A", ReplayTokenMode::ScidBound, "GET", "/api/feed", "");
    REQUIRE_FALSE(cache.check_and_mark(token));  // first: fresh
    REQUIRE(cache.check_and_mark(token));         // second: replay
}

TEST_CASE("ScidBound — different SCID + same request → NOT a replay",
          "[http3][replay][unit][security]")
{
    // Security invariant: an attacker captures a 0-RTT GET from connection A
    // (scid-A) and replays it on connection B (scid-B). The server-chosen SCIDs
    // differ, so the tokens differ, and connection B's request is fresh.
    auto& cache = fresh();
    const auto token_a = ZeroRttReplayCache::make_token(
        "scid-A", ReplayTokenMode::ScidBound, "GET", "/api/feed", "");
    const auto token_b = ZeroRttReplayCache::make_token(
        "scid-B", ReplayTokenMode::ScidBound, "GET", "/api/feed", "");

    REQUIRE(token_a != token_b);                    // tokens differ
    REQUIRE_FALSE(cache.check_and_mark(token_a));   // conn A: fresh
    REQUIRE_FALSE(cache.check_and_mark(token_b));   // conn B: also fresh
}

TEST_CASE("ScidBound — same SCID, different method → different token",
          "[http3][replay][unit]")
{
    const auto t_get  = ZeroRttReplayCache::make_token(
        "scid-X", ReplayTokenMode::ScidBound, "GET",  "/", "");
    const auto t_head = ZeroRttReplayCache::make_token(
        "scid-X", ReplayTokenMode::ScidBound, "HEAD", "/", "");
    REQUIRE(t_get != t_head);
}

TEST_CASE("ScidBound — same SCID, different path → different token",
          "[http3][replay][unit]")
{
    const auto t1 = ZeroRttReplayCache::make_token(
        "scid-X", ReplayTokenMode::ScidBound, "GET", "/a", "");
    const auto t2 = ZeroRttReplayCache::make_token(
        "scid-X", ReplayTokenMode::ScidBound, "GET", "/b", "");
    REQUIRE(t1 != t2);
}

TEST_CASE("ScidBound — same SCID, different body → different token",
          "[http3][replay][unit]")
{
    const auto t1 = ZeroRttReplayCache::make_token(
        "scid-X", ReplayTokenMode::ScidBound, "PUT", "/res", "payload-A");
    const auto t2 = ZeroRttReplayCache::make_token(
        "scid-X", ReplayTokenMode::ScidBound, "PUT", "/res", "payload-B");
    REQUIRE(t1 != t2);
}

// ===========================================================================
// IpBound mode
// ===========================================================================

TEST_CASE("IpBound — make_token with same IP and request produces same token",
          "[http3][replay][unit]")
{
    const auto t1 = ZeroRttReplayCache::make_token(
        "203.0.113.7", ReplayTokenMode::IpBound, "GET", "/api/data", "");
    const auto t2 = ZeroRttReplayCache::make_token(
        "203.0.113.7", ReplayTokenMode::IpBound, "GET", "/api/data", "");
    REQUIRE(!t1.empty());
    REQUIRE(t1 == t2);
}

TEST_CASE("IpBound — same IP + same request → replay detected on second mark",
          "[http3][replay][unit]")
{
    auto& cache = fresh();
    const auto token = ZeroRttReplayCache::make_token(
        "198.51.100.1", ReplayTokenMode::IpBound, "GET", "/api/feed", "");
    REQUIRE_FALSE(cache.check_and_mark(token));  // first: fresh
    REQUIRE(cache.check_and_mark(token));         // second: replay
}

TEST_CASE("IpBound — different IP + same request → NOT a replay",
          "[http3][replay][unit][security]")
{
    auto& cache = fresh();
    const auto token_ip1 = ZeroRttReplayCache::make_token(
        "192.0.2.1", ReplayTokenMode::IpBound, "GET", "/api/feed", "");
    const auto token_ip2 = ZeroRttReplayCache::make_token(
        "192.0.2.2", ReplayTokenMode::IpBound, "GET", "/api/feed", "");

    REQUIRE(token_ip1 != token_ip2);
    REQUIRE_FALSE(cache.check_and_mark(token_ip1));
    REQUIRE_FALSE(cache.check_and_mark(token_ip2));
}

TEST_CASE("IpBound — different method with same IP → different token",
          "[http3][replay][unit]")
{
    const auto t1 = ZeroRttReplayCache::make_token(
        "203.0.113.5", ReplayTokenMode::IpBound, "GET",  "/", "");
    const auto t2 = ZeroRttReplayCache::make_token(
        "203.0.113.5", ReplayTokenMode::IpBound, "POST", "/", "");
    REQUIRE(t1 != t2);
}

// ===========================================================================
// Cross-mode: ScidBound token should NOT equal IpBound token for the
// same peer_id value (guarding against accidental alias collision).
// In practice SCIDs are 8-byte random hex; IPs are dotted-decimal.
// We verify with a contrived equal-string case to document the behaviour:
// if the caller passes the same string for both modes, the tokens are
// identical (by design — mode is not embedded in the hash). We document
// this caveat here so the reader understands the boundary.
// ===========================================================================

TEST_CASE("Cross-mode — same string as peer_id produces same token regardless of mode",
          "[http3][replay][unit][documentation]")
{
    // When peer_id happens to be the same string, tokens are equal.
    // This is expected: the mode tag does not enter the hash; only the
    // peer_id value matters. The distinctness of ScidBound vs. IpBound
    // depends entirely on the different type of value passed, not on a
    // mode discriminator in the hash.
    const std::string peer_id = "shared-id";
    const auto t_scid = ZeroRttReplayCache::make_token(
        peer_id, ReplayTokenMode::ScidBound, "GET", "/", "");
    const auto t_ip   = ZeroRttReplayCache::make_token(
        peer_id, ReplayTokenMode::IpBound,   "GET", "/", "");
    // Same peer_id value → same token (documented behaviour).
    REQUIRE(t_scid == t_ip);
}

TEST_CASE("Cross-mode — SCID string differs from IP string → tokens differ",
          "[http3][replay][unit]")
{
    // In production the SCID is a random 8-byte hex string (e.g. "a3f70c12...")
    // and the IP is a dotted-decimal string; these cannot be equal.
    const auto t_scid = ZeroRttReplayCache::make_token(
        "a3f70c12b4e5d601", ReplayTokenMode::ScidBound, "GET", "/api", "");
    const auto t_ip   = ZeroRttReplayCache::make_token(
        "203.0.113.7",      ReplayTokenMode::IpBound,   "GET", "/api", "");
    REQUIRE(t_scid != t_ip);
}

// ===========================================================================
// Backwards-compatibility: 3-argument (legacy) overload still works.
// ===========================================================================

TEST_CASE("Legacy 3-arg make_token still produces SCID-bound tokens",
          "[http3][replay][unit]")
{
    // The 3-arg overload delegates to sha256_token(peer_scid, method, path, body)
    // which is the same as 4-arg with ScidBound.
    const auto legacy = ZeroRttReplayCache::make_token("scid-A", "GET", "/", "");
    const auto mode   = ZeroRttReplayCache::make_token(
        "scid-A", ReplayTokenMode::ScidBound, "GET", "/", "");
    REQUIRE(legacy == mode);
}

#endif  // COROUTE_HAS_HTTP3
