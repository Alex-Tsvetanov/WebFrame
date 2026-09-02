#include "coroute/net/io_context.hpp"
#include "coroute/net/datagram.hpp"
#include "coroute/net/timer_queue.hpp"

#if defined(COROUTE_PLATFORM_MACOS)

#include <sys/event.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/uio.h>

#include <thread>
#include <vector>
#include <queue>
#include <mutex>
#include <atomic>
#include <algorithm>
#include <memory>
#include <cstring>
#include <stdexcept>
#include <string>
#include "coroute/net/socket_options.hpp"

namespace coroute::net
{

	// ============================================================================
	// kqueue Operation Types
	// ============================================================================

	enum class KqueueOpType
	{
		Accept,
		Read,
		Write,
		Connect
	};

	struct KqueueOperation
	{
		KqueueOpType type;
		std::coroutine_handle<> continuation;
		Error error;
		int result = 0;

		// For accept operations
		int accept_fd = -1;
		sockaddr_in client_addr{};
		socklen_t client_addr_len = sizeof(sockaddr_in);

		// For read/write operations
		void* buffer = nullptr;
		size_t length = 0;

		KqueueOperation(KqueueOpType t) : type(t) { }
	};

	// Registers the operation only once the continuation has been recorded.
	//
	// Registering first and awaiting second has a hole in it. Registration makes the
	// operation eligible immediately, and for a socket that already has data waiting,
	// which on loopback is the normal case rather than the rare one, kevent reports it
	// on another thread before the caller has reached the suspend point. The event loop
	// then finds a null continuation, and because the filter is registered EV_ONESHOT
	// that same delivery removes it. Nothing fires again and the coroutine waits
	// forever.
	//
	// Doing the registration from inside await_suspend closes the window: the
	// continuation is in place before the operation can possibly become eligible.
	//
	// await_suspend returns void on purpose. Registration cannot fail here, so there is
	// no result to report back, and returning nothing means no member of this awaiter
	// is read after the frame may already have been resumed and destroyed.
	template <typename Register>
	struct KqueueRegisterAwaiter
	{
		KqueueOperation& op;
		Register register_op;

		bool await_ready() const noexcept { return false; }

		void await_suspend(std::coroutine_handle<> h)
		{
			op.continuation = h;
			register_op();
		}

		void await_resume() const noexcept { }
	};

	template <typename Register>
	KqueueRegisterAwaiter(KqueueOperation&, Register) -> KqueueRegisterAwaiter<Register>;

	// ============================================================================
	// kqueue Context Implementation
	// ============================================================================

	class KqueueContext : public IoContext
	{
		int kq_ = -1;
		std::vector<std::thread> workers_;
		std::atomic<bool> stopped_{false};
		size_t thread_count_;

		std::mutex callback_mutex_;
		std::queue<std::function<void()>> callbacks_;

		// Multi-accept state: one shared listening socket, several concurrent accept
		// operations against it.
		std::unique_ptr<Listener> multi_listener_;
		bool multi_accept_ = false;

		// Thread-local buffer for batching kqueue registrations
		static thread_local std::vector<struct kevent> pending_changes_;

		// Whether this thread is one that drains the buffer above. Only a thread running
		// process_events() ever submits its own pending changes, so a registration made on
		// any other thread would sit in a buffer nobody reads and the filter would never be
		// armed. That is not hypothetical: enable_multi_accept() starts the accept pool on
		// the caller's thread, before run() has spawned a single worker.
		static thread_local bool in_event_loop_;

	public:
		explicit KqueueContext(size_t thread_count) : thread_count_(thread_count)
		{
			kq_ = kqueue();
			if (kq_ < 0)
			{
				throw std::runtime_error("kqueue() failed");
			}
		}

		~KqueueContext() override
		{
			// Before stop(): a timer firing into a half-torn-down loop has
			// nothing useful to do, and joining first means no callback is in
			// flight while the workers are being joined.
			timers_.stop();
			stop();

			for (auto& worker : workers_)
			{
				if (worker.joinable())
				{
					worker.join();
				}
			}

			if (kq_ >= 0)
			{
				close(kq_);
			}
		}

		int kq() const noexcept { return kq_; }

