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
// What it deliberately is not: it speaks HTTP/1.1 only, over cleartext or TLS.
// Cross-protocol comparisons need h2load and belong to the Linux campaign, where h2load
// builds. This measures HTTP/1.1 throughput and latency, which is what the
// demultiplexing arms, the backlog sweep and the worker sweep need, and it says so
// rather than pretending to more.
//
// On TLS. The claim under test is that one descriptor serves TLS and cleartext at no
// cost against dedicated listeners, so half of it cannot be measured by a client that
// only speaks cleartext. The TLS path here is OpenSSL over the same non-blocking socket
// and the same poll loop, so the two arms differ in the transport and in nothing else.
// Certificate verification is off: the rig uses a self-signed certificate, and a client
// that verified it would be measuring a trust store. That is a limitation of the
// measurement and is reported with it rather than hidden.
//
// On connection establishment. Classification happens once per connection, so its cost
// concentrates entirely in setup and is amortised to nothing across a keep-alive
// connection serving a hundred thousand requests. Establishment is therefore timed
// separately, from just before connect to the moment the connection is usable, and
// reported as its own distribution. Driving the server with --max-requests 1 turns that
// into the primary measurement rather than a footnote.
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
#include <csignal>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <string>
#include <thread>
#include <vector>

// Optional, and the CMake in this directory defines it only when OpenSSL was found. A
// machine without OpenSSL still builds a generator that runs every cleartext campaign;
// asking for --tls on that build fails at startup with a message rather than silently
// connecting in cleartext and recording the run as TLS.
#ifdef COROUTE_LOADGEN_TLS
#include <openssl/err.h>
#include <openssl/ssl.h>
#endif

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
// windows.h defines function-like min and max macros unless this is set, and MSVC then
// reads std::min( as std:: followed by a macro expansion, which is error C2589. The
// library sets it for its own targets, but this generator is deliberately not linked
// against the library (see CMakeLists.txt in this directory), so it cannot inherit the
// definition and has to make it itself, next to the guard it already makes.
//
// Latent until someone points MSVC at this file: MinGW's headers leave min and max
// alone for C++ translation units, so every build this project has ever made compiled.
#ifndef NOMINMAX
#define NOMINMAX
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
#include <ifaddrs.h>
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
		// A file of request paths, one per line, cycled by every connection. Written by
		// the server from the same generator it registered its routes with, so the
		// client cannot end up asking for a table the server does not have.
		std::string paths_file;
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
		// Speak TLS. The server side of this is --tls CERT KEY, and with classification
		// on it is the same port and the same descriptor as the cleartext arm.
		bool tls = false;
		// The name sent in SNI. Defaults to the host, which for these campaigns is a
		// literal address and therefore sends no SNI at all; a rig whose certificate
		// carries a name needs this set to it.
		std::string sni;
		// One request per connection, closed by this side as soon as the response is in.
		//
		// The alternative, letting the server close at its own per-connection request
		// limit, races: the client has already put the next request on the wire when the
		// close arrives, and the stack answers a write to a closed connection with a
		// reset. That is reported as a socket error, and any socket error makes the run
		// inadmissible, so the measurement this flag exists for could never be taken.
		// Measured before it was fixed: 2275 socket errors against 2307 connections.
		//
		// The requests also carry Connection: close, so the server is told rather than
		// discovering it.
		bool reconnect = false;
	};

	// ------------------------------------------------------------- per-thread

	// Everything one worker thread observed. Merged after the run, so no thread ever
	// touches another's counters and there is nothing to synchronise on the hot path.
	struct ThreadResult
	{
		std::uint64_t completed = 0;
		// Every response the connection scanner saw, warmup included. `completed` is
		// the measured window only, so a counter that runs for the process lifetime,
		// such as the server's syscalls, has to be divided by this one or the ratio
		// moves with the warmup setting rather than with the server.
		std::uint64_t responses_total = 0;
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
		// How long each connection took to become usable: from just before connect to
		// the completed TCP handshake, and on the TLS arm to the completed TLS handshake
		// as well.
		//
		// Kept apart from latencies_us because it answers a different question. Request
		// latency divides the classification cost by every request the connection went
		// on to serve; this does not divide it by anything. It is the only place in the
		// run where one extra read and one comparison per connection could be visible.
		std::vector<std::uint32_t> connect_us;
		std::uint64_t established = 0;
		std::uint64_t handshake_failures = 0;
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

	// How long to wait for a TCP handshake before giving up on it.
	//
	// This exists because the default is minutes. A connect to a listener whose accept
	// queue is full has its SYN dropped rather than refused, the stack retransmits with
	// exponential backoff, and the call returns after about two minutes. That is longer
	// than most runs, so one stalled connect takes a worker thread out of the run
	// entirely and the run is then rejected for having offered a load it never offered.
	//
	// Observed rather than reasoned about: a connection-churn run against a server whose
	// backlog had filled sat in connect for 127 seconds on a four second run.
	constexpr int kConnectTimeoutMs = 2000;

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

		// Non-blocking before the connect rather than after it, so the wait below is
		// this function's and not the operating system's. A successful connect behaves
		// exactly as it did before: the function still returns only once the handshake
		// has completed, which is what makes the establishment timing meaningful.
		if (!set_non_blocking(s))
		{
			close_socket(s);
			return kInvalidSocket;
		}

		if (::connect(s, reinterpret_cast<const sockaddr*>(&addr), sizeof(addr)) != 0)
		{
			if (!would_block())
			{
				close_socket(s);
				return kInvalidSocket;
			}

			poll_fd pfd{};
			pfd.fd = s;
			pfd.events = POLLOUT;
			const int ready = poll_fn(&pfd, 1, kConnectTimeoutMs);
			if (ready <= 0)
			{
				close_socket(s);
				return kInvalidSocket;
			}

			// Writable does not mean connected: a refused connection is also writable,
			// and reading SO_ERROR is the only way to tell. Without this a failed
			// connect becomes a connection that fails on its first send instead, which
			// is counted in a different column.
			int err = 0;
			socklen_t len = sizeof(err);
			if (::getsockopt(s, SOL_SOCKET, SO_ERROR, reinterpret_cast<char*>(&err), &len) != 0
			    || err != 0)
			{
				close_socket(s);
				return kInvalidSocket;
			}
		}
		return s;
	}

	// --------------------------------------------------------------------- TLS

	// What a socket operation is waiting for, so the poll loop can ask for the right
	// event. Cleartext only ever wants to read or to write the thing it was already
	// doing; TLS can want the opposite of it, because a read can trigger a renegotiation
	// write and a write can need a read. Polling for the wrong one is how a TLS client
	// on a non-blocking socket deadlocks.
	enum class Want : std::uint8_t
	{
		Nothing,
		Read,
		Write
	};

	// Which interface the kernel actually sent the load over, and what that interface is.
	//
	// The arrangement is asserted by a route metric and nothing else. Where a host has
	// more than one interface on the same subnet, which of them carries a run is decided
	// by a number DHCP can change at a lease renewal, and a run that moved from a wired
	// link to a wireless one would still produce a plausible figure with nothing in the
	// record to say the medium had changed underneath it.
	//
	// So this is read rather than configured. getsockname on an established connection is
	// the address the kernel chose as a source, which is ground truth about the path;
	// a configured label records only what somebody meant to happen. Same reason
	// affinity_applied and euid exist beside their requested counterparts.
	//
	// The name and the link's properties, never the address or the MAC. The address is
	// used here to find the interface and is then discarded: these records reach a
	// repository, and an interface name identifies a wire while an address identifies a
	// machine on somebody's network.
	struct PathInfo
	{
		std::atomic<bool> described{false};
		std::string interface;   // e.g. "enp2s0", or "lo" for a loopback run
		std::string speed_mbit;  // as the driver reports it; empty where it has no notion
		std::string duplex;
		std::string mtu;
	};

	// Reads /sys/class/net/<iface>/<name>, trimmed. Empty when the file is absent or
	// unreadable, which is the ordinary case for several of these on virtual interfaces:
	// a veth has no speed and a loopback has no duplex, and neither is an error.
	std::string read_net_attr(const std::string& iface, const char* name)
	{
#if defined(__linux__)
		std::ifstream in("/sys/class/net/" + iface + "/" + name);
		std::string value;
		if (!in || !std::getline(in, value))
		{
			return {};
		}
		while (!value.empty() && (value.back() == '\n' || value.back() == '\r'))
		{
			value.pop_back();
		}
		// The driver reports -1 for a link with no negotiated speed, which is not a
		// number worth recording as though it were one.
		if (value == "-1")
		{
			return {};
		}
		return value;
#else
		(void)iface;
		(void)name;
		return {};
#endif
	}

	// Fills `path` from an established socket, once per run.
	//
	// Linux only, and silent elsewhere rather than an error: macOS and Windows have no
	// /sys and the field is simply absent there, the same way euid is. A failure to
	// identify the interface is also silent here, because this function does not know
	// whether that matters; the harness decides, and it can tell an empty field from a
	// wrong one.
	void describe_path(PathInfo& path, socket_t s)
	{
#if defined(__linux__)
		bool expected = false;
		if (!path.described.compare_exchange_strong(expected, true))
		{
			return;
		}

		sockaddr_in local{};
		socklen_t len = sizeof(local);
		if (::getsockname(s, reinterpret_cast<sockaddr*>(&local), &len) != 0)
		{
			return;
		}

		// Match the source address the kernel chose against the machine's interfaces to
		// get a name. The address itself goes no further than this function.
		ifaddrs* ifa = nullptr;
		if (::getifaddrs(&ifa) != 0)
		{
			return;
		}
		for (ifaddrs* it = ifa; it != nullptr; it = it->ifa_next)
		{
			if (it->ifa_addr == nullptr || it->ifa_addr->sa_family != AF_INET)
			{
				continue;
			}
			const auto* addr = reinterpret_cast<const sockaddr_in*>(it->ifa_addr);
			if (addr->sin_addr.s_addr == local.sin_addr.s_addr && it->ifa_name != nullptr)
			{
				path.interface = it->ifa_name;
				break;
			}
		}
		::freeifaddrs(ifa);

		if (!path.interface.empty())
		{
			path.speed_mbit = read_net_attr(path.interface, "speed");
			path.duplex = read_net_attr(path.interface, "duplex");
			path.mtu = read_net_attr(path.interface, "mtu");
		}
#else
		(void)path;
		(void)s;
#endif
	}

	struct TlsClient
	{
#ifdef COROUTE_LOADGEN_TLS
		SSL_CTX* ctx = nullptr;
#endif
		// Read out of the first established connection and reported, so the record says
		// which protocol version and cipher the numbers describe rather than leaving a
		// reader to assume the default of whatever OpenSSL was linked.
		std::atomic<bool> described{false};
		std::string version;
		std::string cipher;
	};

