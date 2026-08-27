#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

namespace coroute::util
{

	// ============================================================================
	// SHA-1 (RFC 3174) and Base64 (RFC 4648)
	// ============================================================================
	//
	// These exist for the RFC 6455 Sec-WebSocket-Accept handshake, which is a fixed
	// protocol transform rather than a security primitive: it only proves the peer
	// understood the WebSocket upgrade and stops naive caching proxies from replaying
	// it. Keeping them here means a plaintext WebSocket server does not drag in a TLS
	// library for a 20 byte digest.
	//
	// SHA-1 is broken for collision resistance. Do not reach for this for anything that
	// depends on that property.

	namespace detail
	{

		constexpr uint32_t rotl32(uint32_t value, int bits) noexcept
		{
			return (value << bits) | (value >> (32 - bits));
		}

	}  // namespace detail

	inline std::array<uint8_t, 20> sha1(const void* data, size_t len) noexcept
	{
		const auto* msg = static_cast<const uint8_t*>(data);
		std::array<uint32_t, 5> h{0x67452301U, 0xEFCDAB89U, 0x98BADCFEU, 0x10325476U, 0xC3D2E1F0U};

		auto process = [&h](const uint8_t* chunk) noexcept
		{
			std::array<uint32_t, 80> w{};
			for (size_t i = 0; i < 16; ++i)
			{
				w[i] = (static_cast<uint32_t>(chunk[i * 4]) << 24) | (static_cast<uint32_t>(chunk[i * 4 + 1]) << 16) |
				       (static_cast<uint32_t>(chunk[i * 4 + 2]) << 8) | static_cast<uint32_t>(chunk[i * 4 + 3]);
			}
			for (size_t i = 16; i < 80; ++i)
			{
				w[i] = detail::rotl32(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1);
			}

			uint32_t a = h[0];
			uint32_t b = h[1];
			uint32_t c = h[2];
			uint32_t d = h[3];
			uint32_t e = h[4];

			for (size_t i = 0; i < 80; ++i)
			{
				uint32_t f = 0;
				uint32_t k = 0;
				if (i < 20)
				{
					f = (b & c) | (~b & d);
					k = 0x5A827999U;
				}
				else if (i < 40)
				{
					f = b ^ c ^ d;
					k = 0x6ED9EBA1U;
				}
				else if (i < 60)
				{
					f = (b & c) | (b & d) | (c & d);
					k = 0x8F1BBCDCU;
				}
				else
				{
					f = b ^ c ^ d;
					k = 0xCA62C1D6U;
				}

				const uint32_t tmp = detail::rotl32(a, 5) + f + e + k + w[i];
				e = d;
				d = c;
				c = detail::rotl32(b, 30);
				b = a;
				a = tmp;
			}

			h[0] += a;
			h[1] += b;
			h[2] += c;
			h[3] += d;
			h[4] += e;
		};

		const uint64_t bit_len = static_cast<uint64_t>(len) * 8;

		size_t pos = 0;
		while (len - pos >= 64)
		{
			process(msg + pos);
			pos += 64;
		}

		// Tail: the remaining bytes, the 0x80 terminator, zero fill, then the message
		// length in bits as a big-endian 64-bit value. Needs a second block when the
		// remainder leaves no room for that length field.
		const size_t rem = len - pos;
		std::array<uint8_t, 64> block{};
		std::copy_n(msg + pos, rem, block.begin());
		block[rem] = 0x80;

		if (rem >= 56)
		{
			process(block.data());
			block.fill(0);
		}

		for (size_t i = 0; i < 8; ++i)
		{
			block[63 - i] = static_cast<uint8_t>(bit_len >> (i * 8));
		}
		process(block.data());

		std::array<uint8_t, 20> out{};
		for (size_t i = 0; i < 5; ++i)
		{
			out[i * 4] = static_cast<uint8_t>(h[i] >> 24);
			out[i * 4 + 1] = static_cast<uint8_t>(h[i] >> 16);
			out[i * 4 + 2] = static_cast<uint8_t>(h[i] >> 8);
			out[i * 4 + 3] = static_cast<uint8_t>(h[i]);
		}
		return out;
	}

	inline std::string base64_encode(const uint8_t* data, size_t len)
	{
		constexpr std::string_view table = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

		std::string out;
		out.reserve(((len + 2) / 3) * 4);

		size_t i = 0;
		for (; i + 2 < len; i += 3)
		{
			const uint32_t triple = (static_cast<uint32_t>(data[i]) << 16) | (static_cast<uint32_t>(data[i + 1]) << 8) |
			                        static_cast<uint32_t>(data[i + 2]);
			out.push_back(table[(triple >> 18) & 0x3F]);
			out.push_back(table[(triple >> 12) & 0x3F]);
			out.push_back(table[(triple >> 6) & 0x3F]);
			out.push_back(table[triple & 0x3F]);
		}

		if (i < len)
		{
			const bool has_second = (i + 1 < len);
			uint32_t triple = static_cast<uint32_t>(data[i]) << 16;
			if (has_second)
			{
				triple |= static_cast<uint32_t>(data[i + 1]) << 8;
			}
			out.push_back(table[(triple >> 18) & 0x3F]);
			out.push_back(table[(triple >> 12) & 0x3F]);
			out.push_back(has_second ? table[(triple >> 6) & 0x3F] : '=');
			out.push_back('=');
		}

		return out;
	}

}  // namespace coroute::util