		// Batched on a worker, submitted immediately anywhere else.
		//
		// The batching is what lets process_events() carry its changelist in the same
		// kevent() call that waits for events, which is a syscall saved per operation on the
		// hot path and worth keeping. It is only safe for a thread that goes on to make that
		// call. Off the loop the change is submitted on its own rather than deferred to a
		// drain that never comes; those registrations are the bootstrap ones and are counted
		// in ones, not in millions.
		void submit_or_batch(const struct kevent& ev)
		{
			if (in_event_loop_)
			{
				pending_changes_.push_back(ev);
				return;
			}
			kevent(kq_, &ev, 1, nullptr, 0, nullptr);
		}

		// Register operation for a file descriptor using thread-local batching
		void register_read_op(int fd, KqueueOperation* op)
		{
			struct kevent ev;
			EV_SET(&ev, fd, EVFILT_READ, EV_ADD | EV_ONESHOT, 0, 0, op);
			submit_or_batch(ev);
		}

		void register_write_op(int fd, KqueueOperation* op)
		{
			struct kevent ev;
			EV_SET(&ev, fd, EVFILT_WRITE, EV_ADD | EV_ONESHOT, 0, 0, op);
			submit_or_batch(ev);
		}

		void register_accept_op(int fd, KqueueOperation* op)
		{
			struct kevent ev;
			EV_SET(&ev, fd, EVFILT_READ, EV_ADD | EV_ONESHOT, 0, 0, op);
			submit_or_batch(ev);
		}

		void run() override
		{
		// Deliberately does NOT reset stopped_.
		//
		// It used to, which made stop() racy: a stop arriving before run() had been
		// scheduled was erased here, and the workers then spun forever. Restarting a
		// stopped context is not a supported operation anywhere, so clearing the flag
		// bought nothing and cost a hang.

			for (size_t i = 0; i < thread_count_; ++i)
			{
				workers_.emplace_back([this] { worker_thread(); });
			}

			for (auto& worker : workers_)
			{
				if (worker.joinable())
				{
					worker.join();
				}
			}
		}

		void run_one() override
		{
			process_events();
			process_callbacks();
		}

		void stop() override { stopped_ = true; }

		bool stopped() const noexcept override { return stopped_; }

		size_t worker_count() const noexcept override { return thread_count_; }

		// Defined below KqueueListener, which this needs to construct.
		bool enable_multi_accept(uint16_t port, ConnectionHandler handler, int backlog) override;

		bool is_multi_accept_enabled() const noexcept override { return multi_accept_; }

		const char* backend_name() const noexcept override { return "kqueue"; }

		// Defined below KqueueListener, which this needs to construct. Datagrams are
		// left to IoContext's default nullptr: this backend has none yet.
		std::unique_ptr<Listener> make_listener() override;

		void post(std::function<void()> callback) override
		{
			std::lock_guard lock(callback_mutex_);
			callbacks_.push(std::move(callback));
		}

		void schedule(std::chrono::milliseconds delay, std::function<void()> callback) override
		{
			timers_.schedule(delay, std::move(callback));
		}

	private:
		// Was a detached thread per call that slept for the delay. Fine for a handful of
		// one-off timers, ruinous for anything per-connection: a deadline on every accepted
		// connection would have meant a sleeping thread on every accepted connection. One
		// thread for the whole context, started only if something is ever scheduled.
		TimerQueue timers_{[this](std::function<void()> cb) { post(std::move(cb)); }};

		void worker_thread()
		{
			in_event_loop_ = true;
			while (!stopped_)
			{
				process_events();
				process_callbacks();
			}
		}

