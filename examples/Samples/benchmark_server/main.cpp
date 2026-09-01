/**
 * Coroute v2 - Benchmark Server
 *
 * Minimal server for benchmarking - no compression, simple response.
 *
 * Every knob here is a runtime flag rather than a compile-time option on purpose.
 * Build-to-build variation from code layout and link order is documented at 5-10%,
 * well above the 1-2% run-to-run noise on a controlled machine, so two separately
 * compiled binaries cannot be compared at the resolution this project needs. Both
 * arms of any A/B measurement must be the same binary.
 */

#include <coroute/coroute.hpp>

#include "route_table.hpp"

#include <cerrno>
#include <charconv>
#include <cstring>
#include <fstream>
#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#elif defined(__linux__)
#include <sched.h>
#endif

#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>
#include <thread>

using namespace coroute;

namespace
{

	void print_usage(const char* argv0)
	{
		std::cout << "usage: " << argv0 << " [options]\n"
				  << "  --workers N       worker threads (default: hardware concurrency)\n"
				  << "  --port N          listen port (default: 8080)\n"
				  << "  --payload BYTES   response body size (default: 13, \"Hello, World!\")\n"
				  << "  --backlog N       listen backlog (default: 1024)\n"
				  << "  --no-detect       skip classification and serve one dedicated listener:\n"
				  << "                    cleartext HTTP/1.1, or TLS when --tls is also given\n"
				  << "  --handshake-ms N  limit on the classification window (default: 30000, 0 disables)\n"
				  << "  --keep-alive-ms N idle limit on an established connection (default: 30000, 0 disables)\n"
				  << "  --max-requests N  requests per connection before closing (default: 100, 0 unlimited)\n"
				  << "  --affinity HEX    pin the server to a CPU mask, e.g. ff\n"
				  << "  --tls CERT KEY    serve TLS on the same port\n"
				  << "  --http3           serve HTTP/3 as well (requires --tls)\n"
				  << "  --io-backend B    io_uring or epoll; both are in this binary under\n"
				  << "                    -DCOROUTE_IO_BACKEND=dual and the choice is made\n"
				  << "                    here, at runtime (default: whatever the host allows)\n"
				  << "\n"
				  << "Routing experiment:\n"
				  << "  --router ARM      dfa (default), radix or regex; all three are in\n"
				  << "                    this binary and the choice is made here, at runtime\n"
				  << "  --routes N        register a generated table of N routes (default 0,\n"
				  << "                    meaning just \"/\")\n"
				  << "  --route-shape S   rest (shared /api/v1 prefix) or flat (default rest)\n"
				  << "  --route-params 0|1  routes end in a {id} capture (default 1)\n"
				  << "  --route-depth N   path segments per route (default 5)\n"
				  << "  --dump-routes F   write the concrete request paths to F and exit, so the\n"
				  << "                    load generator asks for exactly the table that was built\n"
				  << "\n"
				  << "The flags exist so that both sides of a comparison come from one binary.\n"
				  << "--no-detect is the arm the demultiplexer is measured against, and --backlog\n"
				  << "is a swept variable rather than a constant: at several thousand concurrent\n"
				  << "connections the queue depth is large enough to matter.\n"
				  << "\n"
				  << "Legacy positional form <workers> <port> is still accepted.\n";
	}

	// Returns false on a malformed value so the caller can fail loudly rather than
	// silently benchmarking a default it did not ask for.
	bool parse_size(std::string_view text, size_t& out)
	{
		const char* begin = text.data();
		const char* end = begin + text.size();
		auto [ptr, ec] = std::from_chars(begin, end, out);
		return ec == std::errc{} && ptr == end;
	}

}  // namespace

