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

// The cycle counter is x86 only. The per-lookup histogram needs it, because a radix
// lookup lands near 100 ns and steady_clock quantises at 100 ns on the platforms
// measured here, so a chrono-timed per-call sample would blur the arm it exists to
// resolve.
//
// The batch instrument has no such need. Timing a thousand lookups at once puts a batch
// near 100 microseconds, where a 100 ns quantum is a thousandth of the sample, so it
// runs anywhere and is how a non-x86 host contributes a dispatch number at all.
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
#define COROUTE_ROUTE_BENCH_HAS_TSC 1
#include <immintrin.h>
#else
#define COROUTE_ROUTE_BENCH_HAS_TSC 0
#endif

#if defined(__linux__)
#include <sched.h>
#endif

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <psapi.h>
#endif

namespace
{

	// ---------------------------------------------------------------- machinery

	bool apply_affinity(std::uint64_t mask)
	{
		if (mask == 0)
		{
			return true;
		}
#if defined(_WIN32)
		return ::SetProcessAffinityMask(::GetCurrentProcess(), static_cast<DWORD_PTR>(mask)) != 0;
#elif defined(__linux__)
		cpu_set_t set;
		CPU_ZERO(&set);
		for (int i = 0; i < 64; ++i)
		{
			if ((mask >> i) & 1ULL)
			{
				CPU_SET(i, &set);
			}
		}
		return ::sched_setaffinity(0, sizeof(set), &set) == 0;
#else
		// macOS has no portable process affinity API. Requested and applied are recorded
		// separately so a reader can tell a mask that was asked for from one that took.
		return false;
#endif
	}

#if COROUTE_ROUTE_BENCH_HAS_TSC
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
#else
	// Without a cycle counter there is no tick, so the conversion is the identity and
	// every duration below is already nanoseconds.
	double calibrate_ns_per_tick(double) { return 1.0; }
#endif

	// Two memory numbers, because at ten thousand routes they stop agreeing and only
	// one of them still answers the question.
	//
	// Resident is the working set: how much of the process is in physical memory right
	// now. It is what the machine is actually spending, and it is capped by how much
	// physical memory there is. A structure larger than free RAM reports a working set
	// the size of free RAM and looks smaller than a structure half its size.
	//
	// Committed is private commit: how much the process has asked the operating system
	// to back, paged out or not. It is not capped by physical memory, so it is the one
	// that keeps meaning "how big is this structure" all the way up the sweep.
	//
	// Both are recorded. Where they diverge, the process was paging, and that is worth
	// seeing rather than smoothing over.
	//
	// The two peak_ fields are high-water marks kept by the operating system, not
	// samples taken at the end. That distinction is the difference between a number
	// that answers "how big did this get" and one that answers "how big was it when we
	// stopped looking", and only the first survives a structure that grows and shrinks.
	// See memory_now() for what fills them per platform, and memory_source() for the
	// string that says so in the record.
	struct MemorySample
	{
		std::uint64_t resident = 0;
		std::uint64_t committed = 0;
		std::uint64_t peak_resident = 0;
		std::uint64_t peak_committed = 0;
	};

	// What each platform actually measured, carried into the record.
	//
	// The two peak numbers are not the same quantity on Windows and Linux and cannot be
	// made so: one reports pagefile commit charge and the other peak virtual size. A
	// reader comparing a Linux row against a Windows row has to know that, and the only
	// place they can learn it is the record itself.
	const char* memory_source() noexcept
	{
#if defined(_WIN32)
		return "windows:GetProcessMemoryInfo PeakPagefileUsage/PeakWorkingSetSize";
#else
		return "linux:/proc/self/status VmPeak/VmHWM";
#endif
	}

