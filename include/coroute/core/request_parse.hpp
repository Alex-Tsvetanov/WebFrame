#pragma once

// Parsing an HTTP/1.1 request head, separated from reading one.
//
// It lived inside App::parse_request, which is a coroutine that reads from a
// Connection, so the largest attacker-controlled parsing surface in the tree could not
// be tested without a socket. Everything from the request line through the last header
// operates on a string_view and needed nothing from the connection; only the body read
// did. Splitting them costs nothing and makes the parser reachable from a test and
// from a fuzz harness.

#include <string>
#include <string_view>

#include "coroute/core/error.hpp"
#include "coroute/core/request.hpp"
#include "coroute/util/expected.hpp"

namespace coroute
{

	// Percent-decoding.
	//
	// plus_is_space belongs to application/x-www-form-urlencoded, not to URIs. It was
	// applied unconditionally, so a path containing a literal plus decoded to a space and
	// routed as a different path than the one requested. RFC 3986 has no such rule for
	// paths; RFC 1866 introduced it for form fields, and that is where it stays.
	std::string url_decode(std::string_view str, bool plus_is_space);

	// The request line and the header fields, and nothing that touches a socket.
	//
	// `data` is the octets up to and including the blank line that ends the headers.
	// Anything after it is the body and is the caller's problem.
	expected<Request, Error> parse_request_head(std::string_view data);

}  // namespace coroute