#ifdef COROUTE_LOADGEN_TLS
	bool tls_init(TlsClient& tls)
	{
		tls.ctx = SSL_CTX_new(TLS_client_method());
		if (!tls.ctx)
		{
			return false;
		}
		// No verification, deliberately. The rig serves a self-signed certificate, so a
		// verifying client would fail every connection; pointing it at a trust store
		// instead would put certificate chain validation into a measurement that is
		// about the first octet of a connection. Stated as a limitation with the
		// results.
		SSL_CTX_set_verify(tls.ctx, SSL_VERIFY_NONE, nullptr);
		// Session resumption off. With it on the second and later connections skip the
		// full handshake, so a connection-establishment distribution would be a mixture
		// of two populations whose proportions depend on how the run happened to be
		// timed. Every connection here pays for a full handshake, which is the
		// conservative case and the reproducible one.
		SSL_CTX_set_session_cache_mode(tls.ctx, SSL_SESS_CACHE_OFF);
		SSL_CTX_set_options(tls.ctx, SSL_OP_NO_TICKET);
		SSL_CTX_set_min_proto_version(tls.ctx, TLS1_2_VERSION);
		return true;
	}

	void tls_describe(TlsClient& tls, SSL* ssl)
	{
		bool expected = false;
		if (!tls.described.compare_exchange_strong(expected, true))
		{
			return;
		}
		const char* version = SSL_get_version(ssl);
		const char* cipher = SSL_get_cipher_name(ssl);
		tls.version = version ? version : "";
		tls.cipher = cipher ? cipher : "";
	}
