#include <catch2/catch_test_macros.hpp>
#include <coroute/net/protocol_detect.hpp>

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <deque>
#include <string>
#include <string_view>
#include <vector>

using namespace coroute;
using namespace coroute::net;

namespace
{

	std::vector<std::uint8_t> bytes_of(std::string_view text) { return {text.begin(), text.end()}; }

	// A Connection that serves a scripted sequence of reads. Each element of the
	// script is returned by one async_read call, which is what lets these tests
	// reproduce a peer that dribbles bytes one segment at a time without opening a
	// socket. Writes are recorded so delegation can be asserted.
	class ScriptedConnection final : public Connection
	{
	public:
		explicit ScriptedConnection(std::deque<std::string> script) : script_(std::move(script)) { }

		Task<ReadResult> async_read(void* buffer, size_t len) override
		{
			++read_calls;
			if (script_.empty())
			{
				co_return size_t{0};  // peer closed
			}
			std::string& front = script_.front();
			const size_t n = std::min(len, front.size());
			std::memcpy(buffer, front.data(), n);
			front.erase(0, n);
			if (front.empty())
			{
				script_.pop_front();
			}
			co_return n;
		}

		// The scripted stream is the same bytes either way; the pushback layer is what
		// these tests are exercising, not the backend's own delimiter scan.
		Task<ReadResult> async_read_until(void* buffer, size_t len, char delimiter) override
		{
			auto* out = static_cast<char*>(buffer);
			size_t total = 0;
			while (total < len)
			{
				auto n = co_await async_read(out + total, 1);
				if (!n || *n == 0)
				{
					break;
				}
				++total;
				if (out[total - 1] == delimiter)
				{
					break;
				}
			}
			co_return total;
		}

		Task<WriteResult> async_write(const void* buffer, size_t len) override
		{
			written.append(static_cast<const char*>(buffer), len);
			co_return len;
		}

		Task<WriteResult> async_write_all(const void* buffer, size_t len) override
		{
			co_return co_await async_write(buffer, len);
		}

		Task<TransmitResult> async_transmit_file(FileHandle, size_t, size_t) override { co_return size_t{0}; }

		void close() override { open_ = false; }
		bool is_open() const noexcept override { return open_; }
		void set_timeout(std::chrono::milliseconds) override { }
		std::string remote_address() const override { return "203.0.113.7"; }
		uint16_t remote_port() const noexcept override { return 44321; }
		void set_cancellation_token(CancellationToken) override { }

		int read_calls = 0;
		std::string written;

	private:
		std::deque<std::string> script_;
		bool open_ = true;
	};

	std::unique_ptr<ScriptedConnection> scripted(std::deque<std::string> script)
	{
		return std::make_unique<ScriptedConnection>(std::move(script));
	}

	std::string read_n(Connection& conn, size_t n)
	{
		std::string out(n, '\0');
		auto result = conn.async_read(out.data(), n).sync_wait();
		REQUIRE(result.has_value());
		out.resize(*result);
		return out;
	}

	WireProtocol classify_text(std::string_view text)
	{
		const auto buf = bytes_of(text);
		return classify(buf);
	}

	PrefaceMatch match_text(std::string_view text)
	{
		const auto buf = bytes_of(text);
		return preface_match(buf);
	}

}  // namespace

