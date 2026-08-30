#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <functional>
#include <chrono>
#include <string_view>

#include "coroute/util/expected.hpp"
#include "coroute/core/error.hpp"
#include "coroute/coro/task.hpp"
#include "coroute/coro/cancellation.hpp"

namespace coroute::net
{

	// Forward declarations
	class Socket;
	class Connection;
	struct IoStatsBlock;

	// ============================================================================
	// IoBackend - selected at runtime, never by compiling a different binary
	// ============================================================================
	//
	// Build-to-build variation from code layout and link order is documented at 5 to 10
	// percent, above the effect an epoll-versus-io_uring comparison is trying to resolve.
	// Both Linux backends therefore ship in one binary and this enum is how a run picks.

	enum class IoBackend : std::uint8_t
	{
		Default,  // platform default: io_uring on Linux, IOCP on Windows, kqueue on macOS
		Epoll,
		IoUring,
		Iocp,
		Kqueue
	};

	[[nodiscard]] inline const char* io_backend_name(IoBackend backend) noexcept
	{
		switch (backend)
		{
			case IoBackend::Epoll: return "epoll";
			case IoBackend::IoUring: return "io_uring";
			case IoBackend::Iocp: return "iocp";
			case IoBackend::Kqueue: return "kqueue";
			case IoBackend::Default: return "default";
		}
		return "unknown";
	}

	[[nodiscard]] inline bool parse_io_backend(std::string_view text, IoBackend& out) noexcept
	{
		if (text == "epoll")
		{
			out = IoBackend::Epoll;
			return true;
		}
		if (text == "io_uring" || text == "uring")
		{
			out = IoBackend::IoUring;
			return true;
		}
		if (text == "iocp")
		{
			out = IoBackend::Iocp;
			return true;
		}
		if (text == "kqueue")
		{
			out = IoBackend::Kqueue;
			return true;
		}
		if (text == "default")
		{
			out = IoBackend::Default;
			return true;
		}
		return false;
	}

	[[nodiscard]] inline IoBackend platform_default_io_backend() noexcept
	{
#if defined(COROUTE_PLATFORM_WINDOWS)
		return IoBackend::Iocp;
#elif defined(COROUTE_PLATFORM_MACOS)
		return IoBackend::Kqueue;
#elif defined(COROUTE_DEFAULT_BACKEND_EPOLL)
		return IoBackend::Epoll;
#else
		return IoBackend::IoUring;
#endif
	}

	// ============================================================================
	// IoContext - Abstract I/O event loop
	// ============================================================================

	// Connection handler callback for multi-accept
	using ConnectionHandler = std::function<void(std::unique_ptr<Connection>)>;

	class IoContext
	{
	public:
		virtual ~IoContext() = default;

		// The backend this context is. Used by Listener::create / DatagramSocket::create
		// to pick the matching implementation when more than one is linked into the
		// binary, which on Linux is the normal case.
		[[nodiscard]] virtual std::string_view backend_name() const noexcept = 0;

		// Optional shared-memory counters for syscall-class accounting. Null-safe: a
		// backend that never receives a block simply does not count.
		virtual void bind_stats(IoStatsBlock* block) { (void)block; }

		// Whether IORING_OP_SEND_ZC is usable on this context. False everywhere except
		// an io_uring ring whose kernel and liburing both advertise the opcode.
		[[nodiscard]] virtual bool supports_send_zc() const noexcept { return false; }

		// Run the event loop (blocking)
		virtual void run() = 0;

		// Run one iteration of the event loop
		virtual void run_one() = 0;

		// Stop the event loop
		virtual void stop() = 0;

		// Check if stopped
		virtual bool stopped() const noexcept = 0;

		// Post a callback to be executed in the event loop
		virtual void post(std::function<void()> callback) = 0;

		// Schedule a callback after a delay
		virtual void schedule(std::chrono::milliseconds delay, std::function<void()> callback) = 0;

		// Number of worker threads this context will run.
		virtual size_t worker_count() const noexcept { return 1; }

		// Whether a callback can be directed at one specific worker thread.
		//
		// Asked once at setup rather than per operation, because the answer decides an
		// architecture rather than a detail. QUIC connection state is owned by exactly
		// one thread, since ngtcp2 is not reentrant, so a design that hands a
		// misdirected packet to the owning worker is only available on a backend that
		// can name that worker. Where this is false the caller has to keep everything on
		// one thread or take a lock, and both are worse than being told up front.
		//
		// True for backends where each worker has its own queue to poll (io_uring, one
		// ring per thread; epoll, one slot per thread). False where every worker pulls
		// from a shared completion queue and no thread has an identity, which is how
		// IOCP and kqueue are used here.
		[[nodiscard]] virtual bool supports_worker_affinity() const noexcept { return false; }

		// Runs `fn` on the given worker thread.
		//
		// The index is taken modulo worker_count(). Without affinity support this
		// degrades to post(), so the work still happens, just not anywhere in
		// particular: check supports_worker_affinity() first if that distinction
		// matters, and it usually does.
		virtual void run_on_worker(size_t index, std::function<void()> fn)
		{
			(void)index;
			post(std::move(fn));
		}

