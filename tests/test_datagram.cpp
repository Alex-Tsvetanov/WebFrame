#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/generators/catch_generators_range.hpp>

#include <coroute/net/datagram.hpp>
#include <coroute/coro/task.hpp>

#include "io_backend_arms.hpp"

#include <atomic>
#include <cstring>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#if defined(COROUTE_PLATFORM_WINDOWS)
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

using namespace coroute;
using namespace coroute::net;

namespace
{

	// A bare REQUIRE(result.has_value()) reports "false" and nothing else, which turns
	// an intermittent I/O failure into a guessing game. The reason is right there in
	// the error, so it belongs in the output.
	template <typename T>
	void require_ok(const expected<T, Error>& result, std::string_view what)
	{
		if (!result)
		{
			FAIL(what << " failed: " << std::string(result.error().message()));
		}
	}

	// Built here rather than exposed on the public interface: nothing outside a test
	// needs to invent an address, since real callers reply to the peer Endpoint that
	// arrived with the datagram.
	Endpoint loopback(uint16_t port)
	{
		sockaddr_in addr{};
		addr.sin_family = AF_INET;
		addr.sin_port = htons(port);
		addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

		Endpoint ep;
		std::memcpy(ep.bytes.data(), &addr, sizeof(addr));
		ep.len = sizeof(addr);
		return ep;
	}

	std::span<const std::uint8_t> bytes_of(std::string_view text)
	{
		return {reinterpret_cast<const std::uint8_t*>(text.data()), text.size()};  // NOLINT
	}

	std::string to_string(std::span<const std::uint8_t> data)
	{
		return {reinterpret_cast<const char*>(data.data()), data.size()};  // NOLINT
	}

	// Runs the event loop on a background thread so sync_wait can block the test
	// thread waiting for a coroutine that only the loop can resume.
	//
	// start() is separate from construction so a backend without datagram support can
	// be detected and skipped without ever spinning the loop up.
	struct LoopHarness
	{
		// Taken rather than created, so the caller decides the backend and so a host
		// that refuses one can skip before anything is constructed.
		std::unique_ptr<IoContext> ctx;
		std::thread thread;

		explicit LoopHarness(std::unique_ptr<IoContext> context) : ctx(std::move(context)) { }

		void start()
		{
			thread = std::thread([this] { ctx->run(); });
		}

		~LoopHarness()
		{
			ctx->stop();
			if (thread.joinable())
			{
				thread.join();
			}
		}
	};

}  // namespace

