#include <catch2/catch_test_macros.hpp>

#ifdef COROUTE_HAS_HTTP3

#include <coroute/http3/headers.hpp>

#include <algorithm>
#include <string>

using namespace coroute;
using namespace coroute::http3;

namespace
{

	// A well-formed minimal request, so each test can vary one thing.
	RequestBuilder minimal()
	{
		RequestBuilder b;
		b.add(":method", "GET");
		b.add(":scheme", "https");
		b.add(":authority", "example.test");
		b.add(":path", "/hello");
		return b;
	}

	const HeaderField* find(const std::vector<HeaderField>& fields, std::string_view name)
	{
		auto it = std::find_if(fields.begin(), fields.end(), [&](const HeaderField& f) { return f.name == name; });
		return it == fields.end() ? nullptr : &*it;
	}

}  // namespace

TEST_CASE("a well-formed HTTP/3 request becomes an ordinary Request", "[http3][headers]")
{
	// The whole point: once converted it goes through the same router, middleware and
	// handlers as HTTP/1.1 and HTTP/2. Adding a protocol should not fork the app.
	RequestBuilder b = minimal();
	b.add("user-agent", "test/1.0");

	auto req = b.build();
	REQUIRE(req.has_value());
	REQUIRE(req->method() == HttpMethod::GET);
	REQUIRE(req->path() == "/hello");
	REQUIRE(req->http_version() == "HTTP/3");

	SECTION("regular fields carry through")
	{
		auto ua = req->header("user-agent");
		REQUIRE(ua.has_value());
		REQUIRE(*ua == "test/1.0");
	}

	SECTION(":authority is surfaced as Host")
	{
		// HTTP/3 has no Host field. Handlers and middleware shared with HTTP/1.1 look
		// for one, so it is synthesised rather than making every caller ask which
		// protocol it is serving.
		auto host = req->header("host");
		REQUIRE(host.has_value());
		REQUIRE(*host == "example.test");
	}
}

TEST_CASE("HTTP/3 rejects what HTTP/1.1 tolerates", "[http3][headers]")
{
	// Each of these is legal or merely odd in HTTP/1.1 and malformed in HTTP/3. A
	// server that accepts them anyway is the lenient half of a request-smuggling
	// pair, which is why they are refused rather than normalised away.

	SECTION("an uppercase field name is malformed, not something to lowercase")
	{
		// RFC 9114 section 4.2. The rule exists so two intermediaries cannot disagree
		// about whether Content-Length and content-length are the same field.
		RequestBuilder b = minimal();
		REQUIRE_FALSE(b.add("Content-Length", "0"));
		REQUIRE(b.error() == HeaderError::UppercaseFieldName);
		REQUIRE_FALSE(b.build().has_value());
	}

	SECTION("connection-specific fields are refused")
	{
		for (std::string_view name : {"connection", "keep-alive", "proxy-connection", "transfer-encoding", "upgrade"})
		{
			RequestBuilder b = minimal();
			INFO("field: " << name);
			REQUIRE_FALSE(b.add(name, "whatever"));
			REQUIRE(b.error() == HeaderError::ConnectionSpecific);
		}
	}

	SECTION("TE is allowed only as trailers")
	{
		RequestBuilder ok = minimal();
		REQUIRE(ok.add("te", "trailers"));
		REQUIRE(ok.build().has_value());

		RequestBuilder bad = minimal();
		REQUIRE_FALSE(bad.add("te", "gzip"));
		REQUIRE(bad.error() == HeaderError::BadTeValue);
	}

	SECTION("a pseudo-header after a regular field is out of order")
	{
		RequestBuilder b;
		b.add(":method", "GET");
		b.add("user-agent", "test");
		REQUIRE_FALSE(b.add(":path", "/late"));
		REQUIRE(b.error() == HeaderError::PseudoHeaderAfterField);
	}

	SECTION("a repeated pseudo-header is refused")
	{
		// Two :path values would let two intermediaries route the same request to
		// different places.
		RequestBuilder b;
		b.add(":method", "GET");
		b.add(":scheme", "https");
		b.add(":path", "/one");
		REQUIRE_FALSE(b.add(":path", "/two"));
		REQUIRE(b.error() == HeaderError::DuplicatePseudoHeader);
	}

	SECTION("an unknown pseudo-header is refused")
	{
		RequestBuilder b;
		REQUIRE_FALSE(b.add(":status", "200"));  // a response pseudo-header
		REQUIRE(b.error() == HeaderError::UnknownPseudoHeader);
	}

	SECTION("the required pseudo-headers really are required")
	{
		// RFC 9114 section 4.3.1: method, scheme and path. Authority is not required,
		// so its absence must still build.
		RequestBuilder no_method;
		no_method.add(":scheme", "https");
		no_method.add(":path", "/x");
		REQUIRE_FALSE(no_method.build().has_value());

		RequestBuilder no_scheme;
		no_scheme.add(":method", "GET");
		no_scheme.add(":path", "/x");
		REQUIRE_FALSE(no_scheme.build().has_value());

		RequestBuilder no_path;
		no_path.add(":method", "GET");
		no_path.add(":scheme", "https");
		REQUIRE_FALSE(no_path.build().has_value());

		RequestBuilder no_authority;
		no_authority.add(":method", "GET");
		no_authority.add(":scheme", "https");
		no_authority.add(":path", "/x");
		REQUIRE(no_authority.build().has_value());
	}

	SECTION("the first failure is the one reported")
	{
		// A caller may add everything and check once at the end, so later violations
		// must not overwrite the reason the message was rejected.
		RequestBuilder b = minimal();
		REQUIRE_FALSE(b.add("Upgrade", "websocket"));  // uppercase and forbidden
		REQUIRE(b.error() == HeaderError::UppercaseFieldName);
		b.add("connection", "close");
		REQUIRE(b.error() == HeaderError::UppercaseFieldName);
	}
}

