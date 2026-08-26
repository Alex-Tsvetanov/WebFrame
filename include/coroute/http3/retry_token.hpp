#pragma once

// HTTP/3 stateless retry-token: HMAC-SHA256 with server-secret rotation.
//
// RFC 9000 §8.1 requires that the server include a token in a Retry packet and
// verify it when the client echoes it back in a subsequent Initial. The token
// proves that the client controls the claimed source address.
//
// Security model
// --------------
//   A bare random token (the pre-hardening scheme) proves source-address
//   ownership but does not bind the token to a particular server key — an
//   attacker who can observe a valid token on the wire can replay it from
//   their own address. HMAC-SHA256 binds the token to a server-held secret,
//   making the token unforgeable.
//
// Token layout (73 bytes total)
// -----------------------------
//   [0]       1 byte  – epoch (0 or 1; selects current vs. previous secret)
//   [1..8]    8 bytes – timestamp (uint64_t little-endian, UNIX seconds)
//   [9..24]  16 bytes – original DCID (ODCID, zero-padded to 16 bytes)
//   [25..40] 16 bytes – client-IP/port hash (SHA-256(ip:port) first 16 bytes)
//   [41..72] 32 bytes – HMAC-SHA256 tag over bytes [0..40]
//
// Secret rotation
// ---------------
//   The server holds two secrets: `current_secret` and `previous_secret`.
//   New tokens are always issued with the current secret (epoch = current_epoch_).
//   On validation, the verifier first tries the current secret, then falls back to
//   the previous secret. After `rotation_interval_` elapses the current secret
//   rotates to previous; a new random current secret is generated.
//
//   Grace window: the previous secret remains valid for `rotation_interval_` *
//   more seconds, allowing tokens issued just before the rotation to be validated.
//   After the second rotation the previous secret is a new random key and any
//   tokens issued under the now-discarded key will fail — this is acceptable
//   because the QUIC RETRY token TTL (checked at validation time) is far shorter
//   than the rotation interval.
//
// Constant-time comparison
// ------------------------
//   HMAC tag comparison uses CRYPTO_memcmp (OpenSSL / BoringSSL) — not
//   std::memcmp. The function always compares exactly 32 bytes, regardless of
//   where the mismatch is, to prevent timing attacks.
//
// Usage
// -----
//   // At server startup:
//   RetryTokenIssuer issuer;          // generates two random secrets
//
//   // When sending a Retry packet:
//   auto token = issuer.issue(odcid, client_ip_port);
//
//   // When validating the looped-back token from the client's second Initial:
//   bool ok = issuer.validate(token_bytes, odcid, client_ip_port,
//                             max_age_seconds);

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace coroute::http3
{

// ---------------------------------------------------------------------------
// RetryTokenIssuer
//
// Issues and validates HMAC-SHA256 stateless retry tokens.  Not thread-safe
// on its own; the caller (QuicServer::run()) runs on a single IoContext
// coroutine, so no locking is required there. If future refactors make the
// server multi-threaded, add a mutex around issue() / validate() / rotate().
// ---------------------------------------------------------------------------
class RetryTokenIssuer
{
public:
    // Total encoded token size in bytes.
    static constexpr std::size_t kTokenSize = 73;

    // Maximum token age accepted during validation (seconds). Tokens older
    // than this are rejected even if the HMAC is valid, preventing indefinite
    // reuse of captured tokens.
    static constexpr uint64_t kDefaultMaxAgeSecs = 120;

    // Rotation interval: how long the current secret stays current before
    // rotating to previous. Default 1 hour (configurable at construction).
    static constexpr std::chrono::seconds kDefaultRotationInterval{3600};

    // Constructs the issuer and generates two independent random secrets.
    // Rotation uses the wall clock.
    explicit RetryTokenIssuer(
        std::chrono::seconds rotation_interval = kDefaultRotationInterval);

    RetryTokenIssuer(const RetryTokenIssuer&)            = delete;
    RetryTokenIssuer& operator=(const RetryTokenIssuer&) = delete;

    // Issue a new token for a Retry packet.
    //
    //   odcid_data / odcid_len  — original DCID bytes from the client's Initial
    //   client_ip_port          — "<address>:<port>" string, hashed into the token
    //
    // Returns kTokenSize bytes.  Never throws; failures (OpenSSL errors) return
    // an all-zero token — callers should treat zero tokens as "issue failed".
    [[nodiscard]] std::array<uint8_t, kTokenSize> issue(
        const uint8_t* odcid_data, std::size_t odcid_len,
        std::string_view client_ip_port) noexcept;

    // Validate a looped-back token from the client's second Initial packet.
    //
    //   token_data / token_len  — raw bytes from the QUIC packet token field
    //   odcid_data / odcid_len  — original DCID bytes from the client's Initial
    //   client_ip_port          — "<address>:<port>" of the client (must match)
    //   max_age_secs            — reject tokens older than this (default 120 s)
    //
    // Returns true only when:
    //   1. token_len == kTokenSize
    //   2. The HMAC over bytes [0..40] matches the expected value (constant-time)
    //   3. The timestamp is within max_age_secs of now
    //   4. The ODCID and client-IP fields match the provided values
    //
    // May rotate expired secrets as a side effect.
    [[nodiscard]] bool validate(
        const uint8_t* token_data, std::size_t token_len,
        const uint8_t* odcid_data, std::size_t odcid_len,
        std::string_view client_ip_port,
        uint64_t max_age_secs = kDefaultMaxAgeSecs) noexcept;

    // Force a secret rotation immediately (for testing / for controlled rotation
    // from a management interface). Current → previous; new random secret → current.
    // The previous secret is zeroized and replaced.
    void rotate() noexcept;

    // Current epoch value (0 or 1, alternating). Exposed for tests.
    [[nodiscard]] uint8_t current_epoch() const noexcept { return current_epoch_; }

    // True if the current secret has aged past rotation_interval_ and a rotation
    // is due. rotate() is called automatically inside validate() / issue().
    [[nodiscard]] bool rotation_due() const noexcept;

private:
    // HMAC-SHA256 key size.
    static constexpr std::size_t kSecretLen = 32;

    // Generate a new random 32-byte secret into `out`. Returns false if
    // RAND_bytes fails (extremely unlikely; treat as fatal configuration error).
    static bool generate_secret(std::array<uint8_t, kSecretLen>& out) noexcept;

    // Compute HMAC-SHA256 of `plaintext` using `secret`.  Returns false on
    // OpenSSL error.  Output is always exactly 32 bytes.
    [[nodiscard]] static bool hmac_sha256(
        std::span<const uint8_t> secret,
        std::span<const uint8_t> plaintext,
        std::array<uint8_t, 32>& out) noexcept;

    // Build the 41-byte plain-text prefix (epoch + timestamp + odcid + ip_hash).
    [[nodiscard]] static std::array<uint8_t, 41> build_plaintext(
        uint8_t epoch,
        uint64_t timestamp_secs,
        const uint8_t* odcid_data, std::size_t odcid_len,
        std::string_view client_ip_port) noexcept;

    // Rotate secrets if rotation_due(), updating current_epoch_ and
    // resetting last_rotation_.
    void maybe_rotate() noexcept;

    using Secret = std::array<uint8_t, kSecretLen>;

    // Epoch 0 and epoch 1 secrets (indexed by current_epoch_).
    // The current secret is secrets_[current_epoch_].
    // The previous secret is secrets_[1 - current_epoch_].
    std::array<Secret, 2> secrets_{};

    uint8_t current_epoch_ = 0;
    std::chrono::seconds rotation_interval_;
    std::chrono::steady_clock::time_point last_rotation_;
};

}  // namespace coroute::http3