	MemorySample memory_now()
	{
		MemorySample s;
#if defined(_WIN32)
		PROCESS_MEMORY_COUNTERS_EX c{};
		c.cb = sizeof(c);
		if (GetProcessMemoryInfo(GetCurrentProcess(), reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&c), c.cb))
		{
			s.resident = c.WorkingSetSize;
			s.peak_resident = c.PeakWorkingSetSize;
			s.committed = c.PrivateUsage;
			s.peak_committed = c.PeakPagefileUsage;
		}
#else
		// /proc/self/status, not /proc/self/statm, and the difference is the whole
		// point: statm has no peak in it. The peak commit used to be filled with
		// statm's VmSize read at the end of the run, which is a final value wearing a
		// peak's name, under the same key Windows fills with PeakPagefileUsage. A
		// structure that grew and shrank reported the size it ended at.
		//
		// That is the same class of error the memory finding exists to report: a
		// 10 000-route table that paged and reported one of the smallest footprints.
		// Repeating it in the instrument that measures it would be worse than not
		// measuring at all, because the number would still look plausible.
		//
		// VmPeak is peak virtual size and VmHWM is peak resident, both maintained by
		// the kernel as high-water marks, so neither can be walked back by a free.
		// VmData replaces statm's total for the running commit figure: it is the
		// private data segment rather than the whole address space, which is the
		// nearest thing Linux has to Windows' PrivateUsage and does not move when a
		// shared library is mapped.
		if (FILE* f = std::fopen("/proc/self/status", "r"))
		{
			char line[256];
			auto field = [&line](const char* name, std::uint64_t& out)
			{
				const std::size_t n = std::strlen(name);
				if (std::strncmp(line, name, n) != 0) return;
				unsigned long long kb = 0;
				// Every Vm* line in this file is reported in kB.
				if (std::sscanf(line + n, " %llu", &kb) == 1)
				{
					out = static_cast<std::uint64_t>(kb) * 1024ull;
				}
			};
			while (std::fgets(line, sizeof(line), f) != nullptr)
			{
				field("VmPeak:", s.peak_committed);
				field("VmHWM:", s.peak_resident);
				field("VmRSS:", s.resident);
				field("VmData:", s.committed);
			}
			std::fclose(f);
		}
#endif
		return s;
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
		// How many distinct paths the query stream draws from.
		//
		// A control, not a tuning knob. A structure that is slow because it does not fit
		// in cache gets faster when the requests touch less of it; one that is slow
		// because of the work it does per lookup does not. Holding everything else fixed
		// and moving only this separates the two.
		size_t distinct = 4096;
		// How many of the table's routes the ring may draw from. Zero means all of them,
		// which is what this benchmark did before the option existed.
		//
		// Without a bound the ring samples the whole table, so the number of DISTINCT
		// paths it holds rises with the route count and table size is confounded with
		// working-set size in every scaling cell. Fixing the pool holds the queried set
		// still while the table grows.
		//
		// How fast it rises matters and is not linear. The ring is `distinct` draws with
		// replacement, so the expected distinct count is pool*(1-(1-1/pool)^distinct),
		// which at the default ring of 4096 runs:
		//
		//     routes      10    100   1000   2000   4096  10000
		//   distinct      10    100    983   1742   2589   3361
		//   % of table  100%   100%  98.3%  87.1%  63.2%  33.6%
		//
		// So the confound is present across the whole sweep, since the count is monotone,
		// but its rate falls away: from ten routes to ten thousand the table grows a
		// thousandfold and the queried set only 336-fold. A cell above a few thousand
		// routes is therefore not "the working set grew with the table" in the way a cell
		// below one thousand is, and a reading that treats the sweep as uniform in this
		// respect is wrong at both ends. Raise `distinct` with the table to keep the
		// proportion, or fix `path_set` to remove it; the two answer different questions.
		size_t path_set = 0;
		// Lookups timed between one pair of counter reads, the result divided by K.
		// Zero keeps the per-lookup histogram alone, which is what every existing record
		// carries.
		//
		// The per-lookup instrument resolves 39 ticks and pays its own ~79 ticks of read
		// overhead on every sample, which on the fastest cells is a larger share than the
		// difference being adjudicated. Timing K at once amortises that overhead by K and
		// de-quantises the reading. It costs the per-lookup tail, so both run and both are
		// reported.
		size_t batch = 0;
		// Hexadecimal CPU mask to confine this process to, 0 for none.
		//
		// A factor of the measurement rather than a convenience. On one host the same
		// binary and cell read 1174.9 ns unpinned at 6.1 per cent spread and 1089.9 at
		// 0.9 per cent pinned to one core, and which core mattered too: a different
		// choice gave 1155.1 at 4.4 per cent. Set externally the mask governs the run and
		// appears in no record, which is a factor that decides the number and cannot be
		// read back off it.
		std::uint64_t affinity_mask = 0;
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
			"  --distinct N            ring length, entries drawn with replacement (default\n"
			"                          4096); with --path-set, a control on working-set size\n"
			"  --path-set N            draw the ring from only the first N routes (default 0,\n"
			"                          the whole table). Without it the queried-path count\n"
			"                          rises with the route count and table size is\n"
			"                          confounded with working-set size\n"
			"  --affinity HEX          confine this process to this hexadecimal CPU mask.\n"
			"                          Which core is chosen changes both the median and the\n"
			"                          spread; the mask is recorded beside the measurement\n"
			"                          because it is a factor of it. No effect on macOS\n"
			"  --batch K               time K lookups between one pair of clock reads and\n"
			"                          divide (default 0, off). Amortises the timer overhead\n"
			"                          and runs where there is no cycle counter; reports a\n"
			"                          different quantity from the per-lookup histogram and\n"
			"                          must not be pooled with it\n"
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
		else if (a == "--distinct") { if (!parse_size(next("--distinct"), opt.distinct) || opt.distinct == 0) return 2; }
		else if (a == "--path-set") { if (!parse_size(next("--path-set"), opt.path_set)) return 2; }
		else if (a == "--batch") { if (!parse_size(next("--batch"), opt.batch)) return 2; }
		else if (a == "--affinity")
		{
			opt.affinity_mask = std::strtoull(std::string(next("--affinity")).c_str(), nullptr, 16);
		}
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

