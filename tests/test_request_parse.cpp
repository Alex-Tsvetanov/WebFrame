#include <catch2/catch_test_macros.hpp>
#include <coroute/core/request_parse.hpp>

#include <string>

using namespace coroute;

TEST_CASE("Percent escapes are hexadecimal or they are not escapes", "[request_parse]")
{
	// The pair used to be handed to strtol and accepted whenever strtol consumed two
	// characters, which it does for "+1" and " 1" because it skips leading space and
	// takes a sign. So "%+1" decoded to 0x01. A decoder that accepts sequences the
	// grammar cannot produce widens what can encode any given octet, and on a path the
	// octets worth encoding are the ones in "..".
	CHECK(url_decode("%2e%2e", false) == "..");
	CHECK(url_decode("%2E%2E", false) == "..");
	CHECK(url_decode("%41", false) == "A");

	CHECK(url_decode("%+1", false) == "%+1");
	CHECK(url_decode("% 1", false) == "% 1");
	CHECK(url_decode("%-1", false) == "%-1");
	CHECK(url_decode("%zz", false) == "%zz");

	// A truncated escape at the end is left alone rather than read past.
	CHECK(url_decode("%4", false) == "%4");
	CHECK(url_decode("%", false) == "%");
	CHECK(url_decode("", false).empty());
}

TEST_CASE("Plus is a space in a form field and a plus in a path", "[request_parse]")
{
	// RFC 1866 introduced plus-for-space for form fields. RFC 3986 has no such rule for
	// URIs, so a path containing a literal plus used to decode to a space and route as
	// a different path than the one requested.
	CHECK(url_decode("a+b", true) == "a b");
	CHECK(url_decode("a+b", false) == "a+b");
}

TEST_CASE("Parsing a request head", "[request_parse]")
{
	SECTION("an ordinary request")
	{
		auto req = parse_request_head("GET /hello?a=1&b=two HTTP/1.1\r\nHost: example.com\r\n"
		                              "User-Agent: probe\r\n\r\n");
		REQUIRE(req.has_value());
		CHECK(req->path() == "/hello");
		CHECK(req->query_string() == "a=1&b=two");
		CHECK(req->header("host") == "example.com");
		// Field names fold, so the case a caller asks in does not matter.
		CHECK(req->header("Host") == "example.com");
		CHECK(req->header("USER-AGENT") == "probe");
	}

	SECTION("a path keeps its literal plus")
	{
		auto req = parse_request_head("GET /a+b HTTP/1.1\r\nHost: x\r\n\r\n");
		REQUIRE(req.has_value());
		CHECK(req->path() == "/a+b");
	}

	SECTION("a query parameter takes plus as a space")
	{
		auto req = parse_request_head("GET /?q=a+b HTTP/1.1\r\nHost: x\r\n\r\n");
		REQUIRE(req.has_value());
		auto q = req->query_params().find("q");
		REQUIRE(q != req->query_params().end());
		CHECK(q->second == "a b");
	}

	SECTION("conflicting Content-Length is refused")
	{
		auto req = parse_request_head("POST / HTTP/1.1\r\nContent-Length: 0\r\nContent-Length: 5\r\n\r\n");
		CHECK_FALSE(req.has_value());

		// Repeated but agreeing is not a conflict.
		auto same = parse_request_head("POST / HTTP/1.1\r\nContent-Length: 5\r\nContent-Length: 5\r\n\r\n");
		CHECK(same.has_value());
	}

	SECTION("chunked is accepted here and decoded by the caller")
	{
		// The head parser has no connection, so it can only say the framing is one it
		// understands. Reading the body is parse_request's job.
		auto req = parse_request_head("POST / HTTP/1.1\r\nTransfer-Encoding: chunked\r\n\r\n");
		CHECK(req.has_value());

		// Including when the field name arrives in the case an HTTP/2 gateway emits.
		auto folded = parse_request_head("POST / HTTP/1.1\r\ntransfer-encoding: chunked\r\n\r\n");
		CHECK(folded.has_value());
	}

	SECTION("a transfer coding this server cannot apply is refused")
	{
		// Not "the part of the list I recognise". Picking a coding out of a list is how
		// a parser ends up framing a body it did not understand.
		CHECK_FALSE(parse_request_head("POST / HTTP/1.1\r\nTransfer-Encoding: gzip\r\n\r\n").has_value());
		CHECK_FALSE(parse_request_head("POST / HTTP/1.1\r\nTransfer-Encoding: gzip, chunked\r\n\r\n").has_value());
		CHECK_FALSE(parse_request_head("POST / HTTP/1.1\r\nTransfer-Encoding: chunked, gzip\r\n\r\n").has_value());
	}

	SECTION("both framings at once is refused")
	{
		// RFC 9112 section 6.3: two parties may resolve this differently, which is the
		// same reason two Content-Length lines are refused.
		auto both = parse_request_head(
		    "POST / HTTP/1.1\r\nTransfer-Encoding: chunked\r\nContent-Length: 5\r\n\r\n");
		CHECK_FALSE(both.has_value());
	}

	SECTION("a request line missing its parts is refused")
	{
		CHECK_FALSE(parse_request_head("").has_value());
		CHECK_FALSE(parse_request_head("GET\r\n\r\n").has_value());
		CHECK_FALSE(parse_request_head("GET /\r\n\r\n").has_value());
		CHECK_FALSE(parse_request_head("no crlf at all").has_value());
	}

	SECTION("a header line without a colon does not become a header")
	{
		auto req = parse_request_head("GET / HTTP/1.1\r\nHost: x\r\nnonsense\r\n\r\n");
		REQUIRE(req.has_value());
		CHECK(req->headers().size() == 1);
	}
}
