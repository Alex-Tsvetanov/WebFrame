#include "coroute/http3/zero_rtt_replay_cache.hpp"

#include <openssl/evp.h>
#include <openssl/sha.h>

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <mutex>

namespace coroute::http3
{

ZeroRttReplayCache& ZeroRttReplayCache::instance()
{
	static ZeroRttReplayCache cache;
	return cache;
}

// Internal helper: SHA-256(peer_id NUL method NUL path NUL body) → hex string.
// `peer_id` is the SCID (ScidBound) or IP address string (IpBound); the
// caller is responsible for passing the correct value for the chosen mode.
static std::string sha256_token(std::string_view peer_id,
                                 std::string_view method,
                                 std::string_view path,
                                 std::string_view body)
{
	EVP_MD_CTX* ctx = EVP_MD_CTX_new();
	if (!ctx) return {};

	if (EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr) != 1)
	{
		EVP_MD_CTX_free(ctx);
		return {};
	}

	// Feed: peer_id NUL method NUL path NUL body
	EVP_DigestUpdate(ctx, peer_id.data(), peer_id.size());
	const char nul = '\0';
	EVP_DigestUpdate(ctx, &nul, 1);
	EVP_DigestUpdate(ctx, method.data(), method.size());
	EVP_DigestUpdate(ctx, &nul, 1);
	EVP_DigestUpdate(ctx, path.data(), path.size());
	EVP_DigestUpdate(ctx, &nul, 1);
	EVP_DigestUpdate(ctx, body.data(), body.size());

	std::array<unsigned char, SHA256_DIGEST_LENGTH> digest{};
	unsigned int digest_len = 0;
	if (EVP_DigestFinal_ex(ctx, digest.data(), &digest_len) != 1)
	{
		EVP_MD_CTX_free(ctx);
		return {};
	}
	EVP_MD_CTX_free(ctx);

	// Hex-encode the digest.
	std::string hex;
	hex.resize(digest_len * 2);
	for (unsigned int i = 0; i < digest_len; ++i)
	{
		std::snprintf(&hex[i * 2], 3, "%02x", static_cast<unsigned>(digest[i]));
	}
	return hex;
}

std::string ZeroRttReplayCache::make_token(std::string_view peer_id,
                                            ReplayTokenMode  mode,
                                            std::string_view method,
                                            std::string_view path,
                                            std::string_view body)
{
	// `mode` controls what the caller passes as `peer_id` (SCID vs. IP).
	// The hash input is identical in both cases — what differs is only the
	// semantic meaning and therefore the value of `peer_id`. We do NOT embed
	// the mode tag into the hash: a ScidBound token and an IpBound token for
	// the same (peer_id_value, request) will collide if the peer_id strings
	// happen to be equal, but that is astronomically unlikely in practice
	// (SCIDs are random 8-byte values; IPs are dotted-decimal strings).
	(void) mode;  // mode is enforced by the caller choosing the peer_id value
	return sha256_token(peer_id, method, path, body);
}

// Convenience three-argument overload: ScidBound semantics (peer_scid passed directly).
// Retained for existing call sites that do not need to specify a mode explicitly.
std::string ZeroRttReplayCache::make_token(std::string_view peer_scid,
                                            std::string_view method,
                                            std::string_view path,
                                            std::string_view body)
{
	// Binding peer_scid to the token makes it connection-specific: a request
	// captured from connection A and replayed from connection B has a
	// different peer_scid and therefore a different token that is not in the
	// cache, so the cross-connection replay is correctly detected as fresh.
	return sha256_token(peer_scid, method, path, body);
}

bool ZeroRttReplayCache::check_and_mark(const std::string& token)
{
	if (token.empty()) return false;  // no token — can't detect, treat as fresh

	const auto now    = std::chrono::steady_clock::now();
	const auto expiry = now + std::chrono::seconds{kWindowSeconds};

	std::lock_guard<std::mutex> lk(mu_);

	auto it = seen_.find(token);
	if (it != seen_.end() && it->second.expires > now)
		return true;  // replay: same token seen recently

	// Register the token (fresh or expired entry).
	seen_[token] = {expiry};

	if (++insert_count_ >= kSweepInterval)
	{
		insert_count_ = 0;
		sweep_expired_locked();
	}

	return false;
}

void ZeroRttReplayCache::sweep_expired_locked()
{
	const auto now = std::chrono::steady_clock::now();
	for (auto it = seen_.begin(); it != seen_.end(); )
	{
		if (it->second.expires <= now)
			it = seen_.erase(it);
		else
			++it;
	}
}

std::size_t ZeroRttReplayCache::size() const noexcept
{
	std::lock_guard<std::mutex> lk(mu_);
	return seen_.size();
}

void ZeroRttReplayCache::clear()
{
	std::lock_guard<std::mutex> lk(mu_);
	seen_.clear();
	insert_count_ = 0;
}

}  // namespace coroute::http3