TEST_CASE("UDP datagram round trip", "[datagram]")
{
	const IoBackend backend = GENERATE(from_range(coroute::testing::io_backend_arms()));
	INFO("backend " << io_backend_name(backend));

	LoopHarness harness{coroute::testing::context_or_skip(1, backend)};

	auto receiver = DatagramSocket::create(*harness.ctx);
	if (!receiver)
	{
		SUCCEED("DatagramSocket is not implemented on this backend yet");
		return;
	}

	auto sender = DatagramSocket::create(*harness.ctx);
	REQUIRE(sender);

	harness.start();

	// Port 0 asks the kernel for an unused port, so the test cannot collide with
	// anything else on the machine.
	REQUIRE(receiver->bind(0).has_value());
	REQUIRE(sender->bind(0).has_value());
	REQUIRE(receiver->local_port() != 0);
	REQUIRE(sender->local_port() != 0);
	REQUIRE(receiver->local_port() != sender->local_port());

	SECTION("a datagram arrives with its payload, peer and local address")
	{
		const std::string payload = "the quick brown fox";
		auto sent = sender->async_send(bytes_of(payload), loopback(receiver->local_port()), Endpoint{}).sync_wait();
		REQUIRE(sent.has_value());
		REQUIRE(*sent == payload.size());

		auto batch = receiver->async_recv_batch().sync_wait();
		require_ok(batch, "recv");
		REQUIRE(batch->size() == 1);

		const Datagram& dg = (*batch)[0];
		REQUIRE(to_string(dg.data) == payload);

		// The peer must identify the sender, otherwise a reply cannot be addressed.
		REQUIRE(dg.peer.len > 0);
		sockaddr_in peer{};
		std::memcpy(&peer, dg.peer.bytes.data(), sizeof(peer));
		REQUIRE(ntohs(peer.sin_port) == sender->local_port());

		// IP_PKTINFO. Without this a wildcard-bound server cannot reply from the
		// address the client actually sent to, and QUIC clients drop such replies.
		REQUIRE(dg.local.len > 0);
		sockaddr_in local{};
		std::memcpy(&local, dg.local.bytes.data(), sizeof(local));
		REQUIRE(local.sin_addr.s_addr == htonl(INADDR_LOOPBACK));
	}

	SECTION("a reply can be addressed from the received peer and local addresses")
	{
		auto sent = sender->async_send(bytes_of("ping"), loopback(receiver->local_port()), Endpoint{}).sync_wait();
		require_ok(sent, "send");

		auto batch = receiver->async_recv_batch().sync_wait();
		require_ok(batch, "recv");
		REQUIRE(batch->size() == 1);

		// This is the shape a QUIC server uses: never construct an address, just
		// mirror the one the datagram arrived with.
		const Endpoint peer = (*batch)[0].peer;
		const Endpoint local = (*batch)[0].local;

		auto replied = receiver->async_send(bytes_of("pong"), peer, local).sync_wait();
		require_ok(replied, "reply");

		auto back = sender->async_recv_batch().sync_wait();
		require_ok(back, "recv back");
		REQUIRE(back->size() == 1);
		REQUIRE(to_string((*back)[0].data) == "pong");
	}

	SECTION("several datagrams are returned as separate entries")
	{
		// The interface contract is one entry per datagram. QUIC must process each as
		// its own packet, so a backend that coalesced them would be wrong even if the
		// bytes were all present.
		const std::vector<std::string> payloads{"alpha", "bravo", "charlie", "delta"};
		for (const auto& text : payloads)
		{
			auto one = sender->async_send(bytes_of(text), loopback(receiver->local_port()), Endpoint{}).sync_wait();
			require_ok(one, "send " + text);
		}

		std::vector<std::string> received;
		while (received.size() < payloads.size())
		{
			auto batch = receiver->async_recv_batch().sync_wait();
			REQUIRE(batch.has_value());
			REQUIRE(!batch->empty());
			for (const Datagram& dg : *batch)
			{
				received.push_back(to_string(dg.data));
			}
		}

		REQUIRE(received.size() == payloads.size());
		for (size_t i = 0; i < payloads.size(); ++i)
		{
			REQUIRE(received[i] == payloads[i]);
		}
	}

	SECTION("a segmented send is delivered as separate datagrams")
	{
		// gso_size asks for segmentation. Where the kernel offloads it this is one
		// syscall; where it does not the backend loops. Either way the receiver must
		// see distinct datagrams of the requested size, which is what makes the
		// emulated path a real substitute rather than an approximation.
		constexpr size_t segment = 8;
		const std::string payload = "AAAAAAAABBBBBBBBCCCCCCCC";  // three segments of 8
		REQUIRE(payload.size() % segment == 0);

		auto sent =
			sender->async_send(bytes_of(payload), loopback(receiver->local_port()), Endpoint{}, segment).sync_wait();
		REQUIRE(sent.has_value());

		std::vector<std::string> pieces;
		while (pieces.size() < payload.size() / segment)
		{
			auto batch = receiver->async_recv_batch().sync_wait();
			REQUIRE(batch.has_value());
			for (const Datagram& dg : *batch)
			{
				pieces.push_back(to_string(dg.data));
			}
		}

		REQUIRE(pieces.size() == payload.size() / segment);
		std::string rejoined;
		for (const auto& piece : pieces)
		{
			REQUIRE(piece.size() == segment);
			rejoined += piece;
		}
		REQUIRE(rejoined == payload);
	}
}
