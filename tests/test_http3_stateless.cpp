#include <catch2/catch_test_macros.hpp>

#ifdef COROUTE_HAS_HTTP3

#include <coroute/http3/packet.hpp>
#include <coroute/http3/stateless.hpp>

#include <algorithm>
#include <array>
#include <cstdint>
#include <set>
#include <vector>

using namespace coroute::http3;

namespace
{

	CidKey cid_from(const std::vector<std::uint8_t>& raw) { return CidKey{raw.data(), raw.size()}; }

	std::vector<std::uint8_t> counted(std::size_t n, std::uint8_t start)
	{
		std::vector<std::uint8_t> v(n);
		for (std::size_t i = 0; i < n; ++i)
		{
			v[i] = static_cast<std::uint8_t>(start + i);
		}
		return v;
	}

	const std::vector<std::uint8_t> server_secret = counted(32, 0x5A);

}  // namespace

TEST_CASE("version negotiation echoes the connection IDs with roles swapped", "[http3][stateless]")
{
	const CidKey client_dcid = cid_from(counted(16, 0x10));
	const CidKey client_scid = cid_from(counted(8, 0xA0));

	std::array<std::uint8_t, 1200> buf{};
	const std::size_t n = write_version_negotiation(buf, client_dcid, client_scid);
	REQUIRE(n > 0);

	SECTION("the reply is addressed back to the sender")
	{
		// This is the part that is easy to get backwards, and the failure is silent:
		// the client simply ignores a packet that is not addressed to it, and there is
		// no handshake yet to surface the mistake.
		const PacketInfo info = classify_packet(std::span<const std::uint8_t>(buf.data(), n), server_cid_length);

		// A Version Negotiation packet carries version 0, which is how a client
		// recognises it rather than treating it as an ordinary long header.
		REQUIRE(info.version == 0);
		REQUIRE(info.dcid.len == client_scid.len);
		REQUIRE(std::equal(client_scid.bytes.begin(), client_scid.bytes.begin() + client_scid.len,
		                   info.dcid.bytes.begin()));
		REQUIRE(info.scid.len == client_dcid.len);
		REQUIRE(std::equal(client_dcid.bytes.begin(), client_dcid.bytes.begin() + client_dcid.len,
		                   info.scid.bytes.begin()));
	}

	SECTION("it offers at least one version the server speaks")
	{
		// The version list follows the two connection IDs. Finding v1 in the tail is
		// enough to know the offer is not empty, which would make the exchange a
		// pointless round trip.
		bool found_v1 = false;
		for (std::size_t i = 0; i + 4 <= n; ++i)
		{
			const std::uint32_t v = (static_cast<std::uint32_t>(buf[i]) << 24) |
			                        (static_cast<std::uint32_t>(buf[i + 1]) << 16) |
			                        (static_cast<std::uint32_t>(buf[i + 2]) << 8) | buf[i + 3];
			if (v == quic_version_v1)
			{
				found_v1 = true;
				break;
			}
		}
		REQUIRE(found_v1);
	}

	SECTION("a buffer too small yields nothing rather than a truncated packet")
	{
		std::array<std::uint8_t, 4> tiny{};
		REQUIRE(write_version_negotiation(tiny, client_dcid, client_scid) == 0);
	}
}

TEST_CASE("stateless reset tokens depend on the server secret", "[http3][stateless]")
{
	const CidKey cid = cid_from(counted(16, 0x30));

	SECTION("the same connection ID and secret give the same token")
	{
		// The server does not store tokens; it re-derives them. If derivation were not
		// deterministic, a reset would never be recognised by the peer.
		REQUIRE(derive_reset_token(server_secret, cid) == derive_reset_token(server_secret, cid));
	}

	SECTION("a different connection ID gives a different token")
	{
		const CidKey other = cid_from(counted(16, 0x31));
		REQUIRE_FALSE(derive_reset_token(server_secret, cid) == derive_reset_token(server_secret, other));
	}

	SECTION("a different secret gives a different token")
	{
		// This is the property that matters. A peer able to compute reset tokens can
		// terminate any connection on the server, so the token must be unreachable
		// without the secret rather than merely a hash of public data.
		const std::vector<std::uint8_t> other_secret = counted(32, 0x77);
		REQUIRE_FALSE(derive_reset_token(server_secret, cid) == derive_reset_token(other_secret, cid));
	}

	SECTION("tokens are spread out, not clustered")
	{
		std::set<StatelessResetToken> tokens;
		for (std::uint8_t i = 0; i < 200; ++i)
		{
			tokens.insert(derive_reset_token(server_secret, cid_from(counted(16, i))));
		}
		REQUIRE(tokens.size() == 200);
	}
}

TEST_CASE("a stateless reset never amplifies", "[http3][stateless]")
{
	const StatelessResetToken token = derive_reset_token(server_secret, cid_from(counted(16, 0x40)));
	std::array<std::uint8_t, 1500> buf{};

	SECTION("the reply is always smaller than the packet that provoked it")
	{
		// RFC 9000 section 10.3. A server answering a small spoofed packet with a
		// larger one is an amplifier that an attacker can aim at a victim's address.
		for (std::size_t incoming :
		     {std::size_t{22}, std::size_t{64}, std::size_t{300}, std::size_t{1200}, std::size_t{1500}})
		{
			const std::size_t n = write_stateless_reset(buf, token, incoming);
			INFO("incoming " << incoming << " reply " << n);
			REQUIRE(n < incoming);
		}
	}

	SECTION("too small to answer compliantly means silence")
	{
		// A reset must still look like an ordinary short-header packet, so there is a
		// floor below which no compliant reply exists. Sending nothing is correct;
		// sending something oversized would not be.
		for (std::size_t incoming = 0; incoming < 22; ++incoming)
		{
			INFO("incoming " << incoming);
			REQUIRE(write_stateless_reset(buf, token, incoming) == 0);
		}
	}

	SECTION("the token is present in the reply")
	{
		// The peer recognises a reset by matching the last 16 bytes against the token
		// it was given. If the token were missing or misplaced the packet would be
		// discarded as an unintelligible one.
		const std::size_t n = write_stateless_reset(buf, token, 1200);
		REQUIRE(n >= token.size());
		REQUIRE(std::equal(token.begin(), token.end(), buf.begin() + static_cast<std::ptrdiff_t>(n - token.size())));
	}

	SECTION("two resets for the same connection do not look identical")
	{
		// The padding is random so that resets are not fingerprintable as a class,
		// which would let an observer count them.
		const std::size_t a = write_stateless_reset(buf, token, 1200);
		std::vector<std::uint8_t> first(buf.begin(), buf.begin() + static_cast<std::ptrdiff_t>(a));

		std::array<std::uint8_t, 1500> buf2{};
		const std::size_t b = write_stateless_reset(buf2, token, 1200);
		std::vector<std::uint8_t> second(buf2.begin(), buf2.begin() + static_cast<std::ptrdiff_t>(b));

		REQUIRE(a == b);
		REQUIRE(first != second);
	}
}

#endif  // COROUTE_HAS_HTTP3