		void process_events()
		{
			struct kevent events[64];
			struct timespec ts = {0, 100000000};  // 100ms

			// Submit pending changes and wait for events in a single syscall (Overlap)
			int n = kevent(kq_, pending_changes_.data(), static_cast<int>(pending_changes_.size()), events, 64, &ts);
			pending_changes_.clear();

			if (n < 0)
			{
				return;
			}

			for (int i = 0; i < n; ++i)
			{
				auto& ev = events[i];
				int fd = static_cast<int>(ev.ident);
				KqueueOperation* op = static_cast<KqueueOperation*>(ev.udata);

				if (!op) continue;

				if (ev.filter == EVFILT_READ)
				{
					if (op->type == KqueueOpType::Accept)
					{
						// Perform accept
						op->accept_fd = accept(fd, reinterpret_cast<sockaddr*>(&op->client_addr), &op->client_addr_len);
						if (op->accept_fd < 0)
						{
							op->error = Error::system(std::error_code(errno, std::system_category()));
						}
						else
						{
							// Set non-blocking
							int flags = fcntl(op->accept_fd, F_GETFL, 0);
							fcntl(op->accept_fd, F_SETFL, flags | O_NONBLOCK);
#ifdef SO_NOSIGPIPE
							// The write path sends with flags 0, and there is no
							// MSG_NOSIGNAL on this platform to pass there instead. A peer
							// that disappears mid-response would otherwise raise SIGPIPE
							// and take the whole server down, which during a campaign
							// looks like a machine fault rather than a closed connection.
							// The epoll backend gets the same protection from
							// MSG_NOSIGNAL on its send; this is the BSD spelling of it.
							const int nosigpipe = 1;
							setsockopt(op->accept_fd, SOL_SOCKET, SO_NOSIGPIPE,
							           &nosigpipe, sizeof(nosigpipe));
#endif
							op->result = op->accept_fd;
						}
					}
					else
					{
						// Regular read operation
						ssize_t bytes = recv(fd, op->buffer, op->length, 0);
						if (bytes < 0)
						{
							op->error = Error::system(std::error_code(errno, std::system_category()));
							op->result = -1;
						}
						else
						{
							op->result = static_cast<int>(bytes);
						}
					}
				}
				else if (ev.filter == EVFILT_WRITE)
				{
					// Perform write
					ssize_t bytes = send(fd, op->buffer, op->length, 0);
					if (bytes < 0)
					{
						op->error = Error::system(std::error_code(errno, std::system_category()));
						op->result = -1;
					}
					else
					{
						op->result = static_cast<int>(bytes);
					}
				}

				// Resume the coroutine
				if (op->continuation)
				{
					op->continuation.resume();
				}
			}
		}

		void process_callbacks()
		{
			std::function<void()> callback;
			{
				std::lock_guard lock(callback_mutex_);
				if (callbacks_.empty()) return;
				callback = std::move(callbacks_.front());
				callbacks_.pop();
			}
			if (callback)
			{
				callback();
			}
		}
	};

	// ============================================================================
	// kqueue Listener Implementation
	// ============================================================================

	class KqueueListener : public Listener
	{
		KqueueContext& ctx_;
		int listen_fd_ = -1;
		uint16_t port_ = 0;

	public:
		explicit KqueueListener(KqueueContext& ctx) : ctx_(ctx) { }

		~KqueueListener() override { close(); }