	const MemorySample mem_before = memory_now();
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
	const MemorySample mem_after = memory_now();
	const double build_ms = std::chrono::duration<double, std::milli>(build_t1 - build_t0).count();

	// Applied before the table is built and before any timing, so no part of the run
	// happens on a core the process is about to be moved off.
	const bool affinity_applied = apply_affinity(opt.affinity_mask);

	// ---------------------------------------------------------------- queries

	// Drawn uniformly over the table. A stream that always asked for the first route
	// would flatter a sequential scan; one that always asked for the last would
	// caricature it. Uniform is the mean case and is the one a paper can defend.
	std::mt19937_64 rng(opt.seed);
	std::vector<std::string_view> queries;
	queries.reserve(opt.distinct);
	// The pool the ring samples from. See Options::path_set for why this is not just
	// table.size().
	const size_t pool =
		(opt.path_set == 0 || opt.path_set > table.size()) ? table.size() : opt.path_set;
	for (size_t k = 0; k < opt.distinct; ++k) queries.push_back(table[rng() % pool].path);

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
#if !COROUTE_ROUTE_BENCH_HAS_TSC
	if (opt.batch == 0)
	{
		std::fprintf(stderr,
		             "this build has no cycle counter, so the per-lookup histogram is not "
		             "available; pass --batch K (1000 is the usual value) to take the batch "
		             "instrument instead\n");
		return 2;
	}
#endif

	Histogram overhead;
#if COROUTE_ROUTE_BENCH_HAS_TSC
	for (size_t k = 0; k < 4096; ++k)
	{
		const std::uint64_t t0 = tsc_now();
		const std::uint64_t t1 = tsc_now();
		overhead.add(t1 - t0);
	}
#endif

	Histogram hist;
	const auto measure_start = std::chrono::steady_clock::now();
	const auto measure_deadline = measure_start + std::chrono::duration<double>(opt.max_seconds);
	bool truncated = false;
#if COROUTE_ROUTE_BENCH_HAS_TSC
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
#endif
	const double measured_s =
		std::chrono::duration<double>(std::chrono::steady_clock::now() - measure_start).count();

