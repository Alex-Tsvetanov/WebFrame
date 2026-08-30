// Level one of the routing experiment: what one dispatch costs, with nothing else in
// the way.
//
// What is measured is coroute::Router::match, the same call the server makes on every
// request, with the arm chosen at runtime by --arm. Not a re-implementation of the
// three structures side by side: a re-implementation would measure code that no server
// runs, and the two levels would stop being about the same thing.
//
// Timing is rdtsc rather than a chrono clock. On this platform steady_clock is backed
// by QueryPerformanceCounter at 100 ns resolution, and a radix lookup is expected to
// land near 100 ns, so the clock would quantise the very arm it is there to resolve.
// The tick-to-nanosecond factor is calibrated against steady_clock at startup and
// written into the record, so a reader can redo the conversion or reject it.
//
// The output is a per-lookup histogram at one-cycle resolution, not a mean. Every
// percentile in the paper is computed from that file, and the file is small enough to
// commit, so the distribution behind each number stays inspectable.

#include <coroute/core/router.hpp>
#include <coroute/core/response.hpp>

#include "route_table.hpp"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <map>
#include <numeric>
#include <random>
#include <string>
#include <string_view>
#include <vector>

#include <immintrin.h>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <psapi.h>
#else
#include <sys/resource.h>
#include <unistd.h>
#endif

namespace
{

	// ---------------------------------------------------------------- machinery

	inline std::uint64_t tsc_now()
	{
		// lfence before the read so earlier work has retired. rdtsc is not a
		// serialising instruction and without this the counter can be sampled out of
		// order with the code being timed.
		_mm_lfence();
		const std::uint64_t t = __rdtsc();
		_mm_lfence();
		return t;
	}

	double calibrate_ns_per_tick(double seconds)
	{
		const auto wall0 = std::chrono::steady_clock::now();
		const std::uint64_t t0 = tsc_now();
		for (;;)
		{
			const auto elapsed = std::chrono::steady_clock::now() - wall0;
			if (std::chrono::duration<double>(elapsed).count() >= seconds) break;
		}
		const std::uint64_t t1 = tsc_now();
		const auto wall1 = std::chrono::steady_clock::now();
		const double ns = std::chrono::duration<double, std::nano>(wall1 - wall0).count();
		return ns / static_cast<double>(t1 - t0);
	}

	// Working set now. Reported as a delta across construction, which is what the
	// question "how much does the structure cost to hold" actually asks.
	std::uint64_t resident_bytes()
	{
#if defined(_WIN32)
		PROCESS_MEMORY_COUNTERS c{};
		c.cb = sizeof(c);
		if (GetProcessMemoryInfo(GetCurrentProcess(), &c, c.cb)) return c.WorkingSetSize;
		return 0;
#else
		long pages = 0;
		if (FILE* f = std::fopen("/proc/self/statm", "r"))
		{
			long total = 0;
			if (std::fscanf(f, "%ld %ld", &total, &pages) != 2) pages = 0;
			std::fclose(f);
		}
		return static_cast<std::uint64_t>(pages) * static_cast<std::uint64_t>(sysconf(_SC_PAGESIZE));
#endif
	}

	std::uint64_t peak_resident_bytes()
	{
#if defined(_WIN32)
		PROCESS_MEMORY_COUNTERS c{};
		c.cb = sizeof(c);
		if (GetProcessMemoryInfo(GetCurrentProcess(), &c, c.cb)) return c.PeakWorkingSetSize;
		return 0;
#else
		rusage ru{};
		getrusage(RUSAGE_SELF, &ru);
		return static_cast<std::uint64_t>(ru.ru_maxrss) * 1024ull;
#endif
	}

	// A histogram rather than a sample list. Percentiles come out exact, and 200k
	// lookups collapse to a few thousand lines instead of 200k, which is the difference
	// between a raw file that can be committed and one that cannot.
	struct Histogram
	{
		std::map<std::uint32_t, std::uint64_t> counts;
		std::uint64_t total = 0;

