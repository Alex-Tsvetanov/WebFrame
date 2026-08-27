#include "coroute/http3/cid.hpp"

#ifdef COROUTE_HAS_HTTP3

#include <openssl/rand.h>

#include <algorithm>
#include <cstring>

namespace coroute::http3
{

	CidKey::CidKey(const std::uint8_t* data, std::size_t length) noexcept
	{
		if (data == nullptr || length == 0)
		{
			return;
		}
		len = static_cast<std::uint8_t>(std::min(length, max_cid_length));
		std::memcpy(bytes.data(), data, len);
	}

	bool CidKey::operator==(const CidKey& other) const noexcept
	{
		// Compares only the meaningful prefix: the tail of bytes is zero-filled
		// padding, not part of the identifier.
		return len == other.len && std::memcmp(bytes.data(), other.bytes.data(), len) == 0;
	}

	bool cid_fill(std::span<std::uint8_t> out, std::size_t worker_index) noexcept
	{
		if (out.empty() || out.size() > max_cid_length)
		{
			return false;
		}

		// RAND_bytes rather than a seeded PRNG. A guessable connection ID lets an
		// off-path attacker inject packets into another peer's connection, so this is
		// a security requirement and a failure has to be fatal to the connection.
		if (RAND_bytes(out.data(), static_cast<int>(out.size())) != 1)
		{
			return false;
		}

		// Byte 0 identifies the owning worker. Plaintext, which is a deliberate
		// simplification: it leaks only how many workers the server runs, and it makes
		// routing a single array access.
		//
		// ponytail: plaintext worker byte. If an off-path attacker being able to aim
		// packets at a chosen worker ever matters, encrypt the first four bytes with a
		// per-server AES-128 key, which is what nginx and quiche do. That changes only
		// this function and cid_worker.
		out[0] = static_cast<std::uint8_t>(worker_index & 0xFF);
		return true;
	}

	std::size_t cid_worker(std::span<const std::uint8_t> dcid, std::size_t worker_count) noexcept
	{
		if (worker_count <= 1 || dcid.empty())
		{
			return 0;
		}
		return static_cast<std::size_t>(dcid[0]) % worker_count;
	}

}  // namespace coroute::http3

namespace std
{
	std::size_t hash<coroute::http3::CidKey>::operator()(const coroute::http3::CidKey& cid) const noexcept
	{
		// FNV-1a over the meaningful prefix. The bytes after the worker marker are
		// already cryptographically random, so there is nothing to gain from a
		// stronger mixer here.
		std::size_t h = 1469598103934665603ULL;
		for (std::uint8_t byte : cid.view())
		{
			h ^= byte;
			h *= 1099511628211ULL;
		}
		return h;
	}
}  // namespace std

#endif  // COROUTE_HAS_HTTP3
