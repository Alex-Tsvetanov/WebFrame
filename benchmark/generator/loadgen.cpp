// A load generator for the measurements in chapter VI.
//
// Why this exists rather than h2load or wrk. The methodology requires that a
// comparison between two systems differ in the systems and not in the client, and the
// campaign spans Windows, Linux and macOS. h2load does not build on Windows without
// effort that would itself become a variable, and using a different generator per
// platform would confound every cross-platform statement with the client. One
// generator, one latency model, three platforms is the weaker tool and the stronger
// method.
//
// What it deliberately is not: it speaks HTTP/1.1 only. Cross-protocol comparisons
// need h2load and belong to the Linux campaign, where h2load builds. This measures
// HTTP/1.1 throughput and latency, which is what the demultiplexing arms, the backlog
// sweep and the worker sweep need, and it says so rather than pretending to more.
//
// Two loops, because they measure different quantities:
//
//   closed loop (--rate 0)  C connections, each sending the next request only after
//                           the previous response. Reports SERVICE time. A stall stops
//                           the client issuing, so the requests that would have been
//                           delayed are never sent: this understates the tail by
//                           construction and is labelled accordingly.
//
//   open loop (--rate N)    requests are offered on a fixed schedule regardless of
//                           whether earlier ones completed. Latency is measured from
//                           the INTENDED send time, not from when the socket was
//                           actually written, which is the whole of the
//                           coordinated-omission correction.

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <thread>
#include <vector>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <mmsystem.h>
using socket_t = SOCKET;
constexpr socket_t kInvalidSocket = INVALID_SOCKET;
#define poll_fn WSAPoll
using poll_fd = WSAPOLLFD;
using poll_count_t = ULONG;
#else
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <sched.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <unistd.h>
using socket_t = int;
constexpr socket_t kInvalidSocket = -1;
#define poll_fn poll
using poll_fd = struct pollfd;
using poll_count_t = nfds_t;
#endif

namespace
{
	using Clock = std::chrono::steady_clock;
	using Nanos = std::chrono::nanoseconds;

	// ------------------------------------------------------------------ options

	struct Options
	{
		std::string host = "127.0.0.1";
		std::string path = "/";
		std::uint16_t port = 8080;
		std::size_t connections = 64;
		std::size_t threads = 4;
		double duration_s = 10.0;
		double warmup_s = 2.0;
		double rate = 0.0;  // requests per second, 0 means closed loop
		std::string out_path;
		std::string samples_path;
		// A CPU mask, hexadecimal. Zero leaves the process wherever the scheduler puts
		// it. On one host the generator and the server compete for the same cores, and
		// a measurement in which they compete is partly a measurement of the scheduler.
		std::uint64_t affinity_mask = 0;
	};

	// ------------------------------------------------------------- per-thread

	// Everything one worker thread observed. Merged after the run, so no thread ever
	// touches another's counters and there is nothing to synchronise on the hot path.
	struct ThreadResult
	{
		std::uint64_t completed = 0;
		std::uint64_t non_2xx = 0;
		std::uint64_t socket_errors = 0;
		// A server closing a keep-alive connection on purpose is not an error and must
		// not be counted as one. The first run reported 9502 "errors" that were the
		// server's own per-connection request limit doing its job.
		std::uint64_t server_closes = 0;
		std::uint64_t bytes_read = 0;
		// Microseconds. Raw rather than bucketed: the tail is the phenomenon, and
		// percentiles computed from raw samples cannot be wrong about it.
		std::vector<std::uint32_t> latencies_us;
		// How late the generator itself was: the gap between when a request was due
		// and when it actually reached the socket.
		//
		// This is the saturation signal for an open loop, and CPU is not. Pacing at a
		// 400 microsecond period means spinning, because no sleep is accurate to that,
		// so the generator sits at 100 percent CPU whether it is keeping up or not. What
		// actually matters is whether the offered schedule was met, and that is this.
		std::vector<std::uint32_t> pacing_us;
	};

	// ------------------------------------------------------------------ socket

	void close_socket(socket_t s)
	{
#if defined(_WIN32)
		::closesocket(s);
#else
		::close(s);
#endif
	}