		void add(std::uint64_t cycles)
		{
			const std::uint32_t clamped =
				cycles > 0xFFFFFFFFull ? 0xFFFFFFFFu : static_cast<std::uint32_t>(cycles);
			++counts[clamped];
			++total;
		}

		std::uint32_t percentile(double p) const
		{
			if (total == 0) return 0;
			// Nearest-rank. The rank is the smallest value at or below which at least
			// p of the observations fall, which is the definition that does not
			// interpolate between two cycle counts that were never observed.
			const std::uint64_t rank =
				static_cast<std::uint64_t>(std::ceil(p * static_cast<double>(total)));
			std::uint64_t seen = 0;
			for (const auto& [value, count] : counts)
			{
				seen += count;
				if (seen >= rank) return value;
			}
			return counts.rbegin()->first;
		}

		double mean() const
		{
			if (total == 0) return 0.0;
			double sum = 0.0;
			for (const auto& [value, count] : counts) sum += double(value) * double(count);
			return sum / double(total);
		}
	};

	// ------------------------------------------------------------------ options

	struct Options
	{
		std::string arm = "dfa";
		size_t routes = 100;
		std::string shape = "rest";
		bool params = true;
		size_t depth = 5;
		size_t lookups = 200000;
		size_t warmup = 20000;
		// The slow arms cost milliseconds a lookup, so a fixed count would put a single
		// cell at hours. The cap bounds the run and the record carries how many samples
		// were actually taken, which is what decides whether its p99.9 means anything.
		double max_seconds = 20.0;
		std::string out_path;
		std::string hist_path;
		std::uint64_t seed = 20260830;
		double calibrate_s = 0.2;
		bool verify = false;
	};

	// Before comparing three routers on speed, show they answer the same question.
	// Every route in the table is asked for by its own concrete path and has to come
	// back as itself, with the parameter it carried. An arm that quietly matched the
	// wrong route, or none, would otherwise show up as a fast arm.
	int verify_arms(const routebench::TableSpec& spec)
	{
		const std::vector<routebench::Route> table = routebench::generate(spec);
		int failures = 0;

		for (const char* name : {"dfa", "radix", "regex"})
		{
			coroute::RouterBackend backend{};
			coroute::parse_router_backend(name, backend);

			coroute::Router router;
			router.backend(backend);
			for (const auto& r : table)
			{
				router.add(coroute::HttpMethod::GET, r.pattern,
				           [](coroute::Request&) -> coroute::Task<coroute::Response>
				           { co_return coroute::Response::ok("x"); });
			}

			size_t wrong = 0, unmatched = 0, bad_params = 0;
			for (size_t i = 0; i < table.size(); ++i)
			{
				const auto m = router.match(coroute::HttpMethod::GET, table[i].path);
				if (!m)
				{
					++unmatched;
					continue;
				}
				if (m.handler != &router.routes()[i].handler) ++wrong;
				if (spec.params && (m.params.size() != 1 || m.params[0] != "42")) ++bad_params;
				if (!spec.params && !m.params.empty()) ++bad_params;
			}

			// A path that matches no route must miss on every arm too, or one arm is
			// answering requests the others refuse.
			size_t false_hits = 0;
			for (const char* absent : {"/", "/nope", "/api/v1/g0/nope/42", "/api/v1/g0/r0/42/extra"})
			{
				if (router.match(coroute::HttpMethod::GET, absent)) ++false_hits;
			}

			const bool ok = wrong == 0 && unmatched == 0 && bad_params == 0 && false_hits == 0;
			failures += ok ? 0 : 1;
			std::printf("verify %-5s routes=%zu  %s  wrong=%zu unmatched=%zu bad_params=%zu false_hits=%zu\n", name,
			            table.size(), ok ? "ok  " : "FAIL", wrong, unmatched, bad_params, false_hits);
		}
		return failures;
	}

