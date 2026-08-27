#include "coroute/http3/stateless.hpp"

#ifdef COROUTE_HAS_HTTP3

#include <ngtcp2/ngtcp2.h>
#include <openssl/hmac.h>
#include <openssl/rand.h>

#include <algorithm>
#include <cstring>

namespace coroute::http3
{

	namespace
	{
		// Versions this server actually speaks. ngtcp2 can also do QUIC v2, but the
		// connection setup here configures v1, and offering a version the handshake
		// would then refuse is worse than not offering it.
		constexpr std::array<std::uint32_t, 1> supported_versions{NGTCP2_PROTO_VER_V1};
	}  // namespace

	std::size_t write_version_negotiation(std::span<std::uint8_t> out, const CidKey& client_dcid,
	                                      const CidKey& client_scid) noexcept
	{
		if (out.empty())
		{
			return 0;
		}

		std::uint8_t unused_random = 0;
		if (RAND_bytes(&unused_random, 1) != 1)
		{
			// The byte only fills the unused bits of the first octet, but failing
			// randomness means something is wrong with the CSPRNG, and this server
			// generates connection IDs from the same source.
			return 0;
		}

		// Roles swapped: the reply goes to where the packet came from.
		const ngtcp2_ssize written = ngtcp2_pkt_write_version_negotiation(
			out.data(), out.size(), unused_random, client_scid.bytes.data(), client_scid.len, client_dcid.bytes.data(),
			client_dcid.len, supported_versions.data(), supported_versions.size());

		return written > 0 ? static_cast<std::size_t>(written) : 0;
	}

	StatelessResetToken derive_reset_token(std::span<const std::uint8_t> secret, const CidKey& cid) noexcept
	{
		StatelessResetToken token{};

		std::array<std::uint8_t, EVP_MAX_MD_SIZE> mac{};
		unsigned int mac_len = 0;

		if (HMAC(EVP_sha256(), secret.data(), static_cast<int>(secret.size()), cid.bytes.data(), cid.len, mac.data(),
		         &mac_len) == nullptr ||
		    mac_len < stateless_reset_token_length)
		{
			// An all-zero token would be a token an attacker can also produce. Report
			// the failure by leaving it zeroed and let the caller decide; callers here
			// treat a failed derivation as "send nothing".
			return token;
		}

		std::copy_n(mac.begin(), stateless_reset_token_length, token.begin());
		return token;
	}

	std::size_t write_stateless_reset(std::span<std::uint8_t> out, const StatelessResetToken& token,
	                                  std::size_t incoming_size) noexcept
	{
		// Strictly smaller than what provoked it. RFC 9000 section 10.3: otherwise a
		// spoofed source address turns this server into an amplifier aimed at the
		// victim. This is the single most important line in the function.
		if (incoming_size == 0)
		{
			return 0;
		}
		const std::size_t budget = std::min(out.size(), incoming_size - 1);

		// A reset has to look like an ordinary short-header packet, or it is trivially
		// identifiable and becomes its own signal. That needs the token plus enough
		// random padding in front of it to pass for a packet number and payload.
		constexpr std::size_t min_reset =
			static_cast<std::size_t>(NGTCP2_MIN_STATELESS_RESET_RANDLEN) + stateless_reset_token_length;
		if (budget < min_reset)
		{
			return 0;  // nothing compliant fits: stay silent
		}

		const std::size_t rand_len = budget - stateless_reset_token_length;
		std::array<std::uint8_t, 256> randomness{};
		const std::size_t use_rand = std::min(rand_len, randomness.size());

		if (RAND_bytes(randomness.data(), static_cast<int>(use_rand)) != 1)
		{
			return 0;
		}

		ngtcp2_stateless_reset_token reset_token{};
		std::memcpy(reset_token.data, token.data(), token.size());

		const ngtcp2_ssize written =
			ngtcp2_pkt_write_stateless_reset2(out.data(), budget, &reset_token, randomness.data(), use_rand);

		return written > 0 ? static_cast<std::size_t>(written) : 0;
	}

}  // namespace coroute::http3

#endif  // COROUTE_HAS_HTTP3
