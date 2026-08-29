#include <catch2/catch_test_macros.hpp>
#include <coroute/core/chunked.hpp>
#include <coroute/net/io_context.hpp>

#include <algorithm>
#include <cstring>
#include <string>

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

// ============================================================================
// The handover from the request parser
// ============================================================================

namespace
{
	// Serves a scripted stream and records how much of it was read, so a test can tell
	// whether the reader took octets off the connection that it should already have had.
	class ScriptedConn final : public net::Connection
	{
	public:
		explicit ScriptedConn(std::string rest) : rest_(std::move(rest)) { }

		Task<net::ReadResult> async_read(void* buffer, size_t len) override
		{
			++reads;
			const size_t n = std::min(len, rest_.size());
			std::memcpy(buffer, rest_.data(), n);
			rest_.erase(0, n);
			co_return n;
		}

		Task<net::ReadResult> async_read_until(void* buffer, size_t len, char) override
		{
			co_return co_await async_read(buffer, len);
		}

		Task<net::WriteResult> async_write(const void*, size_t len) override { co_return len; }
		Task<net::WriteResult> async_write_all(const void*, size_t len) override { co_return len; }
		Task<net::TransmitResult> async_transmit_file(net::FileHandle, size_t, size_t) override
		{
			co_return size_t{0};
		}

		void close() override { }
		bool is_open() const noexcept override { return true; }
		void set_timeout(std::chrono::milliseconds) override { }
		std::string remote_address() const override { return "203.0.113.9"; }
		uint16_t remote_port() const noexcept override { return 40000; }
		void set_cancellation_token(CancellationToken) override { }

		int reads = 0;

	private:
		std::string rest_;
	};
}  // namespace

TEST_CASE("A chunked body split between the header read and the socket", "[chunked]")
{
	// This is the case the request parser actually produces: the read that found the
	// blank line ending the headers carried part of the body with it, so those octets
	// are already off the socket and the rest is still on it. A reader that only knows
	// how to read from the socket starts in the middle of the first chunk.
	const std::string body = "5\r\nHello\r\n6\r\n World\r\n0\r\n\r\n";

	SECTION("all of it already read")
	{
		ScriptedConn conn("");
		ChunkedBodyReader reader(&conn, body);
		auto result = reader.read_all().sync_wait();
		REQUIRE(result.has_value());
		CHECK(*result == "Hello World");
		// Nothing was left to fetch, so the socket was never touched.
		CHECK(conn.reads == 0);
	}

	SECTION("split at every position")
	{
		for (std::size_t split = 0; split <= body.size(); ++split)
		{
			ScriptedConn conn(body.substr(split));
			ChunkedBodyReader reader(&conn, body.substr(0, split));
			auto result = reader.read_all().sync_wait();
			INFO("split at " << split);
			REQUIRE(result.has_value());
			CHECK(*result == "Hello World");
		}
	}

	SECTION("what follows the body is not consumed as part of it")
	{
		// A pipelined second request after the terminating chunk. It must not appear in
		// the body, whatever the reader does with the octets afterwards.
		ScriptedConn conn(body + "GET /next HTTP/1.1\r\n\r\n");
		ChunkedBodyReader reader(&conn, "");
		auto result = reader.read_all().sync_wait();
		REQUIRE(result.has_value());
		CHECK(*result == "Hello World");
		CHECK(reader.finished());
	}
}
