// One App serving HTTP/1.1, HTTP/2 and HTTP/3 from a single configuration.
//
// This is the claim the whole project rests on, reduced to something runnable: one
// port number, one certificate, one set of routes, one thread pool. If serving three
// protocols needed three of anything visible here, the design would not have worked.
#include <csignal>
#include <cstddef>
#include <memory>
#include <cstdio>
#include <cstdlib>
#include <string>

#include "coroute/core/app.hpp"
#ifdef COROUTE_HAS_HTTP3
// app.hpp only forward declares Http3Stats, deliberately, so that ngtcp2 and
// nghttp3 stay out of every translation unit that includes it. Reading the
// counters needs the definition.
#include "coroute/http3/endpoint.hpp"
#endif

using namespace coroute;

// The harness runs this inside a network namespace it later deletes, and it used to be
// left running when the namespace went: the test's trap killed the `ip netns exec`
// wrapper, which is not this process, and nothing here answered SIGTERM. Seven of these
// were found alive after one afternoon of runs, each holding four worker threads. A
// benchmark harness that leaves servers running is one that will eventually measure them.
//
// The handler does the least that is safe from a signal context: set a flag the run loop
// can see, through the App the signal cannot be given as an argument.
namespace
{
	App* g_app = nullptr;

	extern "C" void stop_on_signal(int)
	{
		if (g_app != nullptr)
		{
			g_app->stop();
		}
	}
}

int main(int argc, char** argv)
{
	if (argc < 4)
	{
		std::fprintf(stderr, "usage: http3_app <port> <key.pem> <cert.pem> [threads]\n");
		return 2;
	}
	const auto threads = argc > 4 ? static_cast<size_t>(std::atoi(argv[4])) : 1;

	App app;
	app.threads(threads);

	app.get("/",
	        [](Request&) -> Task<Response>
	        {
				// Under 16 characters on purpose: the reference client hex-dumps the body,
				// and the ASCII column of a hex dump is 16 bytes wide, so a longer string
				// is split across two lines and cannot be grepped in one piece.
				co_return Response::ok("coroute h3 ok\n");
			});

	// A body of a size the caller chooses at startup, for measuring the forwarded share.
	//
	// The share is forwarded_in/received, and `received` counts datagrams, not requests.
	// Two hundred requests for a fourteen-byte body is twelve datagrams, because tiny
	// responses coalesce: the counter is then quantised far more coarsely than the effect
	// it is meant to resolve, and a connection is "long" in requests while being nothing
	// on the wire. Request count is the wrong lever for connection length; bytes are.
	//
	// Stream credit is the other reason. The initial MAX_STREAMS here is 100, so a client
	// asked for more simply stops at the limit and, if it exits when its open streams
	// close, never sends the rest -- silently, with every request it did send succeeding.
	//
	// Size comes from the environment rather than the path so the response is identical
	// for every request in a run, which keeps the packet count a property of the
	// measurement rather than of which URL happened to be fetched.
	{
		const char* env = std::getenv("COROUTE_BULK_BYTES");
		const auto bytes = env != nullptr ? static_cast<std::size_t>(std::atol(env)) : 65536;
		auto body = std::make_shared<std::string>(bytes, 'x');
		app.get("/bulk",
		        [body](Request&) -> Task<Response>
		        {
					co_return Response::ok(*body);
				});
	}

#ifdef COROUTE_HAS_HTTP3
	// The server reporting on its own packet steering. forwarded_in over received is
	// the number that decides whether kernel-side connection-ID steering, which is what
	// nginx does with eBPF, would be worth building: if almost nothing ever needs
	// forwarding, there is almost nothing for it to save.
	app.get("/quic-stats",
	        [&app](Request&) -> Task<Response>
	        {
				const auto stats = app.http3_stats();
				std::string body;
				body += "received " + std::to_string(stats.received) + "\n";
				body += "forwarded_out " + std::to_string(stats.forwarded_out) + "\n";
				body += "forwarded_in " + std::to_string(stats.forwarded_in) + "\n";
				body += "accepted " + std::to_string(stats.accepted) + "\n";
				body += "version_negotiations " + std::to_string(stats.version_negotiations) + "\n";
				body += "stateless_resets " + std::to_string(stats.stateless_resets) + "\n";
				body += "dropped " + std::to_string(stats.dropped) + "\n";
				co_return Response::ok(body);
			});
#endif

	AppTlsConfig tls;
	tls.key_file = argv[2];
	tls.cert_file = argv[3];

	app.enable_tls(tls);
	app.enable_http3();

	g_app = &app;
	std::signal(SIGTERM, stop_on_signal);
	std::signal(SIGINT, stop_on_signal);

	app.run(static_cast<uint16_t>(std::atoi(argv[1])));
	return 0;
}
