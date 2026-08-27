#pragma once

#ifdef COROUTE_HAS_HTTP3

#include <cstdint>
#include <span>

#include "coroute/http3/cid.hpp"

namespace coroute::http3
{

	// ============================================================================
	// First-look packet classification
	// ============================================================================
	//
	// The QUIC counterpart of the first-octet classifier on the TCP side. Every
	// datagram arriving on the UDP socket has to be routed before anything else can
	// happen, and routing needs only the header: the connection ID, and whether the
	// version is one this server speaks.
	//
	// This is deliberately the cheapest possible look. It does no crypto, allocates
	// nothing, and copies only the connection IDs. A packet for an unknown connection
	// costs a header parse and nothing more, which matters because that is exactly
	// what a flood looks like.

	// QUIC version 1, RFC 9000 section 15. Exposed here so callers and tests do not
	// have to include ngtcp2 headers: keeping the QUIC library an implementation
	// detail of this module is deliberate.
	inline constexpr std::uint32_t quic_version_v1 = 0x00000001U;

	enum class PacketKind : std::uint8_t
	{
		// A long-header packet whose version this server does not speak. The reply is
		// a Version Negotiation packet, which is the one response that must be sent
		// without any connection state at all.
		VersionNegotiation,

		// A long-header packet: Initial, Handshake, 0-RTT or Retry. Carries both
		// connection IDs, so a new connection can be created from it.
		LongHeader,

		// A short-header packet, meaning the handshake is done. Carries only the
		// destination connection ID, which is why the server must issue IDs of a known
		// fixed length: there is no length field to read.
		ShortHeader,

		// Not a QUIC packet, or truncated past the point of being parseable.
		Malformed
	};

	struct PacketInfo
	{
		PacketKind kind = PacketKind::Malformed;
		std::uint32_t version = 0;
		CidKey dcid;  // who this packet is for
		CidKey scid;  // who sent it; empty for short-header packets
	};

	// Parses just enough of the header to route the datagram.
	//
	// `short_dcid_length` is the length this server issues, since a short header has
	// no length field. Anything else would be guessing, which is why server_cid_length
	// is fixed rather than negotiated.
	[[nodiscard]] PacketInfo classify_packet(std::span<const std::uint8_t> datagram,
	                                         std::size_t short_dcid_length = server_cid_length) noexcept;

	// True when the header is long-form, which is the only case carrying a source
	// connection ID and the only case that can begin a connection.
	[[nodiscard]] constexpr bool is_long_header(std::span<const std::uint8_t> datagram) noexcept
	{
		// RFC 9000 section 17.2: the high bit of the first byte distinguishes the two
		// header forms.
		return !datagram.empty() && (datagram[0] & 0x80) != 0;
	}

}  // namespace coroute::http3

#endif  // COROUTE_HAS_HTTP3
