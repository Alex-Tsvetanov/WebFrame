#pragma once

// Server-side 0-RTT replay-detection cache.
//
// RFC 8446 §8.1 mandates that servers implement replay detection before
// accepting 0-RTT early data. This cache tracks a hash of each 0-RTT
// request within a time window. A request hash seen more than once within
// the window is a replay and must be rejected.
//
// Design decisions:
//   - Two keying modes, selected via `ReplayTokenMode`:
//
//     ScidBound (default):
//       SHA-256(peer_scid || '\0' || method || '\0' || path || '\0' || body)
//       peer_scid is the server-chosen SCID for this QUIC connection, which is
//       unique per connection. Binding to it ensures cross-connection replays
//       (an attacker capturing a 0-RTT request from connection A and replaying
//       it from connection B) produce a distinct token that the cache has never
//       seen, correctly rejecting the replay. Body is included so clients that
//       vary the body on the same path (e.g., idempotent PUT) get protection too.
//
//     IpBound:
//       SHA-256(peer_ip || '\0' || method || '\0' || path || '\0' || body)
//       Binds the token to the peer's source IP address instead of the SCID.
//       Useful in deployments where the SCID is not reliably available at the
//       replay-check call site (e.g., load-balancer configurations that strip
//       QUIC connection IDs before forwarding). Weaker than ScidBound: an
//       attacker who can spoof the same source IP passes the check.
//
//   - Time window: `kWindowSeconds` (default 60 s). Entries older than the
//     window are eligible for eviction on the next `check_and_mark` call.
//   - Eviction is amortized: every 256 insertions we sweep the map and drop
//     expired entries. No background thread required.
//   - Thread-safe: protected by a single mutex.
//   - Process-wide singleton via `instance()`.
//
// Scope / limitations:
//   - In-memory only. Replays are only detected within the same process
//     lifetime. Across restarts / across server-cluster nodes, a
//     distributed token store (Redis, etc.) is needed — out of scope here.
//   - Replay window is intentionally short (60 s) to bound memory use.

#include <array>
#include <chrono>
#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>

namespace coroute::http3
{

// ---------------------------------------------------------------------------
// ReplayTokenMode — controls how the replay-token cache key is bound.
//
// ScidBound (default): binds the token to the QUIC peer SCID. A request
//   captured from connection A (scid-A) and replayed from connection B
//   (scid-B) has a different token and is correctly treated as fresh on B,
//   while the same request replayed again on A is detected as a replay.
//
// IpBound: binds the token to the peer IP address string. Weaker than
//   ScidBound — an attacker who can forge the source IP can bypass the
//   check — but usable where the SCID is unavailable at the call site.
// ---------------------------------------------------------------------------
enum class ReplayTokenMode
{
    ScidBound,  // SHA-256(peer_scid || NUL || method || NUL || path || NUL || body)
    IpBound,    // SHA-256(peer_ip   || NUL || method || NUL || path || NUL || body)
};

class ZeroRttReplayCache
{
public:
	// Process-wide singleton.
	static ZeroRttReplayCache& instance();

	ZeroRttReplayCache(const ZeroRttReplayCache&)            = delete;
	ZeroRttReplayCache& operator=(const ZeroRttReplayCache&) = delete;

	// Check whether `token` has been seen before within the replay window.
	// Returns true if this is a replay (the token was already registered).
	// Returns false if the token is fresh — and registers it so subsequent
	// calls with the same token return true.
	//
	// `token` is typically produced by `make_token(peer_id, mode, method, path, body)`.
	[[nodiscard]] bool check_and_mark(const std::string& token);

	// Build the canonical replay token for a request.
	//
	// `peer_id` is the SCID (for ScidBound mode) or the peer IP address string
	// (for IpBound mode). The token is a hex-encoded SHA-256 digest of:
	//   peer_id + '\0' + method + '\0' + path + '\0' + body
	//
	// The `mode` parameter is not embedded into the hash — it only controls
	// what the caller passes as `peer_id`. This is intentional: tokens produced
	// under different modes for the same (peer_id, request) tuple are distinct
	// because the peer_id value itself differs.
	//
	// Returns an empty string if the hashing step fails (rare — treat as
	// "token unavailable", skip replay check).
	[[nodiscard]] static std::string make_token(std::string_view peer_id,
	                                             ReplayTokenMode  mode,
	                                             std::string_view method,
	                                             std::string_view path,
	                                             std::string_view body);

	// Convenience overload retained for call sites that pass a pre-computed
	// peer_scid directly (ScidBound semantics assumed). Callers that need
	// IpBound should use the four-argument form and pass the IP string.
	[[nodiscard]] static std::string make_token(std::string_view peer_scid,
	                                             std::string_view method,
	                                             std::string_view path,
	                                             std::string_view body);

	// Diagnostic / test helpers.
	[[nodiscard]] std::size_t size() const noexcept;
	void                       clear();

	// Window size in seconds. Entries older than this are eligible for eviction.
	static constexpr int kWindowSeconds = 60;

private:
	ZeroRttReplayCache() = default;

	void sweep_expired_locked();

	struct Entry
	{
		std::chrono::steady_clock::time_point expires;
	};

	mutable std::mutex                        mu_;
	std::unordered_map<std::string, Entry>    seen_;
	std::uint32_t                             insert_count_ = 0;

	// Sweep every N insertions to bound memory without a background thread.
	static constexpr std::uint32_t kSweepInterval = 256;
};

}  // namespace coroute::http3
