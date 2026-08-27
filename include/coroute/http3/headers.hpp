#pragma once

#ifdef COROUTE_HAS_HTTP3

#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "coroute/core/request.hpp"
#include "coroute/core/response.hpp"

namespace coroute::http3
{

	// ============================================================================
	// HTTP/3 header fields to and from the framework's Request and Response
	// ============================================================================
	//
	// This is where HTTP/3 stops being a separate protocol. Once a request has become
	// a Request it goes through the same router, the same middleware chain and the
	// same handlers as HTTP/1.1 and HTTP/2, which is the point: adding a protocol
	// should not fork the application.
	//
	// The conversion is not merely mechanical. RFC 9114 section 4.2 makes several
	// things that are legal in HTTP/1.1 into malformed requests, and a server that
	// accepts them anyway is the lenient end of a request-smuggling pair.

	// Why a request was rejected. Reported rather than silently dropped, because
	// "malformed" here maps onto a specific stream error the peer needs to see.
	enum class HeaderError : std::uint8_t
	{
		None,
		UppercaseFieldName,   // RFC 9114 section 4.2: names are lowercase, always
		ConnectionSpecific,   // Connection, Keep-Alive, Transfer-Encoding, Upgrade
		BadTeValue,           // TE is allowed, but only with the value "trailers"
		UnknownPseudoHeader,  // an unrecognised colon-prefixed field
		PseudoHeaderAfterField,
		DuplicatePseudoHeader,
		MissingPseudoHeader,  // :method, :scheme and :path are all required
		EmptyPath
	};

	[[nodiscard]] std::string_view describe(HeaderError error) noexcept;

	// Accumulates header fields as nghttp3 delivers them, then produces a Request.
	//
	// Stateful because the ordering rules are stateful: every pseudo-header must
	// precede every regular field, and none may repeat.
	class RequestBuilder
	{
	public:
		// Returns false on the first violation and records it; later calls are then
		// ignored, so a caller can add everything and check once at the end.
		bool add(std::string_view name, std::string_view value);

		// Produces the Request, or nothing if the field block was malformed or
		// incomplete.
		[[nodiscard]] std::optional<Request> build();

		[[nodiscard]] HeaderError error() const noexcept { return error_; }

	private:
		bool fail(HeaderError error) noexcept;

		Request request_;
		std::string method_;
		std::string scheme_;
		std::string authority_;
		std::string path_;
		bool seen_regular_field_ = false;
		HeaderError error_ = HeaderError::None;
	};

	// One HTTP/3 header field. Values are owned so the caller can hand the vector to
	// nghttp3 and let it outlive the Response it came from.
	struct HeaderField
	{
		std::string name;
		std::string value;
	};

	// Renders a Response as HTTP/3 header fields, starting with :status.
	//
	// Connection-specific fields are dropped rather than passed through: the same
	// Response object is produced by handlers shared with HTTP/1.1, where those
	// fields are ordinary, and sending one over HTTP/3 is a protocol violation the
	// peer is entitled to treat as fatal.
	[[nodiscard]] std::vector<HeaderField> response_fields(const Response& response);

}  // namespace coroute::http3

#endif  // COROUTE_HAS_HTTP3