	bool set_non_blocking(socket_t s)
	{
#if defined(_WIN32)
		u_long mode = 1;
		return ::ioctlsocket(s, FIONBIO, &mode) == 0;
#else
		const int flags = ::fcntl(s, F_GETFL, 0);
		return flags >= 0 && ::fcntl(s, F_SETFL, flags | O_NONBLOCK) == 0;
#endif
	}

	bool would_block()
	{
#if defined(_WIN32)
		const int e = ::WSAGetLastError();
		return e == WSAEWOULDBLOCK || e == WSAEINPROGRESS;
#else
		return errno == EAGAIN || errno == EWOULDBLOCK || errno == EINPROGRESS;
#endif
	}

	socket_t connect_to(const sockaddr_in& addr)
	{
		socket_t s = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (s == kInvalidSocket)
		{
			return kInvalidSocket;
		}

		// Nagle off. With it on, a small request can sit in the sending stack waiting
		// for an ack, and the measurement becomes one of Nagle rather than of the
		// server.
		int one = 1;
		::setsockopt(s, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<const char*>(&one), sizeof(one));

		if (::connect(s, reinterpret_cast<const sockaddr*>(&addr), sizeof(addr)) != 0)
		{
			if (!would_block())
			{
				close_socket(s);
				return kInvalidSocket;
			}
		}
		if (!set_non_blocking(s))
		{
			close_socket(s);
			return kInvalidSocket;
		}
		return s;
	}

	// ------------------------------------------------------- response scanning

	// Just enough of an HTTP/1.1 response to know when one has finished and whether it
	// was a success.
	//
	// Allocation free on purpose. The first version built two std::strings per response
	// to find the status and the length, which at 160k responses a second is 320k
	// allocations a second, and the generator failed its own saturation rule. A load
	// generator that spends its time in the allocator is measuring the allocator.
	struct ResponseScanner
	{
		std::string buffer;
		std::size_t head = 0;  // consumed prefix; the buffer is compacted, not erased
		bool in_body = false;
		std::size_t body_remaining = 0;
		bool chunked = false;

		void reset()
		{
			buffer.clear();
			head = 0;
			in_body = false;
			body_remaining = 0;
			chunked = false;
		}

		static char lower(char c) { return static_cast<char>(c >= 'A' && c <= 'Z' ? c + 32 : c); }

		// Finds the blank line that ends a header block.
		//
		// memchr rather than a character loop: the naive version scanned every byte of
		// the header four times with a function call per comparison, and together with
		// the header search below it put the generator at 98 percent CPU, where its own
		// saturation rule refuses the run.
		static std::size_t find_header_end(const char* p, std::size_t n)
		{
			std::size_t i = 0;
			while (i + 3 < n)
			{
				const void* hit = std::memchr(p + i, '\r', n - i - 3);
				if (hit == nullptr)
				{
					return std::string::npos;
				}
				const std::size_t at = static_cast<std::size_t>(static_cast<const char*>(hit) - p);
				if (p[at + 1] == '\n' && p[at + 2] == '\r' && p[at + 3] == '\n')
				{
					return at;
				}
				i = at + 1;
			}
			return std::string::npos;
		}

		// Content-Length, found by walking the field lines rather than by searching the
		// whole header for a fifteen byte needle. Most lines are rejected on their first
		// character.
		static std::size_t find_content_length(const char* p, std::size_t header_len, bool& chunked)
		{
			chunked = false;
			std::size_t line = 0;
			while (line < header_len)
			{
				const void* nl = std::memchr(p + line, '\n', header_len - line);
				const std::size_t stop =
				    nl ? static_cast<std::size_t>(static_cast<const char*>(nl) - p) : header_len;

				const char first = lower(p[line]);
				if (first == 'c' && stop - line > 15 &&
				    match_ci(p + line, "content-length:", 15))
				{
					std::size_t v = line + 15;
					while (v < stop && (p[v] == ' ' || p[v] == '\t'))
					{
						++v;
					}
					std::size_t value = 0;
					while (v < stop && p[v] >= '0' && p[v] <= '9')
					{
						value = value * 10 + static_cast<std::size_t>(p[v] - '0');
						++v;
					}
					return value;
				}
				if (first == 't' && stop - line > 18 &&
				    match_ci(p + line, "transfer-encoding:", 18))
				{
					chunked = true;
					return 0;
				}

				if (nl == nullptr)
				{
					break;
				}
				line = stop + 1;
			}
			return 0;
		}