TEST_CASE("First-octet classification", "[protocol_detect]")
{
	SECTION("TLS handshake records")
	{
		// A real ClientHello: ContentType 0x16, version 0x0301, then the record length.
		const std::vector<std::uint8_t> client_hello{0x16, 0x03, 0x01, 0x00, 0xA5, 0x01};
		REQUIRE(classify(client_hello) == WireProtocol::Tls);

		// One octet is enough. This is the whole point of the design.
		const std::vector<std::uint8_t> single{0x16};
		REQUIRE(classify(single) == WireProtocol::Tls);
	}

	SECTION("every HTTP method token classifies as cleartext")
	{
		for (std::string_view method : {"GET /", "POST /", "PUT /", "DELETE /", "HEAD /", "OPTIONS /", "PATCH /",
		                                "TRACE /", "CONNECT ", "PROPFIND ", "PRI * HTTP/2.0"})
		{
			INFO("method token: " << method);
			REQUIRE(classify_text(method) == WireProtocol::Cleartext);
		}
	}

	SECTION("empty input is Unknown, never a guess") { REQUIRE(classify({}) == WireProtocol::Unknown); }

	SECTION("neither TLS nor an HTTP token")
	{
		// Lowercase: HTTP method tokens are uppercase, so this is not a valid request.
		REQUIRE(classify_text("get /") == WireProtocol::Unknown);
		// Other TLS ContentTypes cannot open a connection: 0x15 is alert, 0x17 is
		// application data. Seeing one first means the peer is out of sync.
		const std::vector<std::uint8_t> alert{0x15, 0x03, 0x03};
		REQUIRE(classify(alert) == WireProtocol::Unknown);
		const std::vector<std::uint8_t> app_data{0x17, 0x03, 0x03};
		REQUIRE(classify(app_data) == WireProtocol::Unknown);
		// SSLv2 ClientHello, which is not supported and must not be mistaken for TLS.
		const std::vector<std::uint8_t> sslv2{0x80, 0x2E, 0x01};
		REQUIRE(classify(sslv2) == WireProtocol::Unknown);
		// Binary junk.
		const std::vector<std::uint8_t> junk{0x00, 0xFF, 0xFE};
		REQUIRE(classify(junk) == WireProtocol::Unknown);
	}
}

TEST_CASE("HTTP/2 preface matching is incremental", "[protocol_detect]")
{
	SECTION("the exact RFC 9113 preface")
	{
		REQUIRE(http2_client_preface == "PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n");
		REQUIRE(http2_client_preface.size() == 24);
		REQUIRE(match_text(http2_client_preface) == PrefaceMatch::Yes);
	}

	SECTION("trailing data after the preface still matches")
	{
		REQUIRE(match_text("PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n\x00\x00\x00\x04") == PrefaceMatch::Yes);
	}

	SECTION("every prefix is Maybe, never a premature verdict")
	{
		for (std::size_t n = 0; n < http2_client_preface.size(); ++n)
		{
			INFO("prefix length: " << n);
			REQUIRE(match_text(http2_client_preface.substr(0, n)) == PrefaceMatch::Maybe);
		}
	}

	SECTION("real methods are rejected within three octets")
	{
		// This is the slowloris property: a peer cannot hold the classifier open by
		// dribbling bytes, because genuine traffic diverges almost immediately.
		struct Case
		{
			std::string_view method;
			std::size_t decisive;  // octets needed before the verdict is No
		};

		// GET differs at index 0. POST, PUT and PATCH share only "P". PROPFIND shares
		// "PR" and is the worst case at three octets.
		for (const Case c : {
				 Case{     "GET / HTTP/1.1", 1},
                 Case{    "POST / HTTP/1.1", 2},
                 Case{     "PUT / HTTP/1.1", 2},
				 Case{   "PATCH / HTTP/1.1", 2},
                 Case{"PROPFIND / HTTP/1.1", 3}
        })
		{
			INFO("method: " << c.method << " decisive at " << c.decisive);
			REQUIRE(match_text(c.method.substr(0, c.decisive)) == PrefaceMatch::No);
			if (c.decisive > 1)
			{
				// One octet short of decisive must still be undecided, otherwise the
				// stated bound is wrong.
				REQUIRE(match_text(c.method.substr(0, c.decisive - 1)) == PrefaceMatch::Maybe);
			}
			REQUIRE(match_text(c.method) == PrefaceMatch::No);
		}
	}

	SECTION("a near miss in the trailing SM section is still rejected")
	{
		// Corrupting the last octet must not be mistaken for the preface.
		std::string near_miss(http2_client_preface);
		near_miss.back() = 'X';
		REQUIRE(match_text(near_miss) == PrefaceMatch::No);
	}

	SECTION("empty input is undecided, not a match") { REQUIRE(preface_match({}) == PrefaceMatch::Maybe); }
}

