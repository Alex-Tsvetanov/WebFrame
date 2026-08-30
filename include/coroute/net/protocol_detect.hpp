#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <string_view>
#include <vector>

#include "coroute/net/io_context.hpp"

namespace coroute::net
{

	// ============================================================================
	// First-octet protocol classification
	// ============================================================================
	//
	// This is what lets one TCP descriptor serve both TLS and cleartext. A TLS record
	// begins with a ContentType octet, and the first record a client sends is always
	// 0x16 (handshake). Every HTTP/1.x method token and the HTTP/2 connection preface
	// begin with an ASCII uppercase letter. One octet separates the two cases, and no
	// other application protocol served here overlaps.

	enum class WireProtocol : std::uint8_t
	{
		Tls,        // TLS record layer, ContentType 0x16
		Cleartext,  // an HTTP method token, or the HTTP/2 preface
		Unknown     // neither: reject rather than guess
	};

	constexpr WireProtocol classify(std::span<const std::uint8_t> first) noexcept
	{
		if (first.empty())
		{
			return WireProtocol::Unknown;
		}

		const std::uint8_t octet = first.front();
		if (octet == 0x16)
		{
			return WireProtocol::Tls;
		}
		if (octet >= 'A' && octet <= 'Z')
		{
			return WireProtocol::Cleartext;
		}
		return WireProtocol::Unknown;
	}

	// ============================================================================
	// HTTP/2 connection preface
	// ============================================================================
	//
	// RFC 9113 section 3.4. Transcribed from the RFC rather than shared with
	// http2::Constants::ClientPreface so that net/ stays usable when HTTP/2 is not
	// compiled in. Both are checked against the same published bytes by their tests.

	inline constexpr std::string_view http2_client_preface = "PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n";

	enum class PrefaceMatch : std::uint8_t
	{
		No,     // a byte already differs: this is not the preface
		Maybe,  // a prefix so far, needs more input to decide
		Yes     // the complete preface is present
	};

	// Incremental on purpose. http2::is_http2_preface answers only once all 24 bytes
	// are buffered, which is fine inside an established HTTP/2 connection but wrong on
	// the accept path: a peer that dribbles one byte at a time would hold the
	// classifier open. Real method tokens diverge from the preface within three
	// octets, PROPFIND being the longest shared prefix at "PR", so a verdict of No
	// arrives almost immediately for genuine traffic.
	constexpr PrefaceMatch preface_match(std::span<const std::uint8_t> data) noexcept
	{
		const std::size_t comparable = std::min(data.size(), http2_client_preface.size());
		for (std::size_t i = 0; i < comparable; ++i)
		{
			if (static_cast<char>(data[i]) != http2_client_preface[i])
			{
				return PrefaceMatch::No;
			}
		}
		return (data.size() >= http2_client_preface.size()) ? PrefaceMatch::Yes : PrefaceMatch::Maybe;
	}

	// ============================================================================
	// PrefaceConnection - replays bytes already consumed for classification
	// ============================================================================
	//
	// Classification has to look at the first octets, and the layer underneath then
	// needs those same octets: the request line goes to the HTTP parser, the
	// ClientHello to the TLS handshake, the preface to Http2Connection. MSG_PEEK would
	// leave them in the kernel at the cost of reading every byte twice and of three
	// separate backend implementations. Reading once and replaying costs one branch
	// per read until the buffer drains, and needs no backend changes at all.
	//
	// This wraps any Connection, including a TlsConnection, so nothing downstream has
	// to know a classification happened.
	class PrefaceConnection final : public Connection
	{
	public:
		PrefaceConnection(std::unique_ptr<Connection> inner, std::vector<std::uint8_t> pushback);

		Task<ReadResult> async_read(void* buffer, size_t len) override;
		Task<ReadResult> async_read_until(void* buffer, size_t len, char delimiter) override;
		Task<WriteResult> async_write(const void* buffer, size_t len) override;
		Task<WriteResult> async_write_all(const void* buffer, size_t len) override;
		Task<WriteResult> async_write_zero_copy(const void* buffer, size_t len) override;
		[[nodiscard]] bool supports_zero_copy_send() const noexcept override;
		Task<TransmitResult> async_transmit_file(FileHandle file, size_t offset, size_t length) override;

		void close() override;
		bool is_open() const noexcept override;
		void set_timeout(std::chrono::milliseconds timeout) override;
		std::string remote_address() const override;
		uint16_t remote_port() const noexcept override;
		void set_cancellation_token(CancellationToken token) override;

		// True while replayed bytes remain. Exposed for tests, not for dispatch logic.
		[[nodiscard]] bool has_pushback() const noexcept { return pos_ < pushback_.size(); }

	private:
		// Copies up to len bytes of remaining pushback into buffer.
		size_t drain(void* buffer, size_t len) noexcept;

		std::unique_ptr<Connection> inner_;
		std::vector<std::uint8_t> pushback_;
		size_t pos_ = 0;
	};

	// ============================================================================
	// read_prefix
	// ============================================================================

	struct Prefix
	{
		std::vector<std::uint8_t> bytes;   // what was read, for classification
		std::unique_ptr<Connection> conn;  // the same connection, replaying bytes
	};

	// Reads until at least min_bytes are buffered, then hands back both the bytes and
	// a connection that will replay them. One byte is enough to separate TLS from
	// cleartext; the h2 preface check asks for more only when the first bytes keep
	// matching.
	Task<expected<Prefix, Error>> read_prefix(std::unique_ptr<Connection> conn, size_t min_bytes = 1);

}  // namespace coroute::net
