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

#include <charconv>
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
				  << "  --no-detect       serve HTTP/1.1 only, skipping protocol classification\n"
				  << "  --handshake-ms N  limit on the classification window (default: 30000, 0 disables)\n"
				  << "  --keep-alive-ms N idle limit on an established connection (default: 30000, 0 disables)\n"
				  << "  --tls CERT KEY    serve TLS on the same port\n"
				  << "  --http3           serve HTTP/3 as well (requires --tls)\n"
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
	bool detect = true;
	bool http3 = false;
	std::string cert_file;
	std::string key_file;

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
	if (!detect && !cert_file.empty())
	{
		std::cerr << "--no-detect serves cleartext HTTP/1.1 only and cannot be combined with --tls\n";
		return 2;
	}

	// Built once at startup, not per request: the point of this server is to measure
	// the framework, not std::string construction.
	const std::string body = (payload == 0) ? std::string("Hello, World!") : std::string(payload, 'x');

	App app;
	app.threads(workers);
	app.backlog(static_cast<int>(backlog));
	app.enable_protocol_detection(detect);
	app.handshake_timeout(std::chrono::milliseconds(handshake_ms));
	app.keep_alive_timeout(std::chrono::milliseconds(keep_alive_ms));
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
			  << ", tls " << (cert_file.empty() ? "off" : "on") << ", http3 " << (http3 ? "on" : "off")
			  << ")" << std::endl;

	app.run(static_cast<uint16_t>(port));

	return 0;
}
