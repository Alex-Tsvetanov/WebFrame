// The smallest server that exercises Http3Endpoint end to end.
//
// Driven by verify_http3.sh, which points ngtcp2's reference client at it. Kept
// deliberately small: everything here except the handler body is the minimum an
// application has to write to serve HTTP/3, so if this file grows, the API got
// harder to use.
#include <cstdio>
#include <cstdlib>
#include <string>

#include "coroute/http3/endpoint.hpp"
#include "coroute/net/io_context.hpp"
#include "coroute/net/tls.hpp"

using namespace coroute;

int main(int argc, char** argv)
{
	if (argc < 4)
	{
		std::fprintf(stderr, "usage: h3srv <port> <key.pem> <cert.pem>\n");
		return 2;
	}
	const auto port = static_cast<std::uint16_t>(std::atoi(argv[1]));

	net::TlsConfig config;
	config.key_file = argv[2];
	config.cert_file = argv[3];
	config.min_version = net::TlsConfig::MinVersion::TLS_1_3;

	auto tls = net::TlsContext::create_quic(config);
	if (!tls)
	{
		std::fprintf(stderr, "TLS context failed: %s\n", std::string(tls.error().message()).c_str());
		return 1;
	}

	auto io = net::IoContext::create(1);

	auto handler = [](Request& request) -> Task<Response>
	{
		std::fprintf(stderr, "[handler] path=%s\n", std::string(request.path()).c_str());
		// Under 16 characters on purpose: the reference client hex-dumps the body, and
		// the ASCII column of a hex dump is 16 bytes wide, so a longer string is split
		// across two lines and the checking script cannot grep for it in one piece.
		co_return Response::ok("coroute h3 ok\n");
	};

	http3::Http3Endpoint endpoint(*io, std::move(*tls), handler);
	if (auto bound = endpoint.bind(port); !bound)
	{
		std::fprintf(stderr, "bind failed: %s\n", std::string(bound.error().message()).c_str());
		return 1;
	}
	std::fprintf(stderr, "listening on UDP %u\n", endpoint.local_port());

	endpoint.run().start_detached();
	io->run();
	return 0;
}
