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
		std::cout << "usage: " << argv0 << " [--workers N] [--port N] [--payload BYTES]\n"
				  << "  --workers N       worker threads (default: hardware concurrency)\n"
				  << "  --port N          listen port (default: 8080)\n"
				  << "  --payload BYTES   response body size (default: 13, \"Hello, World!\")\n"
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

	// Built once at startup, not per request: the point of this server is to measure
	// the framework, not std::string construction.
	const std::string body = (payload == 0) ? std::string("Hello, World!") : std::string(payload, 'x');

	App app;
	app.threads(workers);
	app.get("/", [&body](Request&) -> Task<Response> { co_return Response::ok(body); });

	std::cout << "Benchmark server on port " << port << " (" << workers << " workers, " << body.size()
			  << " byte body)" << std::endl;

	app.run(static_cast<uint16_t>(port));

	return 0;
}