#endif

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
		std::size_t request_index = 0;  // which prebuilt request is in flight
		// When connect was called on this socket, and whether the connection is usable
		// yet. On the cleartext arm it becomes usable the moment connect returns; on the
		// TLS arm not until the handshake finishes, and until then it must not be given
		// a request to send.
		Clock::time_point opened{};
		bool ready = false;
		Want want = Want::Nothing;
#ifdef COROUTE_LOADGEN_TLS
		SSL* ssl = nullptr;
#endif
	};

	// ------------------------------------------------- transport, one or other

	// Everything below reads "if there is an SSL object, go through it, otherwise go
	// through the socket". Written as free functions over Conn rather than as a virtual
	// interface: a virtual call per read at 70 thousand requests a second is a cost the
	// cleartext arm does not have today, and adding one to both arms to measure a
	// difference between them is the kind of thing that makes a null result meaningless.

	void conn_close(Conn& c)
	{
#ifdef COROUTE_LOADGEN_TLS
		if (c.ssl)
		{
			// No SSL_shutdown. A close_notify costs a round trip per connection, and on
			// the churn arm that would add a measured cost to the TLS arm that belongs
			// to the client's politeness rather than to the server's dispatch.
			SSL_free(c.ssl);
			c.ssl = nullptr;
		}
#endif
		if (c.fd != kInvalidSocket)
		{
			close_socket(c.fd);
			c.fd = kInvalidSocket;
		}
		c.ready = false;
		c.want = Want::Nothing;
	}

#ifdef COROUTE_LOADGEN_TLS
	// What a failed OpenSSL call meant. Told apart once, here, because SSL_get_error has
	// to be called immediately after the operation it describes and calling it twice for
	// one failure is how a want comes to be read as a fault.
	enum class TlsOutcome : std::uint8_t
	{
		Want,        // needs the socket readable or writable; c.want says which
		ClosedClean, // the peer is done, whether or not it said close_notify
		Fault        // a protocol or system error; the connection is not usable
	};

	TlsOutcome tls_classify(SSL* ssl, int rc, Want& want)
	{
		const int err = SSL_get_error(ssl, rc);
		switch (err)
		{
			case SSL_ERROR_WANT_READ:
				want = Want::Read;
				return TlsOutcome::Want;
			case SSL_ERROR_WANT_WRITE:
				want = Want::Write;
				return TlsOutcome::Want;
			case SSL_ERROR_ZERO_RETURN:
				// close_notify. The polite close.
				return TlsOutcome::ClosedClean;
			case SSL_ERROR_SYSCALL:
				// rc of zero with an empty error queue is end of file without a
				// close_notify, which is how OpenSSL 1.1.1 reported it.
				if (rc == 0 && ERR_peek_error() == 0)
				{
					return TlsOutcome::ClosedClean;
				}
				ERR_clear_error();
				return TlsOutcome::Fault;
			default:
			{
#ifdef SSL_R_UNEXPECTED_EOF_WHILE_READING
				// OpenSSL 3.0 moved the same event here. A peer that closes the TCP
				// connection without sending close_notify is now a protocol error with
				// this reason rather than a syscall condition.
				//
				// It has to be treated as a clean close, and the reason is the whole
				// churn experiment: a server enforcing a per-connection request limit
				// closes exactly this way, so counting it as a fault made every
				// connection an error, every run inadmissible, and the measurement that
				// carries the paper impossible to take. This is what that looked like
				// before it was found: eight connections, eight socket errors, no
				// completed requests.
				//
				// The distinction being given up is truncation detection, which matters
				// to a client that must not act on a short response and does not matter
				// to one that counts them. Every other SSL error is still a fault.
				const unsigned long queued = ERR_peek_error();
				if (ERR_GET_REASON(queued) == SSL_R_UNEXPECTED_EOF_WHILE_READING)
				{
					ERR_clear_error();
					return TlsOutcome::ClosedClean;
				}
#endif
				// Consumed so a stale entry does not attach itself to a later,
				// unrelated failure on this thread.
				ERR_clear_error();
				return TlsOutcome::Fault;
			}
		}
	}