TEST_CASE("PrefaceConnection replays consumed bytes", "[protocol_detect]")
{
	SECTION("replayed bytes come back before socket bytes")
	{
		auto inner = scripted({"world"});
		PrefaceConnection conn(std::move(inner), bytes_of("hello "));

		REQUIRE(conn.has_pushback());
		REQUIRE(read_n(conn, 6) == "hello ");
		REQUIRE_FALSE(conn.has_pushback());
		REQUIRE(read_n(conn, 5) == "world");
	}

	SECTION("a short read takes only part of the pushback")
	{
		auto inner = scripted({"world"});
		PrefaceConnection conn(std::move(inner), bytes_of("hello "));

		REQUIRE(read_n(conn, 2) == "he");
		REQUIRE(read_n(conn, 2) == "ll");
		REQUIRE(read_n(conn, 99) == "o ");
		REQUIRE_FALSE(conn.has_pushback());
		REQUIRE(read_n(conn, 99) == "world");
	}

	SECTION("read_until finds a delimiter inside the pushback without touching the socket")
	{
		auto* raw = new ScriptedConnection({"never read"});
		PrefaceConnection conn(std::unique_ptr<Connection>(raw), bytes_of("GET / HTTP/1.1\r\nrest"));

		std::string out(64, '\0');
		auto n = conn.async_read_until(out.data(), out.size(), '\n').sync_wait();
		REQUIRE(n.has_value());
		out.resize(*n);
		REQUIRE(out == "GET / HTTP/1.1\r\n");
		REQUIRE(raw->read_calls == 0);
	}

	SECTION("read_until spanning pushback and socket returns one contiguous result")
	{
		// The delimiter is not in the replayed bytes, so the call has to stitch the
		// pushback and the socket together. Getting this wrong silently truncates the
		// first request line of every classified connection.
		auto inner = scripted({"1.1\r\n", "Host: x\r\n"});
		PrefaceConnection conn(std::move(inner), bytes_of("GET / HTTP/"));

		std::string out(64, '\0');
		auto n = conn.async_read_until(out.data(), out.size(), '\n').sync_wait();
		REQUIRE(n.has_value());
		out.resize(*n);
		REQUIRE(out == "GET / HTTP/1.1\r\n");
	}

	SECTION("writes and metadata delegate to the wrapped connection")
	{
		auto* raw = new ScriptedConnection({});
		PrefaceConnection conn(std::unique_ptr<Connection>(raw), bytes_of("x"));

		REQUIRE(conn.async_write_all("pong", 4).sync_wait().has_value());
		REQUIRE(raw->written == "pong");
		REQUIRE(conn.remote_address() == "203.0.113.7");
		REQUIRE(conn.remote_port() == 44321);
		REQUIRE(conn.is_open());
		conn.close();
		REQUIRE_FALSE(conn.is_open());
	}
}

TEST_CASE("read_prefix accumulates across short reads", "[protocol_detect]")
{
	SECTION("one byte is enough to classify TLS")
	{
		auto result = read_prefix(scripted({std::string("\x16\x03\x01", 3)}), 1).sync_wait();
		REQUIRE(result.has_value());
		REQUIRE(classify(result->bytes) == WireProtocol::Tls);
	}

	SECTION("a peer dribbling one byte per segment still yields the full preface")
	{
		// This is the case a naive classifier gets wrong: it reads once, sees three
		// bytes, and decides. read_prefix must keep reading until min_bytes is met.
		std::deque<std::string> one_at_a_time;
		for (char c : http2_client_preface)
		{
			one_at_a_time.emplace_back(1, c);
		}

		auto result = read_prefix(scripted(std::move(one_at_a_time)), http2_client_preface.size()).sync_wait();
		REQUIRE(result.has_value());
		REQUIRE(result->bytes.size() == http2_client_preface.size());
		REQUIRE(preface_match(result->bytes) == PrefaceMatch::Yes);

		// And the bytes are still there for the layer underneath.
		std::string replayed(http2_client_preface.size(), '\0');
		auto n = result->conn->async_read(replayed.data(), replayed.size()).sync_wait();
		REQUIRE(n.has_value());
		replayed.resize(*n);
		REQUIRE(replayed == http2_client_preface);
	}

	SECTION("a peer that closes before sending enough is an error, not a short prefix")
	{
		auto result = read_prefix(scripted({"PR"}), http2_client_preface.size()).sync_wait();
		REQUIRE_FALSE(result.has_value());
	}

	SECTION("a null connection is rejected rather than dereferenced")
	{
		auto result = read_prefix(nullptr, 1).sync_wait();
		REQUIRE_FALSE(result.has_value());
	}
}