		static bool match_ci(const char* p, const char* needle_lower, std::size_t m)
		{
			for (std::size_t k = 0; k < m; ++k)
			{
				if (lower(p[k]) != needle_lower[k])
				{
					return false;
				}
			}
			return true;
		}

		// Drops what has already been consumed, but only when it is worth the move, so
		// the common case of several responses in one read does no copying at all.
		void compact()
		{
			if (head == 0)
			{
				return;
			}
			if (head == buffer.size())
			{
				buffer.clear();
				head = 0;
				return;
			}
			if (head > 8192)
			{
				buffer.erase(0, head);
				head = 0;
			}
		}

		// Consumes as much of the buffer as forms complete responses, counting any that
		// were not 2xx.
		std::size_t consume(std::size_t& non_2xx)
		{
			std::size_t finished = 0;
			const char* base = buffer.data();

			for (;;)
			{
				const std::size_t avail = buffer.size() - head;

				if (!in_body)
				{
					const char* p = base + head;
					const std::size_t sep = find_header_end(p, avail);
					if (sep == std::string::npos)
					{
						break;
					}

					int status = 0;
					if (sep > 12 && std::memcmp(p, "HTTP/", 5) == 0)
					{
						status = ((p[9] - '0') * 100) + ((p[10] - '0') * 10) + (p[11] - '0');
					}
					if (status < 200 || status > 299)
					{
						++non_2xx;
					}

					body_remaining = find_content_length(p, sep, chunked);

					head += sep + 4;
					in_body = true;
					continue;
				}

				if (chunked)
				{
					// The measured surfaces answer with a Content-Length. Decoding chunked
					// here would spend time on a path the measurement never takes.
					const std::size_t term = find_header_end(base + head, buffer.size() - head);
					if (term == std::string::npos)
					{
						break;
					}
					head += term + 4;
				}
				else
				{
					if (buffer.size() - head < body_remaining)
					{
						break;
					}
					head += body_remaining;
				}

				++finished;
				in_body = false;
			}

			compact();
			return finished;
		}
	};

	// ------------------------------------------------------------- connection

	struct Conn
	{
		socket_t fd = kInvalidSocket;
		ResponseScanner scanner;
		std::size_t sent_offset = 0;
		bool awaiting = false;         // a request is outstanding
		Clock::time_point issued{};    // when this request was intended to be sent
		bool connected = false;
	};

	// ------------------------------------------------------------------ worker

