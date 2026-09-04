// The arrangement the demultiplexer replaces, built so it can be counted.
//
// App::run says it in a comment: TLS and cleartext used to be mutually exclusive
// branches, so serving both meant two App instances, two thread pools, two event loops
// and the routes registered twice. Every argument for classifying after accept rests on
// that being expensive, and nothing in the tree has ever measured it, because nothing in
// the tree could produce it. This binary produces it.
//
// It is not a server anyone should deploy and it is not a latency experiment. It exists
// so that a census can put one App beside two and count descriptors, threads, resident
// memory and the cost of holding one route table twice. Both Apps register the same
// generated table, which is the point: the duplication is the thing being measured, not
// an oversight.
//
// Pair it with benchmark_server on the same table and worker count for the other arm.

#include <coroute/coroute.hpp>

#include "route_table.hpp"

#include <atomic>
#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

using namespace coroute;

namespace
{
	bool parse_size(std::string_view text, size_t& out)
	{
		if (text.empty()) return false;
		size_t value = 0;
		for (const char c : text)
		{
			if (c < '0' || c > '9') return false;
			value = value * 10 + static_cast<size_t>(c - '0');
		}
		out = value;
		return true;
	}

	// Registered identically on both Apps. Two routers, two copies of every pattern,
	// which is the memory the census is there to see.
	void install(App& app, const std::vector<routebench::Route>& table, const std::string& body)
	{
		for (const auto& r : table)
		{
			app.get(r.path, [&body](Request&) -> Task<Response> { co_return Response::ok(body); });
		}
		// A path that exists whatever the table is, so a readiness probe has something
		// to ask for.
		app.get("/", [&body](Request&) -> Task<Response> { co_return Response::ok(body); });
	}
}  // namespace

int main(int argc, char** argv)
{
	size_t port = 8080;
	size_t tls_port = 8443;
	size_t workers = 1;
	size_t route_count = 0;
	size_t route_depth = 5;
	size_t route_params = 1;
	std::string route_shape = "rest";
	std::string cert_file;
	std::string key_file;

	for (int i = 1; i < argc; ++i)
	{
		const std::string_view arg = argv[i];
		auto value_for = [&](const char* name) -> std::string_view
		{
			if (i + 1 >= argc)
			{
				std::cerr << name << " requires a value\n";
				std::exit(2);
			}
			return argv[++i];
		};
		if (arg == "--help")
		{
			std::cout
				<< "two_app_server: the two-App arrangement, for the descriptor census\n\n"
				<< "  --port N          cleartext listen port (default: 8080)\n"
				<< "  --tls-port N      TLS listen port (default: 8443)\n"
				<< "  --workers N       workers per App; the process runs two of these\n"
				<< "  --routes N        generated routes, registered on BOTH Apps\n"
				<< "  --route-shape S   rest or flat (default rest)\n"
				<< "  --route-params 0|1  routes end in a {id} capture (default 1)\n"
				<< "  --route-depth N   path segments (default 5)\n"
				<< "  --tls CERT KEY    certificate and key for the TLS App\n";
			return 0;
		}
		else if (arg == "--port") { if (!parse_size(value_for("--port"), port)) return 2; }
		else if (arg == "--tls-port") { if (!parse_size(value_for("--tls-port"), tls_port)) return 2; }
		else if (arg == "--workers") { if (!parse_size(value_for("--workers"), workers)) return 2; }
		else if (arg == "--routes") { if (!parse_size(value_for("--routes"), route_count)) return 2; }
		else if (arg == "--route-depth") { if (!parse_size(value_for("--route-depth"), route_depth)) return 2; }
		else if (arg == "--route-params") { if (!parse_size(value_for("--route-params"), route_params)) return 2; }
		else if (arg == "--route-shape") { route_shape = std::string(value_for("--route-shape")); }
		else if (arg == "--tls")
		{
			cert_file = std::string(value_for("--tls"));
			key_file = std::string(value_for("--tls"));
		}
		else
		{
			std::cerr << "unknown option: " << arg << "\n";
			return 2;
		}
	}

	routebench::TableSpec spec;
	spec.count = route_count;
	spec.params = route_params != 0;
	spec.depth = route_depth;
	if (!routebench::parse_shape(route_shape, spec.shape))
	{
		std::cerr << "unknown --route-shape '" << route_shape << "'\n";
		return 2;
	}
	const std::vector<routebench::Route> table = routebench::generate(spec);
	const std::string body = "Hello, World!";

	App cleartext;
	cleartext.threads(workers);
	install(cleartext, table, body);

	App secure;
	secure.threads(workers);
	install(secure, table, body);

#ifdef COROUTE_HAS_TLS
	if (!cert_file.empty())
	{
		AppTlsConfig tls;
		tls.cert_file = cert_file;
		tls.key_file = key_file;
		secure.enable_tls(tls);
	}
#else
	if (!cert_file.empty())
	{
		std::cerr << "--tls was requested but this build has TLS disabled\n";
		return 2;
	}
#endif

	// run() blocks, so one App per thread. Neither is the main thread's, so that a
	// failure in either is visible rather than being the one that never returned.
	std::thread t1([&] { cleartext.run(static_cast<uint16_t>(port)); });
	std::thread t2([&] { secure.run(static_cast<uint16_t>(tls_port)); });

	// The census waits on this line. It is printed after both threads are started, not
	// after both are listening, so a reader should still poll the ports rather than
	// trust it; each App prints its own listening line.
	std::cout << "two_app_server: cleartext " << port << ", tls " << tls_port
	          << ", " << workers << " workers each, " << table.size()
	          << " routes registered twice" << '\n' << std::flush;

	t1.join();
	t2.join();
	return 0;
}