		// Accept on `port` across all workers, handing each connection to `handler`.
		// Returns false if the backend cannot do it, in which case the caller should
		// fall back to a single accept loop.
		//
		// How the work is spread differs per platform, and the difference is visible
		// in both the descriptor count and the locality of a connection:
		//
		//   Linux    SO_REUSEPORT, one listening socket per worker.   N descriptors.
		//            The kernel hashes the 4-tuple, so a connection is pinned to a
		//            worker for its whole life.
		//   macOS    One shared socket, one accept operation per worker. 1 descriptor.
		//            SO_REUSEPORT exists but does not load-balance the way Linux's
		//            does, so it is deliberately not used. FreeBSD has
		//            SO_REUSEPORT_LB, which does.
		//   Windows  One shared socket, a pool of concurrent AcceptEx operations.
		//            1 descriptor. IOCP wakes threads LIFO, so connections are not
		//            pinned to a worker.
		//
		// Windows and macOS therefore use fewer descriptors than Linux for the same
		// worker count, at the cost of weaker locality.
		virtual bool enable_multi_accept(uint16_t port, ConnectionHandler handler, int backlog = 1024)
		{
			(void)port;
			(void)handler;
			(void)backlog;
			return false;  // Default: not supported
		}

		// Check if multi-accept is enabled
		virtual bool is_multi_accept_enabled() const noexcept { return false; }

		// Factory method. On Linux both epoll and io_uring are linked; `backend` picks.
		static std::unique_ptr<IoContext> create(size_t thread_count = 1,
		                                        IoBackend backend = IoBackend::Default);
	};

	// ============================================================================
	// Async Operations - Awaitables for coroutines
	// ============================================================================

	// Result types for async operations
	using AcceptResult = expected<std::unique_ptr<Connection>, Error>;
	using ReadResult = expected<size_t, Error>;
	using WriteResult = expected<size_t, Error>;
	using ConnectResult = expected<void, Error>;
	using TransmitResult = expected<size_t, Error>;

// Platform-specific file handle type
#ifdef _WIN32
	using FileHandle = void*;  // HANDLE
#else
	using FileHandle = int;  // fd
#endif

	// ============================================================================
	// Listener - Accepts incoming connections
	// ============================================================================

	class Listener
	{
	public:
		virtual ~Listener() = default;

		// Start listening on the specified port
		virtual expected<void, Error> listen(uint16_t port, int backlog = 128) = 0;

		// Accept a connection (coroutine)
		virtual Task<AcceptResult> async_accept() = 0;

		// Close the listener
		virtual void close() = 0;

		// Check if listening
		virtual bool is_listening() const noexcept = 0;

		// Get local port
		virtual uint16_t local_port() const noexcept = 0;

		// Factory method
		static std::unique_ptr<Listener> create(IoContext& ctx);
	};

	// ============================================================================
	// start_accept_pool
	// ============================================================================
	//
	// Runs `depth` concurrent accept operations against one listener, handing each
	// connection to `handler`, until the context stops.
	//
	// Shared by the backends that cannot give each worker its own listening socket
	// (IOCP, kqueue). Both had the identical loop, and keeping it here means the logic
	// is compiled on every platform rather than only on the one being built.
	//
	// `listener` must outlive the context; backends keep it as a member.
	// `depth` above the worker count matters: a worker busy handling a connection
	// should not also be the reason a new one is waiting to be accepted.
	void start_accept_pool(IoContext& ctx, Listener& listener, const ConnectionHandler& handler, size_t depth);

	// ============================================================================
	// Connection - Async read/write on a socket
	// ============================================================================

	class Connection
	{
	public:
		virtual ~Connection() = default;

		// Async read into buffer
		virtual Task<ReadResult> async_read(void* buffer, size_t len) = 0;

		// Async read until delimiter or buffer full
		virtual Task<ReadResult> async_read_until(void* buffer, size_t len, char delimiter) = 0;

		// Async write from buffer
		virtual Task<WriteResult> async_write(const void* buffer, size_t len) = 0;

		// Async write all data (loops until complete)
		virtual Task<WriteResult> async_write_all(const void* buffer, size_t len) = 0;

		// Zero-copy send of an in-memory buffer (io_uring SEND_ZC). Default refuses.
		// Not the same as async_transmit_file: that copies from a file descriptor;
		// this maps a userspace buffer into the socket path without a byte copy.
		virtual Task<WriteResult> async_write_zero_copy(const void* buffer, size_t len);

		[[nodiscard]] virtual bool supports_zero_copy_send() const noexcept;

		// Zero-copy file transfer (TransmitFile on Windows, sendfile on Linux)
		// Returns bytes transmitted
		virtual Task<TransmitResult> async_transmit_file(FileHandle file, size_t offset, size_t length) = 0;

		// Close the connection
		virtual void close() = 0;

		// Check if connected
		virtual bool is_open() const noexcept = 0;

		// Set read/write timeout
		virtual void set_timeout(std::chrono::milliseconds timeout) = 0;

		// Get remote address (as string for now)
		virtual std::string remote_address() const = 0;

		// Get remote port
		virtual uint16_t remote_port() const noexcept = 0;

		// Set cancellation token for this connection
		virtual void set_cancellation_token(CancellationToken token) = 0;
	};

}  // namespace coroute::net