	// One thread drives a slice of the connections with poll. Not one thread per
	// connection: at a few hundred connections the scheduler noise from that would be
	// larger than the differences being measured.
	void worker(const Options& opt, const sockaddr_in& addr, std::size_t count, double rate_per_thread,
	            Clock::time_point start, Clock::time_point warmup_end, Clock::time_point stop,
	            ThreadResult& out)
	{
		const std::string request = "GET " + opt.path +
		                            " HTTP/1.1\r\nHost: " + opt.host +
		                            "\r\nConnection: keep-alive\r\nUser-Agent: coroute-loadgen\r\n\r\n";

		std::vector<Conn> conns(count);
		std::vector<poll_fd> pfds(count);

		for (auto& c : conns)
		{
			c.fd = connect_to(addr);
			if (c.fd == kInvalidSocket)
			{
				++out.socket_errors;
			}
		}

		// Open loop: a fixed schedule this thread must keep to, regardless of whether
		// responses have come back. The next intended send time advances by the period
		// whether or not anything is free to send it, which is what stops a stall from
		// hiding the requests it delayed.
		const bool open_loop = rate_per_thread > 0.0;
		const Nanos period = open_loop
		                         ? Nanos(static_cast<std::int64_t>(1e9 / rate_per_thread))
		                         : Nanos(0);
		Clock::time_point next_due = start;

		out.latencies_us.reserve(1u << 16);

		while (Clock::now() < stop)
		{
			const Clock::time_point now = Clock::now();

			// Issue whatever is due.
			for (std::size_t i = 0; i < conns.size(); ++i)
			{
				Conn& c = conns[i];
				if (c.fd == kInvalidSocket || c.awaiting)
				{
					continue;
				}

				Clock::time_point intended = now;
				if (open_loop)
				{
					if (next_due > now)
					{
						break;  // nothing is due yet
					}
					intended = next_due;
					next_due += period;
				}

				c.issued = intended;
				c.sent_offset = 0;
				c.awaiting = true;
			}

			for (std::size_t i = 0; i < conns.size(); ++i)
			{
				Conn& c = conns[i];
				pfds[i].fd = c.fd;
				pfds[i].events = 0;
				pfds[i].revents = 0;
				if (c.fd == kInvalidSocket)
				{
					continue;
				}
				if (c.awaiting && c.sent_offset < request.size())
				{
					pfds[i].events |= POLLOUT;
				}
				pfds[i].events |= POLLIN;
			}

			// How long to wait is a measurement decision, not a detail.
			//
			// A fixed one millisecond timeout looks harmless and is not. On Windows the
			// system timer granularity is 15.6 ms by default, so a poll asked to wait one
			// millisecond waits a full tick. The generator then woke sixteen times a
			// second, issued every slot that had come due in the meantime, and every
			// request carried the distance from its own due time to that tick. The
			// latency distribution came out uniform between zero and 15.6 ms: p50 7.2 ms
			// and p99 14.5 ms at an offered rate of ten thousand, which is the generator
			// measuring its own clock rather than the server.
			//
			// So the wait is bounded by the next due slot, and the last stretch before it
			// is spun rather than slept, because no sleep on either platform is accurate
			// to the tens of microseconds an open loop at these rates needs.
			int timeout_ms = 1;
			if (open_loop)
			{
				const auto until = next_due - Clock::now();
				if (until <= Nanos(0))
				{
					timeout_ms = 0;
				}
				else
				{
					const auto us = std::chrono::duration_cast<std::chrono::microseconds>(until).count();
					timeout_ms = us < 500 ? 0 : static_cast<int>(std::min<std::int64_t>(us / 1000, 1));
				}
			}

			const int ready =
			    poll_fn(pfds.data(), static_cast<poll_count_t>(pfds.size()), timeout_ms);
			if (ready < 0)
			{
				continue;
			}

			char buf[16384];

			for (std::size_t i = 0; i < conns.size(); ++i)
			{
				Conn& c = conns[i];
				if (c.fd == kInvalidSocket)
				{
					continue;
				}

				// POLLHUP alone is the peer having closed, which on a keep-alive connection
				// is the server enforcing its per-connection request limit. Only POLLERR
				// and POLLNVAL are faults. Readable data is taken below either way, so a
				// response that arrived together with the close is not discarded.
				if ((pfds[i].revents & (POLLERR | POLLNVAL)) != 0)
				{
					close_socket(c.fd);
					c.fd = connect_to(addr);
					c.scanner.reset();
					c.awaiting = false;
					++out.socket_errors;
					continue;
				}

				if ((pfds[i].revents & POLLOUT) != 0 && c.awaiting && c.sent_offset < request.size())
				{
					const int n = ::send(c.fd, request.data() + c.sent_offset,
					                     static_cast<int>(request.size() - c.sent_offset), 0);
					if (n > 0)
					{
						c.sent_offset += static_cast<std::size_t>(n);
						if (c.sent_offset == request.size() && c.issued >= warmup_end)
						{
							const auto late = std::chrono::duration_cast<std::chrono::microseconds>(
							                      Clock::now() - c.issued)
							                      .count();
							out.pacing_us.push_back(
							    static_cast<std::uint32_t>(late < 0 ? 0 : late));
						}
					}
					else if (n < 0 && !would_block())
					{
						close_socket(c.fd);
						c.fd = kInvalidSocket;
						++out.socket_errors;
						continue;
					}
				}

				if ((pfds[i].revents & POLLIN) != 0)
				{
					const int n = ::recv(c.fd, buf, sizeof(buf), 0);
					if (n > 0)
					{
						out.bytes_read += static_cast<std::uint64_t>(n);
						c.scanner.buffer.append(buf, static_cast<std::size_t>(n));

						std::size_t bad = 0;
						const std::size_t done = c.scanner.consume(bad);
						out.non_2xx += bad;

						if (done > 0)
						{
							const Clock::time_point at = Clock::now();
							// Warmup responses are counted for correctness but their
							// latencies are not kept: the first connections pay for
							// accept and for a cold allocator, and that is not what is
							// being compared.
							// Filtered on when the request was ISSUED, not on when it completed. The
							// first version filtered on completion, so a request issued during the
							// warmup and finishing just after it contributed its whole waiting time
							// to the measured window: one run showed 2.1 second samples in the tail
							// of a distribution whose p99 was 785 microseconds.
							if (c.issued >= warmup_end)
							{
								const auto us = std::chrono::duration_cast<std::chrono::microseconds>(
								                    at - c.issued)
								                    .count();
								for (std::size_t k = 0; k < done; ++k)
								{
									out.latencies_us.push_back(static_cast<std::uint32_t>(
									    us < 0 ? 0 : (us > 0xFFFFFFFFll ? 0xFFFFFFFFll : us)));
								}
								out.completed += done;
							}
							c.awaiting = false;
						}
					}
					else if (n == 0)
					{
						// Clean close by the server. Reconnect and carry on; an outstanding request
						// is reissued rather than counted as completed.
						close_socket(c.fd);
						c.fd = connect_to(addr);
						c.scanner.reset();
						c.awaiting = false;
						++out.server_closes;
					}
					else if (!would_block())
					{
						close_socket(c.fd);
						c.fd = kInvalidSocket;
						++out.socket_errors;
					}
				}
			}
		}

		for (auto& c : conns)
		{
			if (c.fd != kInvalidSocket)
			{
				close_socket(c.fd);
			}
		}
	}

