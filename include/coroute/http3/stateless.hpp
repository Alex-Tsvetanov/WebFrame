#pragma once

#ifdef COROUTE_HAS_HTTP3

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

#include "coroute/http3/cid.hpp"

namespace coroute::http3
{

	// ============================================================================
	// Responses a QUIC server sends with no connection state
	// ============================================================================
	//
	// Two of the three replies a QUIC server can make are answers to packets it has
	// no state for: a version it does not speak, and a connection it does not know.
	// They are pure functions of the incoming packet, which is what makes them
	// testable in isolation, and they are the entire cost of a packet flood, which is
	// what makes their size limits a security property rather than an optimisation.

	// RFC 9000 section 10.3: a stateless reset token is 16 bytes.
	inline constexpr std::size_t stateless_reset_token_length = 16;

	using StatelessResetToken = std::array<std::uint8_t, stateless_reset_token_length>;

	// Writes a Version Negotiation packet telling the peer which versions are on
	// offer. Returns the number of bytes written, or 0 if it would not fit.
	//
	// The connection IDs are echoed with the roles swapped: the reply is addressed to
	// the peer's source ID and sent from the peer's destination ID. Getting that
	// backwards produces a packet the client silently discards, and since this is sent
	// before any handshake there is nothing else to notice the mistake.
	[[nodiscard]] std::size_t write_version_negotiation(std::span<std::uint8_t> out, const CidKey& client_dcid,
	                                                    const CidKey& client_scid) noexcept;

	// Derives the reset token for a connection ID.
	//
	// Keyed, not merely hashed. A peer that can compute reset tokens for arbitrary
	// connection IDs can terminate any connection on the server at will, so this has
	// to depend on a secret the peer does not have. RFC 9000 section 10.3.1 says as
	// much: the token must be hard to guess without the server's secret.
	[[nodiscard]] StatelessResetToken derive_reset_token(std::span<const std::uint8_t> secret,
	                                                     const CidKey& cid) noexcept;

	// Writes a Stateless Reset, which tells a peer that the connection it is talking
	// about no longer exists here.
	//
	// `incoming_size` is the size of the packet being answered. The reply is made
	// strictly smaller, per RFC 9000 section 10.3: a server that answered a small
	// packet with a larger one would be an amplifier, and an attacker spoofing a
	// victim's address could aim that amplification at them. Returns 0 when no
	// compliant reset fits, which is the correct outcome for a very small packet:
	// send nothing.
	[[nodiscard]] std::size_t write_stateless_reset(std::span<std::uint8_t> out, const StatelessResetToken& token,
	                                                std::size_t incoming_size) noexcept;

}  // namespace coroute::http3

#endif  // COROUTE_HAS_HTTP3
