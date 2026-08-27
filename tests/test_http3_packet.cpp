#include <catch2/catch_test_macros.hpp>

#ifdef COROUTE_HAS_HTTP3

#include <coroute/http3/packet.hpp>

#include <algorithm>
#include <cstdint>
#include <vector>

using namespace coroute::http3;

namespace
{

	// Builds a QUIC long header by hand, per RFC 9000 section 17.2:
	//   byte 0     1 | fixed bit | type | reserved/pn-len
	//   bytes 1-4  version, big endian
	//   byte 5     DCID length, then DCID
	//   then       SCID length, then SCID
	std::vector<std::uint8_t> long_header(std::uint32_t version, const std::vector<std::uint8_t>& dcid,
	                                      const std::vector<std::uint8_t>& scid, std::uint8_t first = 0xC0)
	{
		std::vector<std::uint8_t> pkt;
		pkt.push_back(first);
		pkt.push_back(static_cast<std::uint8_t>(version >> 24));
		pkt.push_back(static_cast<std::uint8_t>(version >> 16));
		pkt.push_back(static_cast<std::uint8_t>(version >> 8));
		pkt.push_back(static_cast<std::uint8_t>(version));
		pkt.push_back(static_cast<std::uint8_t>(dcid.size()));
		pkt.insert(pkt.end(), dcid.begin(), dcid.end());
		pkt.push_back(static_cast<std::uint8_t>(scid.size()));
		pkt.insert(pkt.end(), scid.begin(), scid.end());
		// Padded to the minimum a datagram carrying an Initial is allowed to be.
		// RFC 9000 section 14.1 sets that at 1200 bytes so that a server never sends
		// more than it has received, which is what stops QUIC being an amplifier for
		// reflection attacks. A shorter one must be discarded, not answered.
		if (pkt.size() < 1200)
		{
			pkt.resize(1200, 0);
		}
		return pkt;
	}

	// A short header is just a first byte with the high bit clear, then the
	// destination connection ID with no length prefix at all. That is precisely why
	// the server must issue IDs of a known fixed length.
	std::vector<std::uint8_t> short_header(const std::vector<std::uint8_t>& dcid)
	{
		std::vector<std::uint8_t> pkt;
		pkt.push_back(0x40);  // high bit clear, fixed bit set
		pkt.insert(pkt.end(), dcid.begin(), dcid.end());
		pkt.resize(pkt.size() + 16, 0);
		return pkt;
	}

	std::vector<std::uint8_t> counted(std::size_t n, std::uint8_t start = 1)
	{
		std::vector<std::uint8_t> v(n);
		for (std::size_t i = 0; i < n; ++i)
		{
			v[i] = static_cast<std::uint8_t>(start + i);
		}
		return v;
	}

}  // namespace

TEST_CASE("QUIC packets are classified from their header alone", "[http3][packet]")
{
	const auto dcid = counted(server_cid_length, 0x10);
	const auto scid = counted(8, 0xA0);

	SECTION("a supported long header yields both connection IDs")
	{
		const auto pkt = long_header(quic_version_v1, dcid, scid);
		const PacketInfo info = classify_packet(pkt);

		REQUIRE(info.kind == PacketKind::LongHeader);
		REQUIRE(info.version == quic_version_v1);
		REQUIRE(info.dcid.len == dcid.size());
		REQUIRE(std::equal(dcid.begin(), dcid.end(), info.dcid.bytes.begin()));
		REQUIRE(info.scid.len == scid.size());
		REQUIRE(std::equal(scid.begin(), scid.end(), info.scid.bytes.begin()));
	}

	SECTION("an undersized initial is discarded rather than answered")
	{
		// Anti-amplification: answering a short datagram with a Version Negotiation
		// packet would let an attacker use this server to reflect traffic at a spoofed
		// address. It has to be dropped instead.
		std::vector<std::uint8_t> small = long_header(0x1a2a3a4aU, dcid, scid);
		small.resize(200);
		REQUIRE(classify_packet(small).kind != PacketKind::VersionNegotiation);
	}

	SECTION("an unsupported version asks for version negotiation, not rejection")
	{
		// A version this build does not speak is not an error: the client is told what
		// is on offer. The connection IDs still have to come back, because the reply
		// echoes them with the roles swapped, and it is sent with no connection state.
		const auto pkt = long_header(0x1a2a3a4aU, dcid, scid);
		const PacketInfo info = classify_packet(pkt);

		REQUIRE(info.kind == PacketKind::VersionNegotiation);
		REQUIRE(info.dcid.len == dcid.size());
		REQUIRE(info.scid.len == scid.size());
	}

	SECTION("a short header yields only the destination")
	{
		const auto pkt = short_header(dcid);
		const PacketInfo info = classify_packet(pkt);

		REQUIRE(info.kind == PacketKind::ShortHeader);
		REQUIRE(info.dcid.len == dcid.size());
		REQUIRE(std::equal(dcid.begin(), dcid.end(), info.dcid.bytes.begin()));
		REQUIRE(info.scid.len == 0);
	}

	SECTION("the header form is read from the high bit")
	{
		REQUIRE(is_long_header(long_header(quic_version_v1, dcid, scid)));
		REQUIRE_FALSE(is_long_header(short_header(dcid)));
		REQUIRE_FALSE(is_long_header({}));
	}
}

TEST_CASE("malformed datagrams are rejected without crashing", "[http3][packet]")
{
	// This is the path a flood takes. It must cost a header parse and nothing more,
	// and above all it must not read past the buffer.
	SECTION("empty") { REQUIRE(classify_packet({}).kind == PacketKind::Malformed); }

	SECTION("truncated long header")
	{
		// Every prefix of a valid packet, cut before the connection IDs are complete.
		const auto full = long_header(quic_version_v1, counted(server_cid_length, 0x10), counted(8, 0xA0));
		for (std::size_t n = 1; n < 6; ++n)
		{
			INFO("prefix length " << n);
			REQUIRE(classify_packet(std::span<const std::uint8_t>(full.data(), n)).kind == PacketKind::Malformed);
		}
	}

	SECTION("a connection ID longer than the protocol allows")
	{
		// RFC 9000 section 5.1 caps connection IDs at 20 bytes. A peer claiming more
		// must be refused rather than trusted into a fixed-size buffer.
		std::vector<std::uint8_t> pkt;
		pkt.push_back(0xC0);
		for (int i = 0; i < 4; ++i)
		{
			pkt.push_back(0x00);
		}
		pkt.push_back(255);  // absurd DCID length
		pkt.resize(pkt.size() + 64, 0);
		REQUIRE(classify_packet(pkt).kind == PacketKind::Malformed);
	}

	SECTION("random bytes")
	{
		const std::vector<std::uint8_t> junk{0x00, 0xFF, 0x13, 0x37};
		const PacketInfo info = classify_packet(junk);
		// Whatever it decides, it must not claim a connection ID it did not read.
		REQUIRE(info.dcid.len <= max_cid_length);
	}
}

#endif  // COROUTE_HAS_HTTP3
