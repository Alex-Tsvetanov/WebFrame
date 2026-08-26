// HTTP/3 stateless retry-token: HMAC-SHA256 with server-secret rotation.
//
// See include/coroute/http3/retry_token.hpp for the full design description.

#ifdef _WIN32
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  ifndef NOMINMAX
#    define NOMINMAX
#  endif
#  include <winsock2.h>
#  include <ws2tcpip.h>
#endif

// Belt-and-braces: prevent any leaked DELETE macro from winsock2 propagating.
#ifdef DELETE
#  undef DELETE
#endif

#include <openssl/hmac.h>
#include <openssl/rand.h>
#include <openssl/sha.h>

#include "coroute/http3/retry_token.hpp"

#include <algorithm>
#include <cstring>
#include <ctime>

namespace coroute::http3
{

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

RetryTokenIssuer::RetryTokenIssuer(std::chrono::seconds rotation_interval)
    : rotation_interval_{rotation_interval}
    , last_rotation_{std::chrono::steady_clock::now()}
{
    // Generate two independent random secrets so we have a valid "previous"
    // secret from the moment the issuer comes alive.
    generate_secret(secrets_[0]);
    generate_secret(secrets_[1]);
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

std::array<uint8_t, RetryTokenIssuer::kTokenSize>
RetryTokenIssuer::issue(const uint8_t* odcid_data, std::size_t odcid_len,
                        std::string_view client_ip_port) noexcept
{
    maybe_rotate();

    const uint64_t ts =
        static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count());

    const std::array<uint8_t, 41> plain =
        build_plaintext(current_epoch_, ts, odcid_data, odcid_len, client_ip_port);

    std::array<uint8_t, 32> tag{};
    if (!hmac_sha256({secrets_[current_epoch_].data(), kSecretLen},
                     {plain.data(), plain.size()}, tag))
    {
        // HMAC failure is extremely rare; return an all-zero token so the
        // caller can detect the failure and skip sending the Retry packet.
        return {};
    }

    std::array<uint8_t, kTokenSize> token{};
    std::memcpy(token.data(),      plain.data(), 41);
    std::memcpy(token.data() + 41, tag.data(),   32);
    return token;
}

bool RetryTokenIssuer::validate(const uint8_t* token_data, std::size_t token_len,
                                const uint8_t* odcid_data, std::size_t odcid_len,
                                std::string_view client_ip_port,
                                uint64_t max_age_secs) noexcept
{
    maybe_rotate();

    if (token_len != kTokenSize) return false;

    const uint8_t  epoch_byte   = token_data[0];
    const uint8_t* token_plain  = token_data;       // first 41 bytes are plain-text
    const uint8_t* token_tag    = token_data + 41;  // last 32 bytes are the HMAC tag

    // Only epochs 0 and 1 are valid.
    if (epoch_byte > 1) return false;

    // Reconstruct the expected HMAC tag using the secret for the token's epoch.
    const Secret& secret = secrets_[epoch_byte];
    std::array<uint8_t, 32> expected_tag{};
    if (!hmac_sha256({secret.data(), kSecretLen},
                     {token_plain, 41}, expected_tag))
    {
        return false;
    }

    // Constant-time comparison — CRYPTO_memcmp returns 0 iff equal.
    if (CRYPTO_memcmp(expected_tag.data(), token_tag, 32) != 0) return false;

    // Extract timestamp (bytes [1..8], little-endian).
    uint64_t ts = 0;
    std::memcpy(&ts, token_data + 1, sizeof(ts));

    // Validate token age.
    const uint64_t now =
        static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count());

    if (now < ts) return false;  // clock skew / wraparound guard
    if (now - ts > max_age_secs) return false;

    // Rebuild the expected plain-text and compare the ODCID + IP fields.
    // This catches token reuse across different clients or connections.
    const std::array<uint8_t, 41> expected_plain =
        build_plaintext(epoch_byte, ts, odcid_data, odcid_len, client_ip_port);

    // Constant-time comparison of the full 41-byte plain-text prefix.
    return CRYPTO_memcmp(expected_plain.data(), token_plain, 41) == 0;
}

void RetryTokenIssuer::rotate() noexcept
{
    // Move current epoch to "previous" by generating a new secret for the
    // current epoch slot and flipping the epoch.
    //
    //   Before: current_epoch_ = 0, secrets_[0] = current, secrets_[1] = previous
    //   After:  current_epoch_ = 1, secrets_[1] = new current, secrets_[0] = old current
    //
    // This way secrets_[old_epoch] (= old current) becomes the new "previous",
    // and the previously-previous secret (secrets_[new_epoch] before regeneration)
    // is overwritten — effectively zeroized and replaced.
    const uint8_t new_epoch = static_cast<uint8_t>(1u - current_epoch_);
    generate_secret(secrets_[new_epoch]);
    current_epoch_ = new_epoch;
    last_rotation_ = std::chrono::steady_clock::now();
}

bool RetryTokenIssuer::rotation_due() const noexcept
{
    const auto now     = std::chrono::steady_clock::now();
    const auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - last_rotation_);
    return elapsed >= rotation_interval_;
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

bool RetryTokenIssuer::generate_secret(std::array<uint8_t, kSecretLen>& out) noexcept
{
    return RAND_bytes(out.data(), static_cast<int>(kSecretLen)) == 1;
}

bool RetryTokenIssuer::hmac_sha256(std::span<const uint8_t> secret,
                                   std::span<const uint8_t> plaintext,
                                   std::array<uint8_t, 32>& out) noexcept
{
    unsigned int out_len = 0;
    const uint8_t* result = HMAC(
        EVP_sha256(),
        secret.data(), static_cast<int>(secret.size()),
        plaintext.data(), plaintext.size(),
        out.data(), &out_len);
    return result != nullptr && out_len == 32;
}

// Build the 41-byte authenticated plain-text prefix:
//   [0]      1 byte  epoch
//   [1..8]   8 bytes timestamp (little-endian uint64_t)
//   [9..24]  16 bytes ODCID (zero-padded / truncated to 16 bytes)
//   [25..40] 16 bytes first 16 bytes of SHA-256(client_ip_port)
std::array<uint8_t, 41>
RetryTokenIssuer::build_plaintext(uint8_t epoch, uint64_t timestamp_secs,
                                  const uint8_t* odcid_data, std::size_t odcid_len,
                                  std::string_view client_ip_port) noexcept
{
    std::array<uint8_t, 41> out{};
    out[0] = epoch;
    std::memcpy(&out[1], &timestamp_secs, sizeof(timestamp_secs));  // little-endian

    // ODCID: copy up to 16 bytes, zero-pad if shorter, truncate if longer.
    constexpr std::size_t kOdcidField = 16;
    const std::size_t copy_len = std::min(odcid_len, kOdcidField);
    std::memcpy(&out[9], odcid_data, copy_len);
    // remaining bytes are already zero from the aggregate initialiser

    // Client-IP/port hash: SHA-256 of the "<ip>:<port>" string, first 16 bytes.
    std::array<uint8_t, SHA256_DIGEST_LENGTH> ip_hash{};
    SHA256(reinterpret_cast<const uint8_t*>(client_ip_port.data()),
           client_ip_port.size(), ip_hash.data());
    std::memcpy(&out[25], ip_hash.data(), 16);

    return out;
}

void RetryTokenIssuer::maybe_rotate() noexcept
{
    if (rotation_due()) rotate();
}

}  // namespace coroute::http3