TEST_CASE("a Response becomes HTTP/3 header fields", "[http3][headers]")
{
	SECTION(":status comes first and is the numeric code")
	{
		// Pseudo-headers must precede regular fields, and there is exactly one here.
		Response resp = Response::ok("hi");
		const auto fields = response_fields(resp);
		REQUIRE_FALSE(fields.empty());
		REQUIRE(fields.front().name == ":status");
		REQUIRE(fields.front().value == "200");
	}

	SECTION("field names are lowercased on the way out")
	{
		Response resp(200,
		              {
						  {"Content-Type", "text/plain"},
                          {    "X-Custom",          "1"}
        },
		              "body");
		const auto fields = response_fields(resp);
		for (const auto& f : fields)
		{
			INFO("field: " << f.name);
			REQUIRE(std::none_of(f.name.begin(), f.name.end(), [](char c) { return c >= 'A' && c <= 'Z'; }));
		}
		REQUIRE(find(fields, "content-type") != nullptr);
		REQUIRE(find(fields, "x-custom") != nullptr);
	}

	SECTION("connection-specific fields are dropped rather than forwarded")
	{
		// Handlers are shared with HTTP/1.1, where these are ordinary. Emitting one
		// over HTTP/3 is a protocol violation the peer may treat as fatal, so the
		// conversion has to remove them rather than trust the handler.
		Response resp(
			200,
			{
				{       "Connection", "keep-alive"},
                {"Transfer-Encoding",    "chunked"},
                {     "Content-Type", "text/plain"}
        },
			"body");
		const auto fields = response_fields(resp);

		REQUIRE(find(fields, "connection") == nullptr);
		REQUIRE(find(fields, "transfer-encoding") == nullptr);
		REQUIRE(find(fields, "content-type") != nullptr);
	}

	SECTION("a non-200 status is carried through")
	{
		Response resp = Response::bad_request("nope");
		const auto fields = response_fields(resp);
		REQUIRE(fields.front().value == "400");
	}
}

#endif  // COROUTE_HAS_HTTP3
