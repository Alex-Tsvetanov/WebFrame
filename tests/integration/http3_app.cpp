// One App serving HTTP/1.1, HTTP/2 and HTTP/3 from a single configuration.
//
// This is the claim the whole project rests on, reduced to something runnable: one
// port number, one certificate, one set of routes, one thread pool. If serving three
// protocols needed three of anything visible here, the design would not have worked.
#include <cstdio>
#include <cstdlib>

#include "coroute/core/app.hpp"

using namespace coroute;

int main(int argc, char** argv)
{
	if (argc < 4)
	{
		std::fprintf(stderr, "usage: http3_app <port> <key.pem> <cert.pem>\n");
		return 2;
	}

	App app;

	app.get("/",
	        [](Request&) -> Task<Response>
	        {
				// Short on purpose: the checking script greps this out of a hex dump,
				// whose ASCII column is 16 bytes wide.
				co_return Response::ok("coroute h3 ok\n");
			});

	AppTlsConfig tls;
	tls.key_file = argv[2];
	tls.cert_file = argv[3];

	app.enable_tls(tls);
	app.enable_http3();
	app.run(static_cast<uint16_t>(std::atoi(argv[1])));
	return 0;
}
