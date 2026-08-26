#include <catch2/catch_test_macros.hpp>
#include <iostream>
#include <thread>
#include "coroute/net/io_context.hpp"

using namespace coroute;
using namespace coroute::net;

TEST_CASE("UDP Socket - Basic Initialization and Binding", "[udp]")
{
	auto ctx = IoContext::create(1);
	auto bind_res = ctx->bind_udp(0);

	REQUIRE(bind_res);
	auto sock = std::move(bind_res.value());
	REQUIRE(sock != nullptr);
	REQUIRE(sock->is_open());
	REQUIRE(sock->local_port() > 0);

	sock->close();
	REQUIRE_FALSE(sock->is_open());
}

struct UdpTestResult {
    bool send_ok = false;
    size_t bytes_sent = 0;
    bool recv_ok = false;
    size_t bytes_recvd = 0;
    std::string recvd_msg;
    std::string peer_addr;
    uint16_t peer_port = 0;
};

TEST_CASE("UDP Socket - Send and Receive datagrams", "[udp]")
{
	auto ctx = IoContext::create(1);

	auto sock_server_res = ctx->bind_udp(0);
	REQUIRE(sock_server_res);
	auto sock_server = std::move(sock_server_res.value());

	auto sock_client_res = ctx->bind_udp(0);
	REQUIRE(sock_client_res);
	auto sock_client = std::move(sock_client_res.value());

	auto server_port = sock_server->local_port();
	UdpEndpoint server_ep{"127.0.0.1", server_port};

	auto make_task = [&](const std::unique_ptr<UdpSocket>& client, const std::unique_ptr<UdpSocket>& server, UdpEndpoint ep, UdpTestResult& res, IoContext* context) -> Task<void>
	{
		std::cout << "Starting UDP task..." << std::endl;
		const char* msg = "Hello UDP";
		std::cout << "Sending to " << ep.address << ":" << ep.port << std::endl;

		auto send_res = co_await client->async_send_to(msg, 9, ep);
		std::cout << "Send completed. Success=" << send_res.has_value() << std::endl;

		res.send_ok = send_res.has_value();
		if (send_res) res.bytes_sent = *send_res;

		char buf[32] = {0};
		std::cout << "Receiving..." << std::endl;
		auto recv_res = co_await server->async_recv_from(buf, sizeof(buf));
		std::cout << "Receive completed. Success=" << recv_res.has_value() << std::endl;

		res.recv_ok = recv_res.has_value();
		if (recv_res)
		{
			auto [bytes_recvd, peer] = *recv_res;
			res.bytes_recvd = bytes_recvd;
			res.recvd_msg = std::string(buf, bytes_recvd);
			res.peer_addr = peer.address;
			res.peer_port = peer.port;
		}

		std::cout << "Stopping context..." << std::endl;
		context->stop();
		co_return;
	};

	UdpTestResult res{};
	auto task = make_task(sock_client, sock_server, server_ep, res, ctx.get());
	ctx->post([&]() {
		task.start_detached();
	});

	ctx->run();

	REQUIRE(res.send_ok);
	REQUIRE(res.bytes_sent == 9);
	REQUIRE(res.recv_ok);
	REQUIRE(res.bytes_recvd == 9);
	REQUIRE(res.recvd_msg == "Hello UDP");
	REQUIRE(res.peer_addr == "127.0.0.1");
	REQUIRE(res.peer_port == sock_client->local_port());
}
