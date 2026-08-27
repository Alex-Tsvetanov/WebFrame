#include <catch2/catch_test_macros.hpp>
#include <coroute/util/sha1.hpp>

#include <string>
#include <string_view>

using namespace coroute;

namespace
{

	std::string hex(const std::array<uint8_t, 20>& digest)
	{
		constexpr std::string_view digits = "0123456789abcdef";
		std::string out;
		out.reserve(40);
		for (uint8_t byte : digest)
		{
			out.push_back(digits[byte >> 4]);
			out.push_back(digits[byte & 0x0F]);
		}
		return out;
	}

	std::string sha1_hex(std::string_view input) { return hex(util::sha1(input.data(), input.size())); }

	std::string b64(std::string_view input)
	{
		return util::base64_encode(reinterpret_cast<const uint8_t*>(input.data()),  // NOLINT
		                           input.size());
	}

}  // namespace

TEST_CASE("SHA-1 known vectors", "[sha1]")
{
	// RFC 3174 section 7.3 and FIPS 180-1 appendix A/B.
	SECTION("empty input") { REQUIRE(sha1_hex("") == "da39a3ee5e6b4b0d3255bfef95601890afd80709"); }

	SECTION("abc") { REQUIRE(sha1_hex("abc") == "a9993e364706816aba3e25717850c26c9cd0d89d"); }

	SECTION("two block message")
	{
		REQUIRE(sha1_hex("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq") ==
		        "84983e441c3bd26ebaae4aa1f95129e5e54670f1");
	}

	// Exercises the padding branch: 55 bytes fits the length field in one block,
	// 56 and 64 force a second block. This is where a hand-rolled SHA-1 breaks.
	SECTION("padding boundaries")
	{
		REQUIRE(sha1_hex(std::string(55, 'a')) == "c1c8bbdc22796e28c0e15163d20899b65621d65a");
		REQUIRE(sha1_hex(std::string(56, 'a')) == "c2db330f6083854c99d4b5bfb6e8f29f201be699");
		REQUIRE(sha1_hex(std::string(63, 'a')) == "03f09f5b158a7a8cdad920bddc29b81c18a551f5");
		REQUIRE(sha1_hex(std::string(64, 'a')) == "0098ba824b5c16427bd7a1122a5a442a25ec644d");
		REQUIRE(sha1_hex(std::string(65, 'a')) == "11655326c708d70319be2610e8a57d9a5b959d3b");
	}
}

TEST_CASE("Base64 known vectors", "[sha1][base64]")
{
	// RFC 4648 section 10.
	SECTION("all padding cases")
	{
		REQUIRE(b64("") == "");
		REQUIRE(b64("f") == "Zg==");
		REQUIRE(b64("fo") == "Zm8=");
		REQUIRE(b64("foo") == "Zm9v");
		REQUIRE(b64("foob") == "Zm9vYg==");
		REQUIRE(b64("fooba") == "Zm9vYmE=");
		REQUIRE(b64("foobar") == "Zm9vYmFy");
	}
}

TEST_CASE("RFC 6455 Sec-WebSocket-Accept", "[sha1][websocket]")
{
	// RFC 6455 section 1.3. This is the vector that matters: it is the exact
	// transform the handshake performs, and it exercises sha1 and base64 together.
	const std::string combined = std::string("dGhlIHNhbXBsZSBub25jZQ==") + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
	const auto digest = util::sha1(combined.data(), combined.size());
	REQUIRE(util::base64_encode(digest.data(), digest.size()) == "s3pPLMBiTxaQ9kYGzzhZRbK+xOo=");
}