	// ------------------------------------------------------------------- CPU

	// The generator's own CPU as a fraction of one core times the thread count. The
	// validity rules refuse a run where this is high, because a saturated generator is
	// measuring itself, and that is the most common way a framework benchmark is wrong.
	// Applied by the process to itself at startup, before any thread is created, so
	// there is no window in which threads run somewhere else and then migrate.
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
		// macOS has no portable affinity API. The run records the mask it was asked
		// for and whether it was applied, so a reader can tell the difference.
		return false;
#endif
	}

	double process_cpu_seconds()
	{
#if defined(_WIN32)
		FILETIME creation, exit, kernel, user;
		if (!::GetProcessTimes(::GetCurrentProcess(), &creation, &exit, &kernel, &user))
		{
			return -1.0;
		}
		auto to_seconds = [](const FILETIME& ft) {
			ULARGE_INTEGER v;
			v.LowPart = ft.dwLowDateTime;
			v.HighPart = ft.dwHighDateTime;
			return static_cast<double>(v.QuadPart) / 1e7;  // 100 ns units
		};
		return to_seconds(kernel) + to_seconds(user);
#else
		rusage ru{};
		if (::getrusage(RUSAGE_SELF, &ru) != 0)
		{
			return -1.0;
		}
		return static_cast<double>(ru.ru_utime.tv_sec) + static_cast<double>(ru.ru_utime.tv_usec) / 1e6 +
		       static_cast<double>(ru.ru_stime.tv_sec) + static_cast<double>(ru.ru_stime.tv_usec) / 1e6;
#endif
	}

	// -------------------------------------------------------------- reporting

	std::uint32_t percentile(const std::vector<std::uint32_t>& sorted, double p)
	{
		if (sorted.empty())
		{
			return 0;
		}
		const double rank = p * static_cast<double>(sorted.size() - 1);
		const std::size_t idx = static_cast<std::size_t>(rank + 0.5);
		return sorted[std::min(idx, sorted.size() - 1)];
	}

	void usage()
	{
		std::puts(
		    "loadgen: an HTTP/1.1 load generator for the coroute measurements\n"
		    "\n"
		    "  --host HOST         default 127.0.0.1\n"
		    "  --port N            default 8080\n"
		    "  --path PATH         default /\n"
		    "  --connections N     total connections, default 64\n"
		    "  --threads N         default 4\n"
		    "  --duration S        measured seconds, default 10\n"
		    "  --warmup S          discarded seconds before measuring, default 2\n"
		    "  --rate N            open loop at N requests per second; 0 (default) is\n"
		    "                      a closed loop, which measures service time\n"
		    "  --out FILE          write the result as JSON\n"
		    "  --samples FILE      write every latency sample, one per line, in us\n"
		    "\n"
		    "In open loop the latency of a request is measured from the time it was due,\n"
		    "not from the time the socket accepted it. That is the difference between\n"
		    "response time and service time, and it is the reason a closed loop tail\n"
		    "cannot be reported as the former.");
	}
}  // namespace

