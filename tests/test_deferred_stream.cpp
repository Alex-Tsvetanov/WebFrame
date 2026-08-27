#include <catch2/catch_test_macros.hpp>

#ifdef COROUTE_HAS_TEMPLATES

#include <coroute/view/deferred_stream.hpp>

#include <string>

using namespace coroute;

TEST_CASE("the runtime gives a page real Promises", "[deferred][stream]")
{
	const std::string script = deferred_runtime_script();

	// The contract page code is written against. If this name moves, every template
	// using it breaks silently at runtime rather than loudly at build time, so it is
	// worth pinning.
	REQUIRE(script.find("api.deferred = function") != std::string::npos);
	REQUIRE(script.find("new Promise") != std::string::npos);

	SECTION("resolving and rejecting are both reachable")
	{
		// A slot with no way to fail leaves a page hanging on a handler that threw,
		// which looks exactly like a slow query and is the worst failure mode there is.
		REQUIRE(script.find("api.__resolve") != std::string::npos);
		REQUIRE(script.find("api.__reject") != std::string::npos);
	}

	SECTION("the no-JavaScript fallback is still there")
	{
		// Kept alongside the Promise, not instead of it: a page that writes no script of
		// its own still fills in, and one that does gets the typed version.
		REQUIRE(script.find("data-coroute-slot") != std::string::npos);
	}

	SECTION("it depends on nothing")
	{
		// A deferred page that had to fetch a framework before it could show a hole
		// would have spent the head start that deferring bought it.
		REQUIRE(script.find("src=") == std::string::npos);
		REQUIRE(script.find("import ") == std::string::npos);
	}
}

TEST_CASE("a value cannot break out of the script element", "[deferred][stream][security]")
{
	// The security boundary. HTML looks for the closing tag inside a script element
	// before JavaScript ever sees the contents, so a value containing </script> ends
	// the block early and everything after it is parsed as markup. A user-supplied
	// string is enough to do that, which makes it stored XSS on any page with a
	// deferred field.

	SECTION("a closing script tag is neutralised")
	{
		const nlohmann::json value = "</script><img src=x onerror=alert(1)>";
		const std::string chunk = deferred_resolve_script(0, value);

		// Exactly one script element: the one this function opened.
		REQUIRE(chunk.find("</script><img") == std::string::npos);
		REQUIRE(chunk.find("\\u003C/script") != std::string::npos);
	}

	SECTION("angle brackets and ampersands are escaped wherever they appear")
	{
		const nlohmann::json value = nlohmann::json{
			{"html", "<b>bold</b> & <i>italic</i>"}
        };
		const std::string chunk = deferred_resolve_script(3, value);

		REQUIRE(chunk.find("<b>") == std::string::npos);
		REQUIRE(chunk.find("&") == std::string::npos);
		REQUIRE(chunk.find("\\u003Cb\\u003E") != std::string::npos);
		REQUIRE(chunk.find("\\u0026") != std::string::npos);
	}

	SECTION("a rejection reason is escaped the same way")
	{
		// An exception message is as attacker-influenced as any other string, and it
		// reaches the page through a different function, so it needs the same treatment
		// rather than being trusted for being internal.
		const std::string chunk = deferred_reject_script(1, "</script><script>alert(1)</script>");
		REQUIRE(chunk.find("</script><script>") == std::string::npos);
		REQUIRE(chunk.find("\\u003C") != std::string::npos);
	}

	SECTION("quotes and backslashes cannot break out of the string")
	{
		const std::string chunk = deferred_reject_script(2, "he said \"hi\" \\ then left");
		// JSON encoding handles these; the check is that the reason really does go
		// through it rather than being pasted in.
		REQUIRE(chunk.find("\\\"hi\\\"") != std::string::npos);
	}
}

TEST_CASE("JavaScript line terminators are escaped, JSON ones are not", "[deferred][stream][security]")
{
	// U+2028 and U+2029 are ordinary characters in JSON and line terminators in
	// JavaScript, so an unescaped one splits the statement in two and everything after
	// it is a syntax error at best.
	// Split at each escape on purpose: a C++ hex escape consumes every hex digit that
	// follows it, so "\xA8after" is the single escape \xA8af rather than what it looks
	// like. Ending the string literal ends the escape with it.
	const nlohmann::json value = std::string("before\xE2\x80\xA8") + "after\xE2\x80\xA9" + "end";
	const std::string chunk = deferred_resolve_script(0, value);

	REQUIRE(chunk.find("\\u2028") != std::string::npos);
	REQUIRE(chunk.find("\\u2029") != std::string::npos);
	REQUIRE(chunk.find("\xE2\x80\xA8") == std::string::npos);
	REQUIRE(chunk.find("\xE2\x80\xA9") == std::string::npos);
}

TEST_CASE("escaping leaves ordinary text alone", "[deferred][stream]")
{
	// The escapes have to be invisible to JSON.parse, so anything that does not need
	// touching must come through untouched. An over-eager escaper would corrupt values
	// just as surely as an absent one lets them escape.
	const std::string plain = R"({"name":"Alex","count":42,"ok":true})";
	REQUIRE(escape_for_script(plain) == plain);

	SECTION("multi-byte text survives")
	{
		// Cyrillic shares a lead byte range with nothing being escaped here, but the
		// U+2028 check reads ahead, so a string starting with 0xE2 is worth a case.
		const std::string utf8 = "\"\xD0\x90\xD0\xBB\xD0\xB5\xD0\xBA\xD1\x81 \xE2\x82\xAC\"";
		REQUIRE(escape_for_script(utf8) == utf8);
	}

	SECTION("a lone 0xE2 near the end does not read past the buffer")
	{
		// The lookahead is two bytes; a truncated sequence at the tail must not be a
		// read past the end, and must not be mistaken for a line terminator.
		REQUIRE(escape_for_script("ok\xE2") == "ok\xE2");
		REQUIRE(escape_for_script("ok\xE2\x80") == "ok\xE2\x80");
	}
}

TEST_CASE("a resolve chunk names its slot", "[deferred][stream]")
{
	const nlohmann::json value = 1234;
	const std::string chunk = deferred_resolve_script(7, value);

	REQUIRE(chunk.find("__resolve(7,") != std::string::npos);
	REQUIRE(chunk.find("1234") != std::string::npos);
	REQUIRE(chunk.starts_with("<script>"));
	REQUIRE(chunk.find("</script>") != std::string::npos);
}

#endif  // COROUTE_HAS_TEMPLATES