	void usage()
	{
		std::printf(
			"usage: route_bench [options]\n"
			"  --arm dfa|radix|regex   dispatch structure, chosen at runtime\n"
			"  --routes N              route table size (default 100)\n"
			"  --shape rest|flat       shared /api/v1 prefix, or branching at the first\n"
			"                          character (default rest)\n"
			"  --params 0|1            routes end in a {id} capture (default 1)\n"
			"  --depth N               path segments per route (default 5)\n"
			"  --lookups N             timed lookups (default 200000)\n"
			"  --warmup N              untimed lookups first (default 20000)\n"
			"  --max-seconds S         stop timing after S seconds even if short of\n"
			"                          --lookups (default 20); the record says how many\n"
			"                          samples were taken\n"
			"  --out FILE              JSON summary\n"
			"  --hist FILE             per-lookup histogram, cycles,count\n"
			"  --seed N                query order seed\n"
			"  --verify                check all three arms resolve the table identically\n"
			"                          and exit; no timing\n"
			"\n"
			"All three arms come out of this one binary. The arm is never a build option:\n"
			"build-to-build variation is larger than the effect being measured.\n");
	}

	// A Windows path in a JSON string is full of backslashes, and a backslash there is
	// an escape. Without this the record does not parse and the run is lost.
	std::string json_escape(std::string_view text)
	{
		std::string out;
		out.reserve(text.size() + 8);
		for (const char c : text)
		{
			if (c == '\\' || c == '"') out += '\\';
			out += c;
		}
		return out;
	}

	bool parse_size(const char* text, size_t& out)
	{
		char* end = nullptr;
		const unsigned long long v = std::strtoull(text, &end, 10);
		if (end == text || *end != '\0') return false;
		out = static_cast<size_t>(v);
		return true;
	}

}  // namespace

