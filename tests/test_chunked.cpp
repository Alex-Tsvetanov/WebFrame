#include <catch2/catch_test_macros.hpp>
#include <coroute/core/chunked.hpp>

using namespace coroute;

TEST_CASE("Chunked encoding utilities", "[chunked]")
{
	SECTION("encode_chunk creates valid chunk format")
	{
		auto chunk = chunked::encode_chunk("Hello");
		CHECK(chunk == "5\r\nHello\r\n");

		chunk = chunked::encode_chunk("Hello, World!");
		CHECK(chunk == "d\r\nHello, World!\r\n");  // 13 = 0xd
	}

	SECTION("encode_chunk handles empty data")
	{
		auto chunk = chunked::encode_chunk("");
		CHECK(chunk.empty());
	}

	SECTION("encode_final_chunk creates terminator")
	{
		auto final_chunk = chunked::encode_final_chunk();
		CHECK(final_chunk == "0\r\n\r\n");
	}

	SECTION("encode_final_chunk with trailers")
	{
		std::vector<std::pair<std::string, std::string>> trailers = {
			{"X-Checksum", "abc123"},
            {   "X-Count",     "42"}
        };
		auto final_chunk = chunked::encode_final_chunk(trailers);
		CHECK(final_chunk == "0\r\nX-Checksum: abc123\r\nX-Count: 42\r\n\r\n");
	}
}

TEST_CASE("Chunked size parsing", "[chunked]")
{
	SECTION("parse_chunk_size handles hex values")
	{
		CHECK(chunked::parse_chunk_size("0") == 0);
		CHECK(chunked::parse_chunk_size("5") == 5);
		CHECK(chunked::parse_chunk_size("a") == 10);
		CHECK(chunked::parse_chunk_size("A") == 10);
		CHECK(chunked::parse_chunk_size("f") == 15);
		CHECK(chunked::parse_chunk_size("F") == 15);
		CHECK(chunked::parse_chunk_size("10") == 16);
		CHECK(chunked::parse_chunk_size("ff") == 255);
		CHECK(chunked::parse_chunk_size("FF") == 255);
		CHECK(chunked::parse_chunk_size("100") == 256);
		CHECK(chunked::parse_chunk_size("1000") == 4096);
	}

	SECTION("parse_chunk_size handles whitespace")
	{
		CHECK(chunked::parse_chunk_size("  5  ") == 5);
		CHECK(chunked::parse_chunk_size("\t10\t") == 16);
	}

	SECTION("parse_chunk_size handles chunk extensions")
	{
		CHECK(chunked::parse_chunk_size("5;name=value") == 5);
		CHECK(chunked::parse_chunk_size("a;ext1;ext2") == 10);
		CHECK(chunked::parse_chunk_size("ff;foo=bar;baz") == 255);
	}

	SECTION("parse_chunk_size returns -1 for invalid input")
	{
		CHECK(chunked::parse_chunk_size("") == -1);
		CHECK(chunked::parse_chunk_size("g") == -1);  // Invalid hex
		CHECK(chunked::parse_chunk_size("xyz") == -1);
		CHECK(chunked::parse_chunk_size("-1") == -1);
	}
}

TEST_CASE("ChunkedResponse configuration", "[chunked]")
{
	ChunkedResponse resp;

	SECTION("Initial state")
	{
		CHECK_FALSE(resp.headers_sent());
		CHECK_FALSE(resp.finished());
	}

	SECTION("Fluent API")
	{
		resp.status(201).header("X-Custom", "value").content_type("application/json").trailer("X-Checksum", "abc");

		// Can't easily verify without connection, but at least it compiles
		CHECK_FALSE(resp.headers_sent());
	}
}

// ============================================================================
// Regressions from fuzz_chunk_size
// ============================================================================

TEST_CASE("A chunk size cannot overflow its way to a small number", "[chunked]")
{
	// This was int64_t accumulated with a bare multiply and no bound, so a long line
	// overflowed a signed integer. That is undefined rather than merely wrong: the
	// caller checks only for a negative result, and a compiler entitled to assume the
	// overflow cannot happen is entitled to drop that check.
	CHECK(chunked::parse_chunk_size("FFFFFFFFFFFFFFFFFFFF") == -1);
	CHECK(chunked::parse_chunk_size("10000000000000000") == -1);
	CHECK(chunked::parse_chunk_size(std::string(64, 'F')) == -1);
}

TEST_CASE("A chunk is bounded by the same limit as a body", "[chunked]")
{
	// Chunked and unchunked framing describe the same quantity. app.cpp rejects a
	// Content-Length body over ten megabytes; a chunk claiming more than that used to
	// pass straight through to a read of that size.
	CHECK(chunked::parse_chunk_size("A00000") == 10485760);  // exactly the limit
	CHECK(chunked::parse_chunk_size("A00001") == -1);        // one octet past it
	CHECK(chunked::parse_chunk_size("BB00A0") == -1);        // the input the fuzzer found

	// Ordinary sizes are unaffected, including the terminating zero chunk and the
	// extension syntax after a semicolon.
	CHECK(chunked::parse_chunk_size("0") == 0);
	CHECK(chunked::parse_chunk_size("1a") == 26);
	CHECK(chunked::parse_chunk_size("1A") == 26);
	CHECK(chunked::parse_chunk_size(" ff ") == 255);
	CHECK(chunked::parse_chunk_size("ff;ext=1") == 255);
	CHECK(chunked::parse_chunk_size("") == -1);
	CHECK(chunked::parse_chunk_size("zz") == -1);
}