int main(int argc, char** argv)
{
	size_t workers = std::thread::hardware_concurrency();
	if (workers == 0) workers = 1;

	size_t port = 8080;
	size_t payload = 0;  // 0 means the default greeting
	size_t backlog = 1024;
	size_t handshake_ms = 30000;
	size_t keep_alive_ms = 30000;
	size_t max_requests = 100;
	std::string affinity_hex;
	bool detect = true;
	bool http3 = false;
	std::string cert_file;
	std::string key_file;

	// Empty means "ask the host", which is IoBackend::Default.
	std::string io_backend_arm;

	std::string router_arm = "dfa";
	size_t route_count = 0;
	std::string route_shape = "rest";
	size_t route_params = 1;
	size_t route_depth = 5;
	std::string dump_routes;

	// Legacy positional form: <workers> [port], kept so existing scripts still work.
	int positional = 0;

	for (int i = 1; i < argc; ++i)
	{
		std::string_view arg = argv[i];

		auto value_for = [&](const char* name) -> std::string_view
		{
			if (i + 1 >= argc)
			{
				std::cerr << name << " requires a value\n";
				std::exit(2);
			}
			return argv[++i];
		};

		if (arg == "--help" || arg == "-h")
		{
			print_usage(argv[0]);
			return 0;
		}
		else if (arg == "--workers")
		{
			if (!parse_size(value_for("--workers"), workers) || workers == 0)
			{
				std::cerr << "invalid --workers\n";
				return 2;
			}
		}
		else if (arg == "--port")
		{
			if (!parse_size(value_for("--port"), port) || port == 0 || port > 65535)
			{
				std::cerr << "invalid --port\n";
				return 2;
			}
		}
		else if (arg == "--payload")
		{
			if (!parse_size(value_for("--payload"), payload))
			{
				std::cerr << "invalid --payload\n";
				return 2;
			}
		}
		else if (arg == "--backlog")
		{
			if (!parse_size(value_for("--backlog"), backlog) || backlog == 0)
			{
				std::cerr << "invalid --backlog\n";
				return 2;
			}
		}
		else if (arg == "--handshake-ms")
		{
			if (!parse_size(value_for("--handshake-ms"), handshake_ms))
			{
				std::cerr << "invalid --handshake-ms\n";
				return 2;
			}
		}
		else if (arg == "--keep-alive-ms")
		{
			if (!parse_size(value_for("--keep-alive-ms"), keep_alive_ms))
			{
				std::cerr << "invalid --keep-alive-ms\n";
				return 2;
			}
		}
		else if (arg == "--max-requests")
		{
			if (!parse_size(value_for("--max-requests"), max_requests))
			{
				std::cerr << "invalid --max-requests\n";
				return 2;
			}
		}
		else if (arg == "--affinity")
		{
			affinity_hex = value_for("--affinity");
		}
		else if (arg == "--no-detect")
		{
			detect = false;
		}
		else if (arg == "--tls")
		{
			cert_file = std::string(value_for("--tls"));
			if (i + 1 >= argc)
			{
				std::cerr << "--tls requires CERT and KEY\n";
				return 2;
			}
			key_file = argv[++i];
		}
		else if (arg == "--http3")
		{
			http3 = true;
		}
		else if (arg == "--io-backend")
		{
			io_backend_arm = std::string(value_for("--io-backend"));
		}
		else if (arg == "--router")
		{
			router_arm = std::string(value_for("--router"));
		}
		else if (arg == "--routes")
		{
			if (!parse_size(value_for("--routes"), route_count))
			{
				std::cerr << "invalid --routes\n";
				return 2;
			}
		}
		else if (arg == "--route-shape")
		{
			route_shape = std::string(value_for("--route-shape"));
		}
		else if (arg == "--route-params")
		{
			if (!parse_size(value_for("--route-params"), route_params) || route_params > 1)
			{
				std::cerr << "invalid --route-params (0 or 1)\n";
				return 2;
			}
		}
		else if (arg == "--route-depth")
		{
			if (!parse_size(value_for("--route-depth"), route_depth) || route_depth == 0)
			{
				std::cerr << "invalid --route-depth\n";
				return 2;
			}
		}
		else if (arg == "--dump-routes")
		{
			dump_routes = std::string(value_for("--dump-routes"));
		}
		else if (!arg.empty() && arg.front() == '-')
		{
			std::cerr << "unknown option: " << arg << "\n";
			print_usage(argv[0]);
			return 2;
		}
		else
		{
			size_t* target = (positional == 0) ? &workers : (positional == 1) ? &port : nullptr;
			if (!target || !parse_size(arg, *target))
			{
				std::cerr << "unexpected argument: " << arg << "\n";
				return 2;
			}
			++positional;
		}
	}

	// Refused rather than quietly ignored. A run configured for HTTP/3 that silently
	// served only TCP would produce a full set of plausible numbers for an experiment
	// that never happened, which is worse than no numbers at all.
	if (http3 && cert_file.empty())
	{
		std::cerr << "--http3 requires --tls CERT KEY (QUIC has no cleartext mode)\n";
		return 2;
	}
	// --no-detect with --tls used to be refused, on the reading that detection off meant
	// cleartext only. That reading left the TLS half of the experiment with no control:
	// the claim is that one descriptor serves TLS and cleartext at no cost against
	// dedicated listeners, and the dedicated TLS listener was the one arrangement the
	// binary could not produce. It can now, and it is what --no-detect --tls means:
	// straight into the handshake on accept, the way `listen 443 ssl` does it.

	// Built once at startup, not per request: the point of this server is to measure
	// the framework, not std::string construction.
	const std::string body = (payload == 0) ? std::string("Hello, World!") : std::string(payload, 'x');

	App app;
// Applied before the io context creates its workers, so no thread starts on a
	// core it will later be moved off. On one host the server and the generator
	// otherwise compete for the same cores, and a measurement in which they compete
	// is partly a measurement of the scheduler.
	if (!affinity_hex.empty())
	{
		const unsigned long long mask = std::strtoull(affinity_hex.c_str(), nullptr, 16);
		bool applied = false;
		#if defined(_WIN32)
		applied = SetProcessAffinityMask(GetCurrentProcess(), static_cast<DWORD_PTR>(mask)) != 0;
		#elif defined(__linux__)
		cpu_set_t set;
		CPU_ZERO(&set);
		for (int i = 0; i < 64; ++i)
		{
			if ((mask >> i) & 1ULL) CPU_SET(i, &set);
		}
		applied = sched_setaffinity(0, sizeof(set), &set) == 0;
		#endif
		if (!applied)
		{
			std::cerr << "warning: could not apply affinity mask " << affinity_hex << "\n";
		}
	}

	// ------------------------------------------------------------- I/O backend arm
	//
	// Selected here, at runtime, from this one binary, on the same reasoning as
	// --router below. Refused rather than defaulted when the arm does not exist: a run
	// that silently measured epoll while its record said io_uring would produce a full
	// set of plausible numbers for an experiment that never happened.
	//
	// Two distinct refusals, because they have different fixes. A backend that is not
	// in this binary is a build to reconfigure. A backend the kernel will not give this
	// process, which on a hardened host means io_uring and EPERM, is a machine to change
	// or a cell to drop.
	net::IoBackend io_backend = net::IoBackend::Default;
	if (!io_backend_arm.empty())
	{
		if (!net::parse_io_backend(io_backend_arm, io_backend))
		{
			std::cerr << "unknown --io-backend '" << io_backend_arm << "' (io_uring or epoll)\n";
			return 2;
		}
		if (!net::io_backend_compiled_in(io_backend))
		{
			std::cerr << "--io-backend " << io_backend_arm
			          << " needs a build configured with COROUTE_IO_BACKEND=" << io_backend_arm
			          << " or COROUTE_IO_BACKEND=dual\n";
			return 2;
		}
		const int probe = net::io_backend_probe(io_backend);
		if (probe != 0)
		{
			std::cerr << "--io-backend " << io_backend_arm << " is not available on this host: "
			          << std::strerror(probe);
			if (probe == EPERM)
			{
				std::cerr << " (EPERM; check kernel.io_uring_disabled and kernel.io_uring_group)";
			}
			std::cerr << "\n";
			return 2;
		}
	}

	app.io_backend(io_backend);
	app.threads(workers);
	app.backlog(static_cast<int>(backlog));
	app.enable_protocol_detection(detect);
	app.handshake_timeout(std::chrono::milliseconds(handshake_ms));
	app.keep_alive_timeout(std::chrono::milliseconds(keep_alive_ms));
	app.max_requests_per_connection(max_requests);

	// --------------------------------------------------------------- routing arm
	//
	// Selected here, at runtime, from this one binary. Refused rather than defaulted
	// when the arm does not exist, on the same reasoning as --http3 above: a run that
	// silently measured the DFA while its record said radix would produce a full set of
	// plausible numbers for an experiment that never happened.
	RouterBackend backend = RouterBackend::Dfa;
	if (!parse_router_backend(router_arm, backend))
	{
		std::cerr << "unknown --router '" << router_arm << "' (dfa, radix or regex)\n";
		return 2;
	}
	if (backend != RouterBackend::Dfa && !Router::arms_available())
	{
		std::cerr << "--router " << router_arm
		          << " needs a build configured with COROUTE_ROUTER_ARMS=ON\n";
		return 2;
	}

	routebench::TableSpec spec;
	spec.count = route_count;
	spec.params = route_params != 0;
	spec.depth = route_depth;
	if (!routebench::parse_shape(route_shape, spec.shape))
	{
		std::cerr << "unknown --route-shape '" << route_shape << "' (rest or flat)\n";
		return 2;
	}

	const std::vector<routebench::Route> table = routebench::generate(spec);

	// Written before the server starts so the load generator asks for exactly the
	// table that was registered, rather than for one regenerated from the same
	// parameters and trusted to match.
	if (!dump_routes.empty())
	{
		std::ofstream out(dump_routes, std::ios::binary);
		if (!out)
		{
			std::cerr << "could not write " << dump_routes << "\n";
			return 2;
		}
		for (const auto& r : table) out << r.path << "\n";
		return 0;
	}

	app.router().backend(backend);

	const auto route_build_t0 = std::chrono::steady_clock::now();
	for (const auto& r : table)
	{
		app.get(r.pattern, [&body](Request&) -> Task<Response> { co_return Response::ok(body); });
	}
	const auto route_build_t1 = std::chrono::steady_clock::now();
	const double route_build_ms =
		std::chrono::duration<double, std::milli>(route_build_t1 - route_build_t0).count();

	// "/" last so a table of zero routes still answers something, and so the readiness
	// probe has a path that exists whatever the table is.
	app.get("/", [&body](Request&) -> Task<Response> { co_return Response::ok(body); });

#ifdef COROUTE_HAS_TLS
	if (!cert_file.empty())
	{
		AppTlsConfig tls;
		tls.cert_file = cert_file;
		tls.key_file = key_file;
		app.enable_tls(tls);
	}
#else
	if (!cert_file.empty())
	{
		std::cerr << "--tls was requested but this build has TLS disabled\n";
		return 2;
	}
#endif

#ifdef COROUTE_HAS_HTTP3
	if (http3)
	{
		app.enable_http3();
	}
#else
	if (http3)
	{
		// The comment that used to sit here said this call throws on a build without
		// HTTP/3. It does not: enable_http3 is not declared at all in that build, so
		// this file did not compile anywhere HTTP/3 was off, which is every build on
		// this machine. Same shape as the TLS block above, and refusing here keeps
		// the rule that a configured protocol is either served or reported.
		std::cerr << "--http3 was requested but this build has HTTP/3 disabled\n";
		return 2;
	}
#endif

	// Printed so the run's configuration is recoverable from its own log rather than
	// from whatever the driver believes it launched.
	std::cout << "Benchmark server on port " << port << " (" << workers << " workers, " << body.size()
			  << " byte body, backlog " << backlog << ", detect " << (detect ? "on" : "off")
			  << ", handshake " << handshake_ms << "ms"
			  << ", keep-alive " << keep_alive_ms << "ms"
			  << ", max-requests " << max_requests
			  << ", affinity " << (affinity_hex.empty() ? std::string("none") : affinity_hex)
			  << ", tls " << (cert_file.empty() ? "off" : "on") << ", http3 " << (http3 ? "on" : "off")
			  << ", io-backend " << (io_backend_arm.empty() ? std::string("default") : io_backend_arm)
			  << ", router " << router_backend_name(backend) << ", routes " << table.size() << " "
			  << routebench::shape_name(spec.shape) << " depth " << spec.depth << " params "
			  << (spec.params ? "on" : "off") << ", route build " << route_build_ms << "ms"
			  << ")" << std::endl;

	app.run(static_cast<uint16_t>(port));

	return 0;
}