int main(int argc, char** argv)
{
	Options opt;

	for (int i = 1; i < argc; ++i)
	{
		const std::string a = argv[i];
		auto next = [&](const char* name) -> const char*
		{
			if (i + 1 >= argc)
			{
				std::fprintf(stderr, "%s requires a value\n", name);
				std::exit(2);
			}
			return argv[++i];
		};

		if (a == "--help" || a == "-h")
		{
			usage();
			return 0;
		}
		else if (a == "--arm") opt.arm = next("--arm");
		else if (a == "--routes") { if (!parse_size(next("--routes"), opt.routes)) return 2; }
		else if (a == "--shape") opt.shape = next("--shape");
		else if (a == "--params") { size_t v = 1; if (!parse_size(next("--params"), v)) return 2; opt.params = v != 0; }
		else if (a == "--depth") { if (!parse_size(next("--depth"), opt.depth)) return 2; }
		else if (a == "--lookups") { if (!parse_size(next("--lookups"), opt.lookups)) return 2; }
		else if (a == "--warmup") { if (!parse_size(next("--warmup"), opt.warmup)) return 2; }
		else if (a == "--max-seconds") opt.max_seconds = std::strtod(next("--max-seconds"), nullptr);
		else if (a == "--out") opt.out_path = next("--out");
		else if (a == "--hist") opt.hist_path = next("--hist");
		else if (a == "--seed") { size_t v = 0; if (!parse_size(next("--seed"), v)) return 2; opt.seed = v; }
		else if (a == "--verify") opt.verify = true;
		else
		{
			std::fprintf(stderr, "unknown option: %s\n", a.c_str());
			usage();
			return 2;
		}
	}

	coroute::RouterBackend backend{};
	if (!coroute::parse_router_backend(opt.arm, backend))
	{
		std::fprintf(stderr, "unknown --arm '%s'\n", opt.arm.c_str());
		return 2;
	}
	if (backend != coroute::RouterBackend::Dfa && !coroute::Router::arms_available())
	{
		std::fprintf(stderr, "this binary has no alternative arms; configure with COROUTE_ROUTER_ARMS=ON\n");
		return 2;
	}

	routebench::TableSpec spec;
	spec.count = opt.routes;
	spec.params = opt.params;
	spec.depth = opt.depth;
	if (!routebench::parse_shape(opt.shape, spec.shape))
	{
		std::fprintf(stderr, "unknown --shape '%s'\n", opt.shape.c_str());
		return 2;
	}

	if (opt.verify)
	{
		return verify_arms(spec) == 0 ? 0 : 1;
	}

	const double ns_per_tick = calibrate_ns_per_tick(opt.calibrate_s);
	const std::vector<routebench::Route> table = routebench::generate(spec);

	// ---------------------------------------------------------------- build

	const std::uint64_t rss_before = resident_bytes();
	const auto build_t0 = std::chrono::steady_clock::now();

	coroute::Router router;
	router.backend(backend);
	for (const auto& r : table)
	{
		router.add(coroute::HttpMethod::GET, r.pattern,
		           [](coroute::Request&) -> coroute::Task<coroute::Response>
		           { co_return coroute::Response::ok("x"); });
	}

	// Inside the build window on purpose: compressing the tree is part of what it
	// costs to stand this router up, and leaving it out would understate the arm.
	router.finalise();

	const auto build_t1 = std::chrono::steady_clock::now();
	const std::uint64_t rss_after = resident_bytes();
	const double build_ms = std::chrono::duration<double, std::milli>(build_t1 - build_t0).count();

	// ---------------------------------------------------------------- queries

	// Drawn uniformly over the table. A stream that always asked for the first route
	// would flatter a sequential scan; one that always asked for the last would
	// caricature it. Uniform is the mean case and is the one a paper can defend.
	std::mt19937_64 rng(opt.seed);
	std::vector<std::string_view> queries;
	queries.reserve(4096);
	for (size_t k = 0; k < 4096; ++k) queries.push_back(table[rng() % table.size()].path);

	size_t sink = 0;
	size_t misses = 0;

	// Warmup gets a tenth of the budget. On the slow arms a fixed warmup count would
	// take longer than the measurement it is warming up for.
	const auto warm_deadline =
		std::chrono::steady_clock::now() + std::chrono::duration<double>(opt.max_seconds * 0.1);
	size_t warmed = 0;
	for (size_t k = 0; k < opt.warmup; ++k)
	{
		auto m = router.match(coroute::HttpMethod::GET, queries[k % queries.size()]);
		if (!m) ++misses;
		sink += m.params.size();
		++warmed;
		if ((k & 0x3F) == 0x3F && std::chrono::steady_clock::now() >= warm_deadline) break;
	}

	// The cost of the measurement itself, taken the same way and reported rather than
	// subtracted. A reader who disagrees with subtracting it can still use the numbers.
	Histogram overhead;
	for (size_t k = 0; k < 4096; ++k)
	{
		const std::uint64_t t0 = tsc_now();
		const std::uint64_t t1 = tsc_now();
		overhead.add(t1 - t0);
	}

	Histogram hist;
	const auto measure_start = std::chrono::steady_clock::now();
	const auto measure_deadline = measure_start + std::chrono::duration<double>(opt.max_seconds);
	bool truncated = false;
	for (size_t k = 0; k < opt.lookups; ++k)
	{
		const std::string_view q = queries[k % queries.size()];
		const std::uint64_t t0 = tsc_now();
		auto m = router.match(coroute::HttpMethod::GET, q);
		const std::uint64_t t1 = tsc_now();
		hist.add(t1 - t0);
		if (!m) ++misses;
		sink += m.params.size();
		// Checked every 64 lookups so the clock read is not itself a cost on the fast
		// arms, where a lookup is a hundred nanoseconds and a steady_clock read is
		// twenty.
		if ((k & 0x3F) == 0x3F && std::chrono::steady_clock::now() >= measure_deadline)
		{
			truncated = true;
			break;
		}
	}
	const double measured_s =
		std::chrono::duration<double>(std::chrono::steady_clock::now() - measure_start).count();

	const std::uint64_t peak = peak_resident_bytes();

	// ---------------------------------------------------------------- output

	auto ns = [&](std::uint32_t cycles) { return double(cycles) * ns_per_tick; };

	std::printf("arm=%-5s routes=%6zu shape=%-4s params=%d depth=%2zu  "
	            "p50=%10.1fns p99=%11.1fns p99.9=%11.1fns  build=%9.2fms rss=%8.1fMB n=%7llu%s misses=%zu\n",
	            opt.arm.c_str(), opt.routes, opt.shape.c_str(), opt.params ? 1 : 0, opt.depth,
	            ns(hist.percentile(0.50)), ns(hist.percentile(0.99)), ns(hist.percentile(0.999)), build_ms,
	            double(rss_after - rss_before) / 1048576.0, static_cast<unsigned long long>(hist.total),
	            truncated ? "*" : " ", misses);

	if (!opt.hist_path.empty())
	{
		if (FILE* f = std::fopen(opt.hist_path.c_str(), "wb"))
		{
			std::fprintf(f, "cycles,count\n");
			for (const auto& [value, count] : hist.counts)
			{
				std::fprintf(f, "%u,%llu\n", value, static_cast<unsigned long long>(count));
			}
			std::fclose(f);
		}
		else
		{
			std::fprintf(stderr, "could not write %s\n", opt.hist_path.c_str());
			return 1;
		}
	}

	if (!opt.out_path.empty())
	{
		FILE* f = std::fopen(opt.out_path.c_str(), "wb");
		if (!f)
		{
			std::fprintf(stderr, "could not write %s\n", opt.out_path.c_str());
			return 1;
		}
		std::fprintf(f, "{\n");
		std::fprintf(f, "  \"arm\": \"%s\",\n", opt.arm.c_str());
		std::fprintf(f, "  \"routes\": %zu,\n", opt.routes);
		std::fprintf(f, "  \"shape\": \"%s\",\n", opt.shape.c_str());
		std::fprintf(f, "  \"params\": %s,\n", opt.params ? "true" : "false");
		std::fprintf(f, "  \"depth\": %zu,\n", opt.depth);
		std::fprintf(f, "  \"lookups\": %llu,\n", static_cast<unsigned long long>(hist.total));
		std::fprintf(f, "  \"lookups_requested\": %zu,\n", opt.lookups);
		std::fprintf(f, "  \"warmup_lookups\": %zu,\n", warmed);
		std::fprintf(f, "  \"truncated_by_time\": %s,\n", truncated ? "true" : "false");
		std::fprintf(f, "  \"measured_seconds\": %.4f,\n", measured_s);
		std::fprintf(f, "  \"misses\": %zu,\n", misses);
		std::fprintf(f, "  \"seed\": %llu,\n", static_cast<unsigned long long>(opt.seed));
		std::fprintf(f, "  \"ns_per_tick\": %.9f,\n", ns_per_tick);
		std::fprintf(f, "  \"build_ms\": %.4f,\n", build_ms);
		std::fprintf(f, "  \"rss_delta_bytes\": %lld,\n",
		             static_cast<long long>(rss_after) - static_cast<long long>(rss_before));
		std::fprintf(f, "  \"peak_rss_bytes\": %llu,\n", static_cast<unsigned long long>(peak));
		std::fprintf(f, "  \"timer_overhead_cycles\": {\"p50\": %u, \"p99\": %u},\n",
		             overhead.percentile(0.50), overhead.percentile(0.99));
		std::fprintf(f, "  \"cycles\": {\"p50\": %u, \"p90\": %u, \"p99\": %u, \"p999\": %u, \"max\": %u, "
		                "\"mean\": %.2f},\n",
		             hist.percentile(0.50), hist.percentile(0.90), hist.percentile(0.99), hist.percentile(0.999),
		             hist.counts.empty() ? 0u : hist.counts.rbegin()->first, hist.mean());
		std::fprintf(f, "  \"ns\": {\"p50\": %.2f, \"p90\": %.2f, \"p99\": %.2f, \"p999\": %.2f, \"max\": %.2f, "
		                "\"mean\": %.2f},\n",
		             ns(hist.percentile(0.50)), ns(hist.percentile(0.90)), ns(hist.percentile(0.99)),
		             ns(hist.percentile(0.999)), ns(hist.counts.empty() ? 0u : hist.counts.rbegin()->first),
		             hist.mean() * ns_per_tick);
		std::fprintf(f, "  \"histogram\": \"%s\",\n", json_escape(opt.hist_path).c_str());
		std::fprintf(f, "  \"sink\": %zu\n", sink);
		std::fprintf(f, "}\n");
		std::fclose(f);
	}

	return 0;
}
