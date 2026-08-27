#include "coroute/http3/packet.hpp"

#ifdef COROUTE_HAS_HTTP3

#include <ngtcp2/ngtcp2.h>

namespace coroute::http3
{

	PacketInfo classify_packet(std::span<const std::uint8_t> datagram, std::size_t short_dcid_length) noexcept
	{
		PacketInfo info;

		if (datagram.empty())
		{
			return info;  // Malformed
		}

		ngtcp2_version_cid decoded{};
		const int rv = ngtcp2_pkt_decode_version_cid(&decoded, datagram.data(), datagram.size(), short_dcid_length);

		if (rv == NGTCP2_ERR_VERSION_NEGOTIATION)
		{
			// The header parsed but names a version this build does not speak. The
			// connection IDs are still valid and are needed to construct the reply,
			// which has to echo them back with the roles swapped.
			info.kind = PacketKind::VersionNegotiation;
			info.version = decoded.version;
			info.dcid = CidKey{decoded.dcid, decoded.dcidlen};
			info.scid = CidKey{decoded.scid, decoded.scidlen};
			return info;
		}

		if (rv != 0)
		{
			return info;  // Malformed
		}

		info.version = decoded.version;
		info.dcid = CidKey{decoded.dcid, decoded.dcidlen};

		if (is_long_header(datagram))
		{
			info.kind = PacketKind::LongHeader;
			info.scid = CidKey{decoded.scid, decoded.scidlen};
		}
		else
		{
			// A short header carries no source connection ID and no version on the
			// wire; the decoder reports 0 for it.
			info.kind = PacketKind::ShortHeader;
		}

		return info;
	}

}  // namespace coroute::http3

#endif  // COROUTE_HAS_HTTP3