		expected<void, Error> listen(uint16_t port, int backlog) override
		{
			listen_fd_ = socket(AF_INET, SOCK_STREAM, 0);
			if (listen_fd_ < 0)
			{
				return unexpected(Error::system(std::error_code(errno, std::system_category())));
			}

			// Set non-blocking
			int flags = fcntl(listen_fd_, F_GETFL, 0);
			fcntl(listen_fd_, F_SETFL, flags | O_NONBLOCK);

			int opt = 1;
			setsockopt(listen_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

			sockaddr_in addr{};
			addr.sin_family = AF_INET;
			addr.sin_addr.s_addr = INADDR_ANY;
			addr.sin_port = htons(port);

			if (bind(listen_fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0)
			{
				::close(listen_fd_);
				listen_fd_ = -1;
				return unexpected(Error::system(std::error_code(errno, std::system_category())));
			}

			if (::listen(listen_fd_, backlog) < 0)
			{
				::close(listen_fd_);
				listen_fd_ = -1;
				return unexpected(Error::system(std::error_code(errno, std::system_category())));
			}

			sockaddr_in bound_addr{};
			socklen_t addr_len = sizeof(bound_addr);
			getsockname(listen_fd_, reinterpret_cast<sockaddr*>(&bound_addr), &addr_len);
			port_ = ntohs(bound_addr.sin_port);

			return {};
		}

		Task<AcceptResult> async_accept() override;

		void close() override
		{
			if (listen_fd_ >= 0)
			{
				::close(listen_fd_);
				listen_fd_ = -1;
			}
		}

		bool is_listening() const noexcept override { return listen_fd_ >= 0; }

		uint16_t local_port() const noexcept override { return port_; }

		int fd() const noexcept { return listen_fd_; }
		KqueueContext& context() noexcept { return ctx_; }
	};

	// ============================================================================
	// kqueue Connection Implementation
	// ============================================================================

	class KqueueConnection : public Connection
	{
		KqueueContext& ctx_;
		int fd_;
		std::chrono::milliseconds timeout_{30000};
		CancellationToken cancel_token_;
		std::string remote_addr_;
		uint16_t remote_port_ = 0;

	public:
		KqueueConnection(KqueueContext& ctx, int fd, const sockaddr_in& addr) : ctx_(ctx), fd_(fd)
		{
			configure_accepted_socket(fd);
			char ip[INET_ADDRSTRLEN];
			inet_ntop(AF_INET, &addr.sin_addr, ip, sizeof(ip));
			remote_addr_ = ip;
			remote_port_ = ntohs(addr.sin_port);
		}

		~KqueueConnection() override { close(); }

		Task<ReadResult> async_read(void* buffer, size_t len) override;
		Task<ReadResult> async_read_until(void* buffer, size_t len, char delimiter) override;
		Task<WriteResult> async_write(const void* buffer, size_t len) override;
		Task<WriteResult> async_write_all(const void* buffer, size_t len) override;
		Task<TransmitResult> async_transmit_file(FileHandle file, size_t offset, size_t length) override;

		void close() override
		{
			if (fd_ >= 0)
			{
				::close(fd_);
				fd_ = -1;
			}
		}

		bool is_open() const noexcept override { return fd_ >= 0; }

		void set_timeout(std::chrono::milliseconds timeout) override { timeout_ = timeout; }

		std::string remote_address() const override { return remote_addr_; }

		uint16_t remote_port() const noexcept override { return remote_port_; }

		void set_cancellation_token(CancellationToken token) override { cancel_token_ = std::move(token); }

		int fd() const noexcept { return fd_; }
	};

	// ============================================================================
	// Async Operation Implementations
	// ============================================================================

	// ============================================================================
	// Multi-accept: one shared socket, several concurrent accept operations
	// ============================================================================
	//
	// Deliberately NOT SO_REUSEPORT. macOS has the option, but unlike Linux it does
	// not load-balance across the bound sockets: it gives the connection to the most
	// recently bound listener, so N sockets would leave N-1 of them idle. That is a
	// silent performance bug rather than a build failure, which is the worst kind.
	//
	// FreeBSD added SO_REUSEPORT_LB for exactly this and does balance, so it takes the
	// Linux-style path where available.
	//
	// The shared-socket model uses one descriptor regardless of worker count. Which
	// worker services a connection is decided by whichever one's kevent fires, so
	// connections are not pinned the way Linux's 4-tuple hash pins them.
	bool KqueueContext::enable_multi_accept(uint16_t port, ConnectionHandler handler, int backlog)
	{
		auto listener = std::make_unique<KqueueListener>(*this);
		auto listening = listener->listen(port, backlog);
		if (!listening)
		{
			return false;
		}

		multi_listener_ = std::move(listener);
		multi_accept_ = true;

		// Depth one, as on epoll, and for the same reason: a readiness interface cannot
		// hold several independent waiters on one descriptor.
		//
		// kqueue identifies a knote by (ident, filter). udata is carried, not keyed, and
		// EV_ADD on an existing knote modifies it rather than adding a second. So a pool
		// of N accept operations on one listening descriptor collapses to a single knote
		// holding the last operation registered, and the other N-1 coroutine frames
		// suspend with nothing that will ever resume them. The depth was real on IOCP,
		// where each AcceptEx is a separate overlapped operation, and this line was
		// copied from there.
		//
		// A deeper pool here would need a dup()ed descriptor per slot so the idents
		// differ, at the cost of N-1 spurious EAGAIN returns per arrival. That is a
		// change to the system under measurement and is not made during a campaign.
		start_accept_pool(*this, *multi_listener_, handler, 1);

		return true;
	}

	Task<AcceptResult> KqueueListener::async_accept()
	{
		if (!is_listening())
		{
			co_return unexpected(Error::io(IoError::InvalidArgument, "Not listening"));
		}

		KqueueOperation op{KqueueOpType::Accept};
		int fd = listen_fd_;

		// Register the accept operation
		co_await KqueueRegisterAwaiter{op, [&] { ctx_.register_accept_op(fd, &op); }};

		if (op.error)
		{
			co_return unexpected(op.error);
		}

		if (op.accept_fd < 0)
		{
			co_return unexpected(Error::io(IoError::Unknown, "Accept failed"));
		}

		co_return AcceptResult(std::make_unique<KqueueConnection>(ctx_, op.accept_fd, op.client_addr));
	}

	Task<ReadResult> KqueueConnection::async_read(void* buffer, size_t len)
	{
		if (!is_open())
		{
			co_return unexpected(Error::io(IoError::ConnectionReset, "Connection closed"));
		}

		if (cancel_token_.is_cancelled())
		{
			co_return unexpected(Error::cancelled());
		}

		KqueueOperation op{KqueueOpType::Read};
		op.buffer = buffer;
		op.length = len;
		int fd = fd_;

		// Register the read operation
		co_await KqueueRegisterAwaiter{op, [&] { ctx_.register_read_op(fd, &op); }};

		if (op.error)
		{
			co_return unexpected(op.error);
		}

		if (op.result == 0)
		{
			co_return unexpected(Error::io(IoError::EndOfStream, "Connection closed by peer"));
		}

		if (op.result < 0)
		{
			co_return unexpected(Error::io(IoError::Unknown, "Read failed"));
		}

		co_return static_cast<size_t>(op.result);
	}

	Task<ReadResult> KqueueConnection::async_read_until(void* buffer, size_t len, char delimiter)
	{
		char* buf = static_cast<char*>(buffer);
		size_t total = 0;

		while (total < len)
		{
			auto result = co_await async_read(buf + total, 1);
			if (!result)
			{
				co_return unexpected(result.error());
			}

			total += *result;
			if (buf[total - 1] == delimiter)
			{
				break;
			}
		}

		co_return total;
	}

	Task<WriteResult> KqueueConnection::async_write(const void* buffer, size_t len)
	{
		if (!is_open())
		{
			co_return unexpected(Error::io(IoError::ConnectionReset, "Connection closed"));
		}

		if (cancel_token_.is_cancelled())
		{
			co_return unexpected(Error::cancelled());
		}

		KqueueOperation op{KqueueOpType::Write};
		op.buffer = const_cast<void*>(buffer);
		op.length = len;
		int fd = fd_;

		// Register the write operation
		co_await KqueueRegisterAwaiter{op, [&] { ctx_.register_write_op(fd, &op); }};

		if (op.error)
		{
			co_return unexpected(op.error);
		}

		if (op.result < 0)
		{
			co_return unexpected(Error::io(IoError::Unknown, "Write failed"));
		}

		co_return static_cast<size_t>(op.result);
	}

	Task<WriteResult> KqueueConnection::async_write_all(const void* buffer, size_t len)
	{
		const char* buf = static_cast<const char*>(buffer);
		size_t total = 0;

		while (total < len)
		{
			auto result = co_await async_write(buf + total, len - total);
			if (!result)
			{
				co_return unexpected(result.error());
			}
			total += *result;
		}

		co_return total;
	}

	Task<TransmitResult> KqueueConnection::async_transmit_file(FileHandle file, size_t offset, size_t length)
	{
		if (!is_open())
		{
			co_return unexpected(Error::io(IoError::ConnectionReset, "Connection closed"));
		}

		size_t total_sent = 0;
		while (total_sent < length)
		{
			if (cancel_token_.is_cancelled())
			{
				co_return unexpected(Error::cancelled());
			}

			// Use macOS native sendfile for zero-copy transmission
			off_t len = static_cast<off_t>(length - total_sent);
			if (sendfile(file, fd_, static_cast<off_t>(offset + total_sent), &len, nullptr, 0) < 0)
			{
				if (errno == EAGAIN || errno == EINTR)
				{
					// If would block or interrupted, wait for the socket to become writable
					KqueueOperation op{KqueueOpType::Write};
					co_await KqueueRegisterAwaiter{op, [&] { ctx_.register_write_op(fd_, &op); }};
					continue;
				}
				co_return unexpected(Error::system(std::error_code(errno, std::system_category())));
			}

			if (len == 0)
			{
				break;  // EOF
			}
			total_sent += static_cast<size_t>(len);
		}

		co_return total_sent;
	}

	// Definition of thread_local member
	thread_local std::vector<struct kevent> KqueueContext::pending_changes_;
	thread_local bool KqueueContext::in_event_loop_ = false;

	// ============================================================================
	// Factory Functions
	// ============================================================================

	std::unique_ptr<Listener> KqueueContext::make_listener()
	{
		return std::make_unique<KqueueListener>(*this);
	}

	namespace detail
	{
		std::unique_ptr<IoContext> make_kqueue_context(std::size_t thread_count)
		{
			return std::make_unique<KqueueContext>(thread_count);
		}
	}  // namespace detail

}  // namespace coroute::net

#endif  // COROUTE_PLATFORM_MACOS