	// The batch instrument, run over the same ring in the same order.
	//
	// The per-lookup loop above pays a counter read on either side of every call, and
	// reports that cost rather than subtracting it. On the fastest cells the read is a
	// larger share of the sample than the difference the cell is used to adjudicate, and
	// the counter's own step is coarse enough that the between-run spread can fall below
	// it, which is what makes a bootstrap return an interval of zero width.
	//
	// Timing K lookups between one pair of reads divides that overhead by K and gives a
	// distribution over batches rather than a histogram of a quantised per-call counter.
	// What it cannot give is the per-lookup tail, so it does not replace the histogram
	// above; both are recorded and the record says which is which.
	//
	// Samples are per-lookup cost in thousandths of a tick, so that integer bins keep
	// the resolution the division buys.
	Histogram batch_hist;
	std::uint64_t batch_lookups = 0;
	bool batch_truncated = false;
	if (opt.batch > 0)
	{
		const auto b_deadline =
			std::chrono::steady_clock::now() + std::chrono::duration<double>(opt.max_seconds);
		for (size_t b = 0; b + opt.batch <= opt.lookups; b += opt.batch)
		{
#if COROUTE_ROUTE_BENCH_HAS_TSC
			const std::uint64_t t0 = tsc_now();
#else
			const auto t0 = std::chrono::steady_clock::now();
#endif
			for (size_t j = 0; j < opt.batch; ++j)
			{
				auto m = router.match(coroute::HttpMethod::GET, queries[(b + j) % queries.size()]);
				sink += m.params.size();
			}
#if COROUTE_ROUTE_BENCH_HAS_TSC
			const double elapsed_ns = double(tsc_now() - t0) * ns_per_tick;
#else
			const double elapsed_ns =
				std::chrono::duration<double, std::nano>(std::chrono::steady_clock::now() - t0).count();
#endif
			// Picoseconds per lookup. One unit on every architecture, so a batch row from
			// an Apple host and one from an x86 host name the same quantity; and integer
			// bins at that scale keep the resolution the division buys.
			batch_hist.add(static_cast<std::uint64_t>((elapsed_ns * 1000.0) / double(opt.batch)));
			batch_lookups += opt.batch;
			if (std::chrono::steady_clock::now() >= b_deadline)
			{
				batch_truncated = true;
				break;
			}
		}
	}

	const MemorySample mem_end = memory_now();

	// ---------------------------------------------------------------- output

	auto ns = [&](std::uint32_t cycles) { return double(cycles) * ns_per_tick; };
	const double rss_mb = double(mem_after.resident - mem_before.resident) / 1048576.0;
	const double commit_mb = double(mem_after.committed - mem_before.committed) / 1048576.0;