#endif

	// Starts a connection and, on the TLS arm, the handshake with it. Returns false when
	// the socket could not be opened at all; a handshake still in flight is a true with
	// c.ready false.
	bool conn_open(Conn& c, const sockaddr_in& addr, const Options& opt, TlsClient& tls)
	{
		c.scanner.reset();
		c.awaiting = false;
		c.sent_offset = 0;
		c.opened = Clock::now();
		c.fd = connect_to(addr);
		if (c.fd == kInvalidSocket)
		{
			c.ready = false;
			return false;
		}

		if (!opt.tls)
		{
			// connect_to blocks until the TCP handshake completes, so the connection is
			// usable the instant it returns and the establishment time is already known.
			c.ready = true;
			c.want = Want::Nothing;
			return true;
		}

#ifdef COROUTE_LOADGEN_TLS
		c.ssl = SSL_new(tls.ctx);
		if (!c.ssl)
		{
			conn_close(c);
			return false;
		}
		if (SSL_set_fd(c.ssl, static_cast<int>(c.fd)) != 1)
		{
			conn_close(c);
			return false;
		}
		if (!opt.sni.empty())
		{
			// Ignored when the name is a literal address, which is what these campaigns
			// use: RFC 6066 forbids sending one, and OpenSSL refuses to.
			SSL_set_tlsext_host_name(c.ssl, opt.sni.c_str());
		}
		SSL_set_connect_state(c.ssl);
		c.ready = false;
		// Nothing is driven here. The first SSL_do_handshake happens in the poll loop,
		// so a run whose connections all start at once does not serialise 64 handshakes
		// on one thread before the clock starts.
		c.want = Want::Write;
		return true;
#else
		(void)tls;
		conn_close(c);
		return false;
#endif
	}

	// Pushes the handshake forward by whatever it can do now. Returns false on a fault.
	bool conn_advance_handshake(Conn& c, TlsClient& tls)
	{
#ifdef COROUTE_LOADGEN_TLS
		const int rc = SSL_do_handshake(c.ssl);
		if (rc == 1)
		{
			c.ready = true;
			c.want = Want::Nothing;
			tls_describe(tls, c.ssl);
			return true;
		}
		c.want = Want::Nothing;
		// A peer that closed mid-handshake is a failed handshake here and not a clean
		// close: nothing was established, so there is nothing to have closed.
		return tls_classify(c.ssl, rc, c.want) == TlsOutcome::Want;
#else
		(void)c;
		(void)tls;
		return false;
#endif
	}

	// Both return the byte count, 0 for a clean close by the peer, and -1 for either a
	// want or a fault. The two are told apart by c.want, which a want sets and a fault
	// leaves as Nothing.
	int conn_send(Conn& c, const char* data, int len)
	{
#ifdef COROUTE_LOADGEN_TLS
		if (c.ssl)
		{
			const int rc = SSL_write(c.ssl, data, len);
			if (rc > 0)
			{
				c.want = Want::Nothing;
				return rc;
			}
			c.want = Want::Nothing;
			// Either way this returns -1; what differs is whether c.want was set, which
			// is how the caller tells a retry from a fault.
			tls_classify(c.ssl, rc, c.want);
			return -1;
		}
#endif
		const int n = ::send(c.fd, data, len, 0);
		c.want = (n < 0 && would_block()) ? Want::Write : Want::Nothing;
		return n;
	}

	int conn_recv(Conn& c, char* data, int len)
	{
#ifdef COROUTE_LOADGEN_TLS
		if (c.ssl)
		{
			const int rc = SSL_read(c.ssl, data, len);
			if (rc > 0)
			{
				c.want = Want::Nothing;
				return rc;
			}
			c.want = Want::Nothing;
			switch (tls_classify(c.ssl, rc, c.want))
			{
				case TlsOutcome::Want:
					return -1;  // c.want says which event to wait for
				case TlsOutcome::ClosedClean:
					return 0;   // the caller reconnects and counts a server close
				case TlsOutcome::Fault:
				default:
					return -1;  // c.want is Nothing, so the caller counts an error
			}
		}
#endif
		const int n = ::recv(c.fd, data, len, 0);
		c.want = (n < 0 && would_block()) ? Want::Read : Want::Nothing;
		return n;
	}

	// Whether more application data is already decrypted and waiting inside OpenSSL.
	// poll cannot see it: the bytes have left the socket. Without this check a response
	// that arrived in the same record as the previous one waits for the next packet.
	bool conn_has_buffered(const Conn& c)
	{
#ifdef COROUTE_LOADGEN_TLS
		return c.ssl != nullptr && SSL_pending(c.ssl) > 0;
#else
		(void)c;
		return false;
#endif
	}

	// ------------------------------------------------------------------ worker

	// One thread drives a slice of the connections with poll. Not one thread per
	// connection: at a few hundred connections the scheduler noise from that would be
	// larger than the differences being measured.
	void worker(const Options& opt, const sockaddr_in& addr, const std::vector<std::string>& requests,
	            std::size_t first_index, std::size_t count, double rate_per_thread, Clock::time_point start,
	            Clock::time_point warmup_end, Clock::time_point stop, TlsClient& tls,
	            PathInfo& path, ThreadResult& out)
	{
		std::vector<Conn> conns(count);
		std::vector<poll_fd> pfds(count);

		// Establishment samples are filtered on when the connection was opened, on the
		// same rule as the latency samples: the connections made before the warmup ended
		// paid for a cold allocator and, on the TLS arm, for the first use of every
		// cipher implementation OpenSSL had not yet touched.
		auto note_established = [&out, warmup_end, &path](const Conn& c)
		{
			++out.established;
			// Once per run, from a socket the kernel has actually connected, so the
			// source address is the one it chose rather than one we asked for.
			describe_path(path, c.fd);
			if (c.opened < warmup_end)
			{
				return;
			}
			const auto us =
			    std::chrono::duration_cast<std::chrono::microseconds>(Clock::now() - c.opened).count();
			out.connect_us.push_back(static_cast<std::uint32_t>(us < 0 ? 0 : us));
		};

		auto reopen = [&](Conn& c)
		{
			conn_close(c);
			if (!conn_open(c, addr, opt, tls))
			{
				++out.socket_errors;
				return;
			}
			if (c.ready)
			{
				note_established(c);
			}
		};

		for (std::size_t i = 0; i < conns.size(); ++i)
		{
			// Each connection starts at a different point in the path list, so the
			// connections do not march through the table in lockstep and hammer the
			// same route at the same instant.
			conns[i].request_index = (first_index + i) % requests.size();
			if (!conn_open(conns[i], addr, opt, tls))
			{
				++out.socket_errors;
			}
			else if (conns[i].ready)
			{
				note_established(conns[i]);
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

			// The schedule for the measured window starts at the end of the warmup, not
			// at the start of the run.
			//
			// next_due only advances when a connection is free to take a slot, so any
			// shortfall during the warmup is carried forward as debt. On the cleartext
			// arm there is none: every connection is usable the moment connect returns.
			// On the TLS arm all sixty-four connections handshake at once at t=0, and
			// until they finish there is nothing to issue on, so the loop arrives at the
			// measured window already seconds behind a schedule it was never able to
			// keep.
			//
			// Carrying that debt would charge the measurement for the warmup, which is
			// the one thing a warmup exists to prevent. Measured before this was fixed:
			// a TLS run at a thousand requests a second reported a pacing lag of 14 ms
			// at p99 and was refused, on a server that was delivering 99.7 percent of
			// the offered rate.
			//
			// Debt accrued after this point is real and is still reported: the reset
			// happens once, at the boundary.
			if (open_loop && next_due < warmup_end && now >= warmup_end)
			{
				next_due = warmup_end;
			}

			// Issue whatever is due.
			for (std::size_t i = 0; i < conns.size(); ++i)
			{
				Conn& c = conns[i];
				// Not ready covers a handshake still in flight. A connection that cannot
				// send yet must not be given a due slot, or the slot's latency would be
				// the handshake's.
				if (c.fd == kInvalidSocket || !c.ready || c.awaiting)
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
				// Advanced when the request is issued rather than when it completes, so
				// a connection that stalls does not keep re-sending the same path.
				c.request_index = (c.request_index + 1) % requests.size();
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
				if (!c.ready)
				{
					// A handshake in flight waits for exactly what OpenSSL last asked
					// for. Asking for the other one is how a non-blocking TLS client
					// stops making progress without ever reporting an error.
					pfds[i].events |= (c.want == Want::Write) ? POLLOUT : POLLIN;
					continue;
				}
				if (c.awaiting && c.sent_offset < requests[c.request_index].size())
				{
					// A TLS write can need the socket readable, so the pending want wins
					// over the direction the application is going in.
					pfds[i].events |= (c.want == Want::Read) ? POLLIN : POLLOUT;
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
					// The threshold is generous because the failure is not the length of
					// the sleep, it is the overshoot. A one millisecond wait is quantised
					// to the system tick, and even with the tick raised to a millisecond
					// it lands late under load. At the rates the sustained campaigns use
					// the next slot is always within this window, so those runs spin as
					// they always did and are unaffected.
					//
					// The connection-establishment design is what makes this matter. Its
					// rates are necessarily low, because it offers whole connections
					// rather than requests, and at a low rate the old bound slept. A TLS
					// establishment run at a thousand a second reported a pacing lag of
					// 14 ms at p99, which is a scheduler tick and not a property of
					// anything being measured, and it was refused for it.
					constexpr std::int64_t kSpinBelowUs = 20000;
					timeout_ms = us < kSpinBelowUs ? 0 : 1;
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
					reopen(c);
					++out.socket_errors;
					continue;
				}

				if (!c.ready)
				{
					// Nothing arrived for this one yet. POLLHUP is included because a
					// server that refused the handshake shows up here and not as an
					// error, and leaving it would spin on a dead socket until the run
					// ended.
					if ((pfds[i].revents & (POLLIN | POLLOUT | POLLHUP)) == 0)
					{
						continue;
					}
					if (!conn_advance_handshake(c, tls))
					{
						++out.handshake_failures;
						reopen(c);
						continue;
					}
					if (!c.ready)
					{
						continue;  // more of the handshake still to come
					}
					note_established(c);
					// Deliberately no request this iteration. The issue loop at the top
					// owns when a request becomes due, and sending one here would put a
					// request outside the open loop's schedule.
					continue;
				}

				const std::string& request = requests[c.request_index];
				// Writable, or readable while the last write said it needed a read. The
				// second case is TLS only: conn_send never leaves a cleartext connection
				// wanting a read, so the cleartext arm makes exactly the send calls it
				// made before TLS existed.
				const bool can_send = (pfds[i].revents & POLLOUT) != 0 ||
				                      (c.want == Want::Read && (pfds[i].revents & POLLIN) != 0);
				if (can_send && c.awaiting && c.sent_offset < request.size())
				{
					const int n = conn_send(c, request.data() + c.sent_offset,
					                        static_cast<int>(request.size() - c.sent_offset));
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
					else if (n < 0 && c.want == Want::Nothing)
					{
						conn_close(c);
						++out.socket_errors;
						continue;
					}
				}

				// SSL_pending is checked as well as poll, because bytes that OpenSSL has
				// already decrypted have left the socket and poll will never mention them
				// again. Two responses in one TLS record is the ordinary case at these
				// rates, and without this the second waits for the next packet to arrive.
				if ((pfds[i].revents & POLLIN) != 0 || conn_has_buffered(c))
				{
					const int n = conn_recv(c, buf, sizeof(buf));
					if (n > 0)
					{
						// Under the same filter as `completed` below, so bytes per second
						// and requests per second describe one window. Unfiltered, the
						// warmup's bytes were divided by the measured wall.
						if (c.issued >= warmup_end)
						{
							out.bytes_read += static_cast<std::uint64_t>(n);
						}
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
							out.responses_total += done;
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

							// One request per connection: this side closes as soon as
							// the response is in, rather than waiting to be closed and
							// racing the next request against it.
							if (opt.reconnect)
							{
								reopen(c);
								continue;
							}
						}
					}
					else if (n == 0)
					{
						// Clean close by the server. Reconnect and carry on; an outstanding request
						// is reissued rather than counted as completed.
						reopen(c);
						++out.server_closes;
					}
					else if (c.want == Want::Nothing)
					{
						conn_close(c);
						++out.socket_errors;
					}
				}
			}
		}

		for (auto& c : conns)
		{
			conn_close(c);
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
		    "  --paths FILE        request paths, one per line, drawn in turn instead of\n"
		    "                      --path; for the routing experiment, where a single hot\n"
		    "                      path would measure one route rather than a table\n"
		    "  --connections N     total connections, default 64\n"
		    "  --threads N         default 4\n"
		    "  --duration S        measured seconds, default 10\n"
		    "  --warmup S          discarded seconds before measuring, default 2\n"
		    "  --rate N            open loop at N requests per second; 0 (default) is\n"
		    "                      a closed loop, which measures service time\n"
		    "  --out FILE          write the result as JSON\n"
		    "  --samples FILE      write every latency sample, one per line, in us\n"
		    "  --affinity HEX      confine the generator to this hexadecimal CPU mask, so\n"
		    "                      it does not compete with the server for cores; has no\n"
		    "                      effect on macOS, which has no process affinity API\n"
		    "  --tls               speak TLS; the server side is --tls CERT KEY, and with\n"
		    "                      classification on that is the same port as cleartext\n"
		    "  --sni NAME          server name to send; omitted for a literal address,\n"
		    "                      which is what RFC 6066 requires\n"
		    "  --reconnect         one request per connection, closed by this side as soon\n"
		    "                      as the response is in, with Connection: close on the\n"
		    "                      request. This is the establishment design, and\n"
		    "                      connect_us is the measurement it exists to take\n"
		    "\n"
		    "Certificate verification is off. The rig serves a self-signed certificate and\n"
		    "a verifying client would measure a trust store, so the TLS numbers describe a\n"
		    "handshake without chain validation and say so in the record.\n"
		    "\n"
		    "connect_us is reported apart from latency_us and answers a different question.\n"
		    "Classification happens once per connection, so across a keep-alive connection\n"
		    "serving a hundred thousand requests its cost is divided by a hundred thousand.\n"
		    "Drive the server with --max-requests 1 to make establishment the measurement.\n"
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
		else if (a == "--paths") opt.paths_file = next("--paths");
		else if (a == "--connections") opt.connections = static_cast<std::size_t>(std::stoul(next("--connections")));
		else if (a == "--threads") opt.threads = static_cast<std::size_t>(std::stoul(next("--threads")));
		else if (a == "--duration") opt.duration_s = std::stod(next("--duration"));
		else if (a == "--warmup") opt.warmup_s = std::stod(next("--warmup"));
		else if (a == "--rate") opt.rate = std::stod(next("--rate"));
		else if (a == "--out") opt.out_path = next("--out");
		else if (a == "--samples") opt.samples_path = next("--samples");
		else if (a == "--affinity") opt.affinity_mask = std::strtoull(next("--affinity").c_str(), nullptr, 16);
		else if (a == "--tls") opt.tls = true;
		else if (a == "--sni") opt.sni = next("--sni");
		else if (a == "--reconnect") opt.reconnect = true;
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
#else
	// Writing to a connection the server has already closed is ordinary here, not a
	// fault: the server enforces a per-connection request limit, and the close can land
	// between the poll and the send, which is the same path the server_closes counter
	// treats as normal. The default disposition of SIGPIPE would end the process on that
	// path, before it writes its result file, so a routine keep-alive close would be
	// reported as a failed run and abort the campaign.
	::signal(SIGPIPE, SIG_IGN);
#endif

	sockaddr_in addr{};
	addr.sin_family = AF_INET;
	// Unqualified, not ::htons. On Darwin htons is a macro expanding to a parenthesised
	// cast, so the global-scope qualifier would apply to an expression rather than to a
	// name and the file would not compile. Plain htons is correct on all three: a
	// function on Windows and glibc, and the macro where there is one.
	addr.sin_port = htons(opt.port);
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

	// Every request built once, before the clock starts. Formatting a request inside
	// the send loop would put string construction in the pacing path, and an open loop
	// that is late because it was building a string reports that lateness as latency.
	std::vector<std::string> requests;
	{
		std::vector<std::string> paths;
		if (!opt.paths_file.empty())
		{
			std::ifstream in(opt.paths_file);
			if (!in)
			{
				std::fprintf(stderr, "could not read --paths %s\n", opt.paths_file.c_str());
				return 2;
			}
			std::string line;
			while (std::getline(in, line))
			{
				if (!line.empty() && line.back() == '\r') line.pop_back();
				if (!line.empty()) paths.push_back(line);
			}
			if (paths.empty())
			{
				std::fprintf(stderr, "--paths %s held no paths\n", opt.paths_file.c_str());
				return 2;
			}
		}
		else
		{
			paths.push_back(opt.path);
		}

		requests.reserve(paths.size());
		for (const auto& p : paths)
		{
			requests.push_back("GET " + p + " HTTP/1.1\r\nHost: " + opt.host +
			                   (opt.reconnect ? "\r\nConnection: close"
			                                  : "\r\nConnection: keep-alive") +
			                   "\r\nUser-Agent: coroute-loadgen\r\n\r\n");
		}
	}

	// Refused rather than downgraded. A generator that quietly connected in cleartext
	// because it had no TLS would produce a full set of plausible numbers for an
	// experiment that never happened, and the record would say tls=true.
	TlsClient tls;
	if (opt.tls)
	{
#ifdef COROUTE_LOADGEN_TLS
		// --sni is not defaulted to --host. The host in these campaigns is a literal
		// address, and RFC 6066 forbids sending one as a server name, so defaulting it
		// would put an extension on the wire that a real client would not send.
		if (!tls_init(tls))
		{
			std::fprintf(stderr, "could not create a TLS client context\n");
			return 2;
		}
#else
		std::fprintf(stderr, "--tls was requested but this generator was built without OpenSSL\n");
		return 2;
#endif
	}

	const auto start = Clock::now();
	const auto warmup_end = start + std::chrono::duration_cast<Clock::duration>(
	                                    std::chrono::duration<double>(opt.warmup_s));
	const auto stop = warmup_end + std::chrono::duration_cast<Clock::duration>(
	                                   std::chrono::duration<double>(opt.duration_s));


	std::vector<ThreadResult> results(opt.threads);
	std::vector<std::thread> pool;
	pool.reserve(opt.threads);

	// Shared across threads and filled once, by whichever connection establishes first.
	PathInfo path;

	const std::size_t per_thread = opt.connections / opt.threads;
	std::size_t remainder = opt.connections % opt.threads;
	const double rate_per_thread = opt.rate > 0.0 ? opt.rate / static_cast<double>(opt.threads) : 0.0;

	std::size_t next_start = 0;
	for (std::size_t t = 0; t < opt.threads; ++t)
	{
		const std::size_t mine = per_thread + (remainder > 0 ? 1 : 0);
		if (remainder > 0) --remainder;
		pool.emplace_back(worker, std::cref(opt), std::cref(addr), std::cref(requests), next_start, mine,
		                  rate_per_thread, start, warmup_end, stop, std::ref(tls), std::ref(path),
		                  std::ref(results[t]));
		next_start += mine;
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
	std::uint64_t responses_total = 0, established = 0, handshake_failures = 0;
	std::vector<std::uint32_t> all;
	std::vector<std::uint32_t> pacing;
	std::vector<std::uint32_t> connects;
	for (const auto& r : results)
	{
		completed += r.completed;
		responses_total += r.responses_total;
		non_2xx += r.non_2xx;
		sock_errors += r.socket_errors;
		closes += r.server_closes;
		bytes += r.bytes_read;
		established += r.established;
		handshake_failures += r.handshake_failures;
		all.insert(all.end(), r.latencies_us.begin(), r.latencies_us.end());
		pacing.insert(pacing.end(), r.pacing_us.begin(), r.pacing_us.end());
		connects.insert(connects.end(), r.connect_us.begin(), r.connect_us.end());
	}
	std::sort(all.begin(), all.end());
	std::sort(pacing.begin(), pacing.end());
	std::sort(connects.begin(), connects.end());

	const double cpu_used = (cpu_before >= 0.0 && cpu_after >= 0.0) ? (cpu_after - cpu_before) : -1.0;
	// Against all cores the generator could have used, which is how the validity rule
	// is stated: a generator pinned at its own thread count is saturated.
	// Against the cores the generator was actually allowed to use, not against its
	// thread count. With an affinity mask narrower than the thread count the two differ,
	// and dividing by threads reports a generator as idle when it is in fact pinned flat
	// against the cores it was given.
	// On macOS there is no affinity API and apply_affinity refuses, so the process is
	// spread over every core the scheduler chooses. Dividing by the bits of a mask the
	// process was never confined to would inflate the reported fraction and could refuse
	// a run that was in fact idle, so the narrowing needs the mask to have been applied.
	std::size_t usable_cores = opt.threads;
	if (opt.affinity_mask != 0 && affinity_applied)
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
		// Escaped, because one of these fields is a Windows path and a backslash in a
		// JSON string is not a backslash. The record is only useful if it parses.
		for (const char c : value)
		{
			if (quote && (c == '\\' || c == '"')) json += '\\';
			json += c;
		}
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
	field_s("paths_file", opt.paths_file);
	field_u("distinct_paths", requests.size());
	field_u("connections", opt.connections);
	field_u("threads", opt.threads);
	field_u("usable_cores", usable_cores);
	field_s("affinity_mask", mask_hex);
	field_s("affinity_applied", affinity_applied ? "true" : "false", false);
#if !defined(_WIN32)
	// Which user actually ran the load, for the same reason affinity_applied exists:
	// asking and getting are two different things. The namespace arrangement launches
	// the generator through a prefix that is supposed to drop back to the invoking user
	// while the server stays root, and that prefix is a free-form string nothing
	// inspects. An operator who omits the runuser gets a root generator and a record
	// that reads exactly like a correct run. Absent on Windows, which has no such id.
	field_u("euid", static_cast<unsigned long long>(geteuid()));
#endif
	// The path the kernel chose, not the one that was configured. Empty off Linux and on
	// a run where the interface could not be identified; the harness distinguishes an
	// empty field from a wrong one and decides what to do about it. Never an address or
	// a MAC: the name identifies a wire, an address identifies a machine.
	field_s("local_interface", path.interface);
	field_s("local_interface_speed_mbit", path.speed_mbit);
	field_s("local_interface_duplex", path.duplex);
	field_s("local_interface_mtu", path.mtu);
	field_f("offered_rate", opt.rate, 3);
	field_f("duration_s", wall, 6);
	field_u("completed", completed);
	field_u("responses_total", responses_total);
	field_u("non_2xx", non_2xx);
	field_u("socket_errors", sock_errors);
	field_u("server_closes", closes);
	field_u("bytes_read", bytes);
	field_s("tls", opt.tls ? "true" : "false", false);
	field_s("tls_version", tls.version);
	field_s("tls_cipher", tls.cipher);
	field_s("tls_verify", opt.tls ? "none" : "");
	field_s("reconnect", opt.reconnect ? "true" : "false", false);
	field_u("connections_established", established);
	field_u("handshake_failures", handshake_failures);
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
	json += "  },\n";

	// Empty on a keep-alive run whose connections were all made during the warmup,
	// which is the ordinary case and is why the samples count is reported next to the
	// percentiles rather than left to be inferred from them.
	json += "  \"connect_us\": {\n";
	json += "    \"samples\": " + std::to_string(connects.size()) + ",\n";
	pct("min", connects.empty() ? 0U : connects.front());
	pct("p50", percentile(connects, 0.50));
	pct("p90", percentile(connects, 0.90));
	pct("p99", percentile(connects, 0.99));
	pct("p999", percentile(connects, 0.999));
	pct("max", connects.empty() ? 0U : connects.back(), true);
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
