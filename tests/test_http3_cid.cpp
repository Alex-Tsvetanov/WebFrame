#include <catch2/catch_test_macros.hpp>

#ifdef COROUTE_HAS_HTTP3

#include <coroute/http3/cid.hpp>

#include <array>
#include <cstdint>
#include <set>
#include <unordered_map>
#include <vector>

using namespace coroute::http3;

namespace
{

	CidKey make_cid(std::size_t worker_index, std::size_t length = server_cid_length)
	{
		std::vector<std::uint8_t> raw(length);
		REQUIRE(cid_fill(raw, worker_index));
		return CidKey{raw.data(), raw.size()};
	}

}  // namespace

TEST_CASE("connection IDs carry their owning worker", "[http3][cid]")
{
	SECTION("the worker index survives a round trip")
	{
		// This is the property the whole portable CID-routing design rests on: a
		// datagram that lands on the wrong worker after a client migrates must be
		// attributable to the worker that actually owns the connection.
		for (std::size_t workers : {std::size_t{1}, std::size_t{2}, std::size_t{4}, std::size_t{8}, std::size_t{16}})
		{
			for (std::size_t worker = 0; worker < workers; ++worker)
			{
				const CidKey cid = make_cid(worker);
				INFO("workers=" << workers << " worker=" << worker);
				REQUIRE(cid_worker(cid.view(), workers) == worker);
			}
		}
	}

	SECTION("a connection ID from a differently sized run still maps somewhere valid")
	{
		// A peer can echo a connection ID issued by an earlier run of the server with
		// a different worker count. The result must stay in range rather than index
		// past the end of the worker table.
		const CidKey cid = make_cid(200);
		for (std::size_t workers = 1; workers <= 16; ++workers)
		{
			INFO("workers=" << workers);
			REQUIRE(cid_worker(cid.view(), workers) < workers);
		}
	}

	SECTION("degenerate inputs answer worker 0 rather than misbehaving")
	{
		// Worker 0 exists in every configuration, so an unroutable packet is dropped
		// there exactly as it would be anywhere else.
		REQUIRE(cid_worker({}, 4) == 0);
		REQUIRE(cid_worker(make_cid(3).view(), 1) == 0);
		REQUIRE(cid_worker(make_cid(3).view(), 0) == 0);
	}
}

TEST_CASE("connection IDs are unpredictable", "[http3][cid]")
{
	// RFC 9000 section 5.1 requires connection IDs an off-path observer cannot guess.
	// A guessable ID lets an attacker inject packets into someone else's connection,
	// so this is a security property rather than a quality-of-implementation nicety.
	SECTION("no two generated IDs collide")
	{
		constexpr int samples = 2000;
		std::set<std::vector<std::uint8_t>> seen;
		for (int i = 0; i < samples; ++i)
		{
			const CidKey cid = make_cid(i % 8);
			auto v = cid.view();
			seen.emplace(v.begin(), v.end());
		}
		REQUIRE(seen.size() == samples);
	}

	SECTION("the bytes after the worker marker vary")
	{
		// If the random fill silently failed, every ID for a given worker would be
		// identical past byte 0 and the set below would collapse to one entry.
		std::set<std::vector<std::uint8_t>> tails;
		for (int i = 0; i < 200; ++i)
		{
			const CidKey cid = make_cid(1);
			auto v = cid.view();
			tails.emplace(v.begin() + 1, v.end());
		}
		REQUIRE(tails.size() == 200);
	}
}

TEST_CASE("CidKey works as a map key", "[http3][cid]")
{
	SECTION("equality compares only the meaningful prefix")
	{
		std::array<std::uint8_t, 4> raw{0x01, 0x02, 0x03, 0x04};
		const CidKey a{raw.data(), raw.size()};
		const CidKey b{raw.data(), raw.size()};
		REQUIRE(a == b);

		// Same bytes, different length: a different connection ID.
		const CidKey shorter{raw.data(), 3};
		REQUIRE_FALSE(a == shorter);
	}

	SECTION("connections can be looked up by their ID")
	{
		// The endpoint keeps its connection table keyed this way, so a lookup miss
		// here would mean every packet after the first is treated as a new connection.
		std::unordered_map<CidKey, int> table;
		std::vector<CidKey> issued;

		for (int i = 0; i < 500; ++i)
		{
			CidKey cid = make_cid(static_cast<std::size_t>(i) % 4);
			table[cid] = i;
			issued.push_back(cid);
		}

		REQUIRE(table.size() == issued.size());
		for (int i = 0; i < static_cast<int>(issued.size()); ++i)
		{
			auto it = table.find(issued[static_cast<std::size_t>(i)]);
			REQUIRE(it != table.end());
			REQUIRE(it->second == i);
		}
	}

	SECTION("an empty ID is distinct from any real one")
	{
		const CidKey empty;
		REQUIRE(empty.len == 0);
		REQUIRE_FALSE(empty == make_cid(0));
	}
}

#endif  // COROUTE_HAS_HTTP3