	// Two summary lines, because the two instruments do not report the same quantity and
	// a run that has only one of them must not print the other's empty bins as zeros. A
	// benchmark whose console line reads p50=0.0ns n=0 after a successful run is the
	// failure this project exists to avoid.
	if (hist.total > 0)
	{
		std::printf("arm=%-5s routes=%6zu shape=%-4s params=%d depth=%2zu  "
		            "p50=%10.1fns p99=%11.1fns p99.9=%11.1fns  build=%9.2fms rss=%8.1fMB commit=%9.1fMB "
		            "n=%7llu%s misses=%zu\n",
		            opt.arm.c_str(), opt.routes, opt.shape.c_str(), opt.params ? 1 : 0, opt.depth,
		            ns(hist.percentile(0.50)), ns(hist.percentile(0.99)), ns(hist.percentile(0.999)), build_ms,
		            rss_mb, commit_mb, static_cast<unsigned long long>(hist.total),
		            truncated ? "*" : " ", misses);
	}
	if (batch_hist.total > 0)
	{
		// Picoseconds per lookup in the histogram, nanoseconds here.
		auto bns = [&](std::uint32_t ps) { return double(ps) / 1000.0; };
		std::printf("arm=%-5s routes=%6zu shape=%-4s params=%d depth=%2zu  batch=%zu  "
		            "p50=%10.1fns p90=%11.1fns p99=%11.1fns  build=%9.2fms rss=%8.1fMB commit=%9.1fMB "
		            "batches=%7llu%s lookups=%llu misses=%zu\n",
		            opt.arm.c_str(), opt.routes, opt.shape.c_str(), opt.params ? 1 : 0, opt.depth,
		            opt.batch, bns(batch_hist.percentile(0.50)), bns(batch_hist.percentile(0.90)),
		            bns(batch_hist.percentile(0.99)), build_ms, rss_mb, commit_mb,
		            static_cast<unsigned long long>(batch_hist.total), batch_truncated ? "*" : " ",
		            static_cast<unsigned long long>(batch_lookups), misses);
	}

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
		// rss is what was in physical memory and is capped by how much there is.
		// commit is what the process asked to be backed and is not. Above a few
		// thousand routes the DFA arm's two numbers part company, and the gap is the
		// paging.
		std::fprintf(f, "  \"rss_delta_bytes\": %lld,\n",
		             static_cast<long long>(mem_after.resident) - static_cast<long long>(mem_before.resident));
		std::fprintf(f, "  \"commit_delta_bytes\": %lld,\n",
		             static_cast<long long>(mem_after.committed) - static_cast<long long>(mem_before.committed));
		std::fprintf(f, "  \"peak_rss_bytes\": %llu,\n",
		             static_cast<unsigned long long>(mem_end.peak_resident));
		std::fprintf(f, "  \"peak_commit_bytes\": %llu,\n",
		             static_cast<unsigned long long>(mem_end.peak_committed));
		// Next to the two numbers rather than in a header block, because these are the
		// two the paper's memory column is built from and the quantity each one names
		// differs by platform.
		std::fprintf(f, "  \"memory_source\": \"%s\",\n", memory_source());
		std::fprintf(f, "  \"baseline_commit_bytes\": %llu,\n",
		             static_cast<unsigned long long>(mem_before.committed));
		std::fprintf(f, "  \"timer_overhead_cycles\": {\"p50\": %u, \"p99\": %u},\n",
		             overhead.percentile(0.50), overhead.percentile(0.99));
		// Requested and applied are separate fields, as they are for the generator: a
		// mask that was asked for and not granted is a different record from one that
		// took, and macOS grants none.
		std::fprintf(f, "  \"affinity_requested\": \"%llx\",\n",
		             static_cast<unsigned long long>(opt.affinity_mask));
		std::fprintf(f, "  \"affinity_applied\": %s,\n",
		             (opt.affinity_mask == 0) ? "null" : (affinity_applied ? "true" : "false"));
		std::fprintf(f, "  \"path_set\": %zu,\n", pool);
		// Which instruments this build actually has. Without a cycle counter the
		// per-lookup histogram is absent rather than zero, and a reader must not average
		// its bins with an x86 row's.
		std::fprintf(f, "  \"per_lookup_histogram\": %s,\n",
		             COROUTE_ROUTE_BENCH_HAS_TSC ? "true" : "false");
		std::fprintf(f, "  \"batch\": %zu,\n", opt.batch);
		if (opt.batch > 0)
		{
			// Picoseconds per lookup: divide by 1000 for nanoseconds. Named in the key
			// so the unit cannot be mistaken for the per-lookup histogram's cycles, and
			// chosen so an x86 row and a non-x86 row carry the same quantity.
			std::fprintf(f, "  \"batch_lookups\": %llu,\n",
			             static_cast<unsigned long long>(batch_lookups));
			std::fprintf(f, "  \"batch_truncated_by_time\": %s,\n",
			             batch_truncated ? "true" : "false");
			std::fprintf(f, "  \"batch_picoseconds_per_lookup\": {\"p50\": %u, \"p90\": %u, "
			                "\"p99\": %u, \"max\": %u},\n",
			             batch_hist.percentile(0.50), batch_hist.percentile(0.90),
			             batch_hist.percentile(0.99), batch_hist.percentile(1.0));
		}
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