int main(int argc, char** argv)
{
	Options opt;

	for (int i = 1; i < argc; ++i)
	{
		const std::string a = argv[i];
		auto next = [&](const char* name) -> std::string {
			if (i + 1 >= argc)
			{
				std::fprintf(stderr, "%s needs a value\n", name);
				std::exit(2);
			}
			return argv[++i];
		};

		if (a == "--host") opt.host = next("--host");
		else if (a == "--port") opt.port = static_cast<std::uint16_t>(std::stoi(next("--port")));
		else if (a == "--path") opt.path = next("--path");
		else if (a == "--connections") opt.connections = static_cast<std::size_t>(std::stoul(next("--connections")));
		else if (a == "--threads") opt.threads = static_cast<std::size_t>(std::stoul(next("--threads")));
		else if (a == "--duration") opt.duration_s = std::stod(next("--duration"));
		else if (a == "--warmup") opt.warmup_s = std::stod(next("--warmup"));
		else if (a == "--rate") opt.rate = std::stod(next("--rate"));
		else if (a == "--out") opt.out_path = next("--out");
		else if (a == "--samples") opt.samples_path = next("--samples");
		else if (a == "--affinity") opt.affinity_mask = std::strtoull(next("--affinity").c_str(), nullptr, 16);
		else if (a == "--help" || a == "-h") { usage(); return 0; }
		else { std::fprintf(stderr, "unknown option: %s\n", a.c_str()); usage(); return 2; }
	}

	if (opt.threads == 0) opt.threads = 1;
	if (opt.connections < opt.threads) opt.connections = opt.threads;

#if defined(_WIN32)
	WSADATA wsa;
	if (::WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		std::fprintf(stderr, "WSAStartup failed\n");
		return 1;
	}
#endif

	sockaddr_in addr{};
	addr.sin_family = AF_INET;
	addr.sin_port = ::htons(opt.port);
	if (::inet_pton(AF_INET, opt.host.c_str(), &addr.sin_addr) != 1)
	{
		std::fprintf(stderr, "--host must be a literal IPv4 address\n");
		return 2;
	}

#if defined(_WIN32)
	// Ask for a one millisecond timer instead of the 15.6 ms default. Without it every
	// sleep in the process is quantised to a tick, which for an open loop generator
	// means the pacing is quantised to a tick as well. Released at exit so the request
	// does not outlive the process that needed it.
	const bool timer_raised = ::timeBeginPeriod(1) == TIMERR_NOERROR;
#endif

	const bool affinity_applied = apply_affinity(opt.affinity_mask);
	if (opt.affinity_mask != 0 && !affinity_applied)
	{
		std::fprintf(stderr, "warning: could not apply affinity mask %llx\n",
		             static_cast<unsigned long long>(opt.affinity_mask));
	}

	const auto start = Clock::now();
	const auto warmup_end = start + std::chrono::duration_cast<Clock::duration>(
	                                    std::chrono::duration<double>(opt.warmup_s));
	const auto stop = warmup_end + std::chrono::duration_cast<Clock::duration>(
	                                   std::chrono::duration<double>(opt.duration_s));


	std::vector<ThreadResult> results(opt.threads);
	std::vector<std::thread> pool;
	pool.reserve(opt.threads);

	const std::size_t per_thread = opt.connections / opt.threads;
	std::size_t remainder = opt.connections % opt.threads;
	const double rate_per_thread = opt.rate > 0.0 ? opt.rate / static_cast<double>(opt.threads) : 0.0;

	for (std::size_t t = 0; t < opt.threads; ++t)
	{
		const std::size_t mine = per_thread + (remainder > 0 ? 1 : 0);
		if (remainder > 0) --remainder;
		pool.emplace_back(worker, std::cref(opt), std::cref(addr), mine, rate_per_thread, start, warmup_end,
		                  stop, std::ref(results[t]));
	}
	// Sampled across the measured window only. The first version sampled before the
	// warmup and divided by the post-warmup wall time, which reported more CPU than
	// the threads could physically have used and failed its own saturation rule for
	// arithmetic reasons rather than real ones.
	std::this_thread::sleep_until(warmup_end);
	const double cpu_before = process_cpu_seconds();

	for (auto& th : pool)
	{
		th.join();
	}

	const double cpu_after = process_cpu_seconds();
	const double wall = std::chrono::duration<double>(Clock::now() - warmup_end).count();

	std::uint64_t completed = 0, non_2xx = 0, sock_errors = 0, bytes = 0, closes = 0;
	std::vector<std::uint32_t> all;
	std::vector<std::uint32_t> pacing;
	for (const auto& r : results)
	{
		completed += r.completed;
		non_2xx += r.non_2xx;
		sock_errors += r.socket_errors;
		closes += r.server_closes;
		bytes += r.bytes_read;
		all.insert(all.end(), r.latencies_us.begin(), r.latencies_us.end());
		pacing.insert(pacing.end(), r.pacing_us.begin(), r.pacing_us.end());
	}
	std::sort(all.begin(), all.end());
	std::sort(pacing.begin(), pacing.end());

	const double cpu_used = (cpu_before >= 0.0 && cpu_after >= 0.0) ? (cpu_after - cpu_before) : -1.0;
	// Against all cores the generator could have used, which is how the validity rule
	// is stated: a generator pinned at its own thread count is saturated.
	// Against the cores the generator was actually allowed to use, not against its
	// thread count. With an affinity mask narrower than the thread count the two differ,
	// and dividing by threads reports a generator as idle when it is in fact pinned flat
	// against the cores it was given.
	std::size_t usable_cores = opt.threads;
	if (opt.affinity_mask != 0)
	{
		std::size_t bits = 0;
		for (int i = 0; i < 64; ++i)
		{
			bits += static_cast<std::size_t>((opt.affinity_mask >> i) & 1ULL);
		}
		usable_cores = std::min(opt.threads, bits == 0 ? opt.threads : bits);
	}
	const double cpu_fraction = (cpu_used >= 0.0 && wall > 0.0)
	                                ? cpu_used / (wall * static_cast<double>(usable_cores))
	                                : -1.0;

	const double rps = wall > 0.0 ? static_cast<double>(completed) / wall : 0.0;
	const double error_rate = completed + non_2xx > 0
	                              ? static_cast<double>(non_2xx) / static_cast<double>(completed + non_2xx)
	                              : 0.0;

	// Built field by field rather than with one large snprintf.
	//
	// The format string and the argument list drifted apart three times while this was
	// being written, and each time the result was a plausible looking JSON document with
	// the wrong numbers in it. A measurement tool that can silently mislabel its own
	// output is worse than one that is slow.
	std::string json;
	json.reserve(1024);

	auto field_s = [&json](const char* name, const std::string& value, bool quote = true)
	{
		json += "  \"";
		json += name;
		json += "\": ";
		if (quote) json += '"';
		json += value;
		if (quote) json += '"';
		json += ",\n";
	};
	auto field_u = [&json](const char* name, unsigned long long value)
	{
		json += "  \"";
		json += name;
		json += "\": ";
		json += std::to_string(value);
		json += ",\n";
	};
	auto field_f = [&json](const char* name, double value, int places)
	{
		char buf[64];
		std::snprintf(buf, sizeof(buf), "%.*f", places, value);
		json += "  \"";
		json += name;
		json += "\": ";
		json += buf;
		json += ",\n";
	};

	char mask_hex[32];
	std::snprintf(mask_hex, sizeof(mask_hex), "%llx", static_cast<unsigned long long>(opt.affinity_mask));

	json += "{\n";
	field_s("generator", "coroute-loadgen");
	field_s("loop", opt.rate > 0.0 ? "open" : "closed");
	field_s("latency_kind", opt.rate > 0.0 ? "response_time" : "service_time");
	field_s("host", opt.host);
	field_u("port", opt.port);
	field_s("path", opt.path);
	field_u("connections", opt.connections);
	field_u("threads", opt.threads);
	field_u("usable_cores", usable_cores);
	field_s("affinity_mask", mask_hex);
	field_s("affinity_applied", affinity_applied ? "true" : "false", false);
	field_f("offered_rate", opt.rate, 3);
	field_f("duration_s", wall, 6);
	field_u("completed", completed);
	field_u("non_2xx", non_2xx);
	field_u("socket_errors", sock_errors);
	field_u("server_closes", closes);
	field_u("bytes_read", bytes);
	field_f("rps", rps, 3);
	field_f("error_rate", error_rate, 9);
	field_f("generator_cpu_seconds", cpu_used, 6);
	field_f("generator_cpu_fraction", cpu_fraction, 6);

	auto pct = [&json](const char* name, unsigned value, bool last = false)
	{
		json += "    \"";
		json += name;
		json += "\": ";
		json += std::to_string(value);
		json += last ? "\n" : ",\n";
	};

	json += "  \"latency_us\": {\n";
	json += "    \"samples\": " + std::to_string(all.size()) + ",\n";
	pct("min", all.empty() ? 0U : all.front());
	pct("p50", percentile(all, 0.50));
	pct("p75", percentile(all, 0.75));
	pct("p90", percentile(all, 0.90));
	pct("p99", percentile(all, 0.99));
	pct("p999", percentile(all, 0.999));
	pct("p9999", percentile(all, 0.9999));
	pct("max", all.empty() ? 0U : all.back(), true);
	json += "  },\n";

	json += "  \"pacing_us\": {\n";
	json += "    \"samples\": " + std::to_string(pacing.size()) + ",\n";
	pct("p50", percentile(pacing, 0.50));
	pct("p99", percentile(pacing, 0.99));
	pct("max", pacing.empty() ? 0U : pacing.back(), true);
	json += "  }\n}\n";

	std::fputs(json.c_str(), stdout);

	if (!opt.out_path.empty())
	{
		if (FILE* f = std::fopen(opt.out_path.c_str(), "wb"))
		{
			std::fputs(json.c_str(), f);
			std::fclose(f);
		}
	}

	if (!opt.samples_path.empty())
	{
		if (FILE* f = std::fopen(opt.samples_path.c_str(), "wb"))
		{
			for (std::uint32_t v : all)
			{
				std::fprintf(f, "%u\n", v);
			}
			std::fclose(f);
		}
	}

#if defined(_WIN32)
	if (timer_raised)
	{
		::timeEndPeriod(1);
	}
	::WSACleanup();
#endif

	// A non-zero exit for a run that cannot be a data point, so a driver that ignores
	// the JSON still cannot record it by accident.
	// Two different rules, because the two loops fail in different ways.
	//
	// A closed loop can only be limited by its own CPU, so that is the check. An open
	// loop paces by spinning and is at full CPU by construction, so CPU says nothing;
	// what matters is whether it met the schedule it promised. A generator that fell
	// milliseconds behind its own due times was offering a different load than the one
	// recorded, and the run is a measurement of the generator.
	const bool open_loop = opt.rate > 0.0;
	const std::uint32_t pacing_p99 = percentile(pacing, 0.99);
	const double achieved_share = opt.rate > 0.0 ? rps / opt.rate : 1.0;

	if (error_rate > 0.001)
	{
		std::fprintf(stderr, "run is not admissible: error_rate=%.6f\n", error_rate);
		return 3;
	}
	if (!open_loop && cpu_fraction >= 0.0 && cpu_fraction > 0.85)
	{
		std::fprintf(stderr,
		             "run is not admissible: closed loop with generator_cpu_fraction=%.3f\n",
		             cpu_fraction);
		return 3;
	}
	if (open_loop && (pacing_p99 > 1000 || achieved_share < 0.99))
	{
		std::fprintf(stderr,
		             "run is not admissible: pacing_p99=%uus achieved=%.1f%% of offered\n",
		             pacing_p99, achieved_share * 100.0);
		return 3;
	}
	return 0;
}
