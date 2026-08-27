#include "coroute/net/io_context.hpp"
#include "coroute/net/datagram.hpp"

#if defined(COROUTE_BACKEND_EPOLL)

#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/sendfile.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <linux/udp.h>  // UDP_SEGMENT; netinet/udp.h clashes with it over struct udphdr
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>

#include <array>
#include <thread>
#include <vector>
#include <queue>
#include <mutex>
#include <atomic>
#include <algorithm>
#include <memory>
#include <cstring>
#include <cerrno>
#include <stdexcept>
#include <string>

namespace coroute::net
{

	// ============================================================================
	// Why this backend exists
	// ============================================================================
	//
	// Not as a fallback. Linux already has io_uring here, and this deliberately
	// duplicates it.
	//
	// epoll is a readiness interface: it reports that a descriptor is ready and the
	// syscall is then performed by the caller. io_uring is a completion interface: the
	// operation is submitted and completion is reported. Comparing the two normally
	// means comparing two different programs, so the measurement absorbs every other
	// difference between them. Behind one IoContext seam, serving the same routes
	// through the same parser on the same kernel and hardware, the completion
	// mechanism becomes the only variable.
	//
	// It also earns its place independently: io_uring is restricted or unavailable in
	// enough hardened and containerised environments that a Linux server intended to
	// run everywhere needs this path regardless.

	// ============================================================================
	// Operation types
	// ============================================================================

	enum class EpollOpType
	{
		Accept,
		Read,
		Write
	};

	struct EpollOperation
	{
		EpollOpType type;
		std::coroutine_handle<> continuation;
		Error error;

		// The descriptor the operation was armed on. epoll reports readiness rather
		// than completion, so the event loop needs this to make the actual syscall.
		int fd = -1;
		int result = 0;

		// Accept
		int accept_fd = -1;
		sockaddr_in client_addr{};
		socklen_t client_addr_len = sizeof(sockaddr_in);

		// Read/write
		void* buffer = nullptr;
		size_t length = 0;

		explicit EpollOperation(EpollOpType t) : type(t) { }
	};

	// Arms the descriptor only once the continuation has been recorded.
	//
	// The obvious arrangement, arm the fd and then co_await, has a hole in it. Arming
	// makes the operation eligible immediately, and for a socket that already has data
	// waiting, which on loopback is the normal case rather than the rare one, epoll_wait
	// reports it on another thread before the caller has reached the suspend point. The
	// event loop then finds a null continuation, and because the interest is registered
	// with EPOLLONESHOT the descriptor is disarmed by that same delivery. Nothing will
	// ever fire again and the coroutine waits forever.
	//
	// Doing the arming from inside await_suspend closes the window: the continuation is
	// in place before the descriptor can possibly become eligible.
	template <typename Arm>
	struct EpollArmAwaiter
	{
		EpollOperation& op;
		Arm arm;
		bool armed = false;

		bool await_ready() const noexcept { return false; }

		bool await_suspend(std::coroutine_handle<> h)
		{
			op.continuation = h;

			// Published before arming, while this thread still owns the frame. Once the
			// descriptor is armed the event loop may resume this coroutine at any
			// moment, and a write landing after that races the resumption it caused,
			// into a frame that may already be gone. Only the failure path writes
			// again, and that path is safe by construction: nothing was armed, so
			// nothing can be resuming us.
			armed = true;
			if (!arm())
			{
				armed = false;
				// Resume immediately. No event will ever arrive to wake us.
				return false;
			}

			// A literal, not `return armed`. The frame may already be destroyed, so
			// reading a member here would be a use-after-free.
			return true;
		}

		bool await_resume() const noexcept { return armed; }
	};

	template <typename Arm>
	EpollArmAwaiter(EpollOperation&, Arm) -> EpollArmAwaiter<Arm>;

	// ============================================================================
	// Context
	// ============================================================================

	class EpollContext : public IoContext
	{
		int epfd_ = -1;
		std::vector<std::thread> workers_;
		std::atomic<bool> stopped_{false};
		size_t thread_count_;

		std::mutex callback_mutex_;
		std::queue<std::function<void()>> callbacks_;

		// One SO_REUSEPORT listener per worker, matching the io_uring backend so the
		// two Linux backends are comparable on descriptor count.
		std::vector<std::unique_ptr<Listener>> multi_listeners_;
		bool multi_accept_ = false;

	public:
		explicit EpollContext(size_t thread_count) : thread_count_(thread_count > 0 ? thread_count : 1)
		{
			epfd_ = epoll_create1(EPOLL_CLOEXEC);
			if (epfd_ < 0)
			{
				throw std::runtime_error("epoll_create1() failed");
			}
		}

		~EpollContext() override
		{
			stop();

			for (auto& worker : workers_)
			{
				if (worker.joinable())
				{
					worker.join();
				}
			}

			if (epfd_ >= 0)
			{
				::close(epfd_);
			}
		}

		int epfd() const noexcept { return epfd_; }

		// Arm fd for a single notification of `events`, carrying `op` as the payload.
		//
		// EPOLLONESHOT is what makes calling epoll_wait from every worker safe: the
		// kernel disarms the descriptor as it hands the event out, so exactly one
		// thread ever sees it. Without it several workers wake on the same readable
		// socket and race to read it.
		//
		// A disarmed descriptor stays registered, so re-arming is MOD rather than ADD.
		// Attempting MOD first and falling back on ENOENT avoids tracking which
		// descriptors have been registered before.
		bool arm(int fd, uint32_t events, EpollOperation* op)
		{
			epoll_event ev{};
			ev.events = events | EPOLLONESHOT;
			ev.data.ptr = op;

			if (epoll_ctl(epfd_, EPOLL_CTL_MOD, fd, &ev) == 0)
			{
				return true;
			}
			return errno == ENOENT && epoll_ctl(epfd_, EPOLL_CTL_ADD, fd, &ev) == 0;
		}

		bool arm_read(int fd, EpollOperation* op) { return arm(fd, EPOLLIN, op); }
		bool arm_write(int fd, EpollOperation* op) { return arm(fd, EPOLLOUT, op); }

		void forget(int fd) noexcept { epoll_ctl(epfd_, EPOLL_CTL_DEL, fd, nullptr); }

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

		// Defined below EpollListener, which this needs to construct.
		bool enable_multi_accept(uint16_t port, ConnectionHandler handler, int backlog) override;

		bool is_multi_accept_enabled() const noexcept override { return multi_accept_; }

		void post(std::function<void()> callback) override
		{
			std::lock_guard lock(callback_mutex_);
			callbacks_.push(std::move(callback));
		}

		void schedule(std::chrono::milliseconds delay, std::function<void()> callback) override
		{
			std::thread(
				[this, delay, cb = std::move(callback)]() mutable
				{
					std::this_thread::sleep_for(delay);
					post(std::move(cb));
				})
				.detach();
		}

	private:
		void worker_thread()
		{
			while (!stopped_)
			{
				process_events();
				process_callbacks();
			}
		}

		void process_events()
		{
			std::array<epoll_event, 64> events{};

			// Bounded wait rather than blocking indefinitely, so stop() is noticed
			// without needing an eventfd to interrupt the wait.
			int n = epoll_wait(epfd_, events.data(), static_cast<int>(events.size()), 100);
			if (n < 0)
			{
				return;  // EINTR and friends: the loop comes straight back
			}

			for (int i = 0; i < n; ++i)
			{
				auto* op = static_cast<EpollOperation*>(events[static_cast<size_t>(i)].data.ptr);
				if (!op)
				{
					continue;
				}

				perform(op, events[static_cast<size_t>(i)].events);

				if (op->continuation)
				{
					op->continuation.resume();
				}
			}
		}

		// epoll reports only readiness, so the syscall happens here rather than having
		// been done by the kernel as it would with io_uring. This is precisely the
		// difference the backend exists to measure.
		static void perform(EpollOperation* op, uint32_t revents)
		{
			if ((revents & (EPOLLERR | EPOLLHUP)) != 0 && op->type != EpollOpType::Accept)
			{
				op->error = Error::io(IoError::ConnectionReset, "peer reset the connection");
				op->result = -1;
				return;
			}

			switch (op->type)
			{
				case EpollOpType::Accept:
				{
					op->accept_fd = ::accept4(op->fd, reinterpret_cast<sockaddr*>(&op->client_addr),
					                          &op->client_addr_len, SOCK_NONBLOCK | SOCK_CLOEXEC);
					if (op->accept_fd < 0)
					{
						op->error = Error::system(std::error_code(errno, std::system_category()));
					}
					op->result = op->accept_fd;
					break;
				}
				case EpollOpType::Read:
				{
					ssize_t bytes = ::recv(op->fd, op->buffer, op->length, 0);
					if (bytes < 0)
					{
						op->error = Error::system(std::error_code(errno, std::system_category()));
						op->result = -1;
					}
					else
					{
						op->result = static_cast<int>(bytes);
					}
					break;
				}
				case EpollOpType::Write:
				{
					// length 0 means the caller only wanted the readiness signal, as
					// async_transmit_file does when sendfile returns EAGAIN.
					if (op->length == 0)
					{
						op->result = 0;
						break;
					}
					ssize_t bytes = ::send(op->fd, op->buffer, op->length, MSG_NOSIGNAL);
					if (bytes < 0)
					{
						op->error = Error::system(std::error_code(errno, std::system_category()));
						op->result = -1;
					}
					else
					{
						op->result = static_cast<int>(bytes);
					}
					break;
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
	// Listener
	// ============================================================================

	class EpollListener : public Listener
	{
		EpollContext& ctx_;
		int listen_fd_ = -1;
		uint16_t port_ = 0;
		bool reuse_port_ = false;

	public:
		explicit EpollListener(EpollContext& ctx) : ctx_(ctx) { }

		~EpollListener() override { close(); }

		// SO_REUSEPORT lets several sockets bind the same port, with the kernel hashing
		// the 4-tuple across them so a connection is pinned to one worker for its whole
		// life. Must be set before bind or it has no effect.
		void set_reuse_port(bool enable) noexcept { reuse_port_ = enable; }

		expected<void, Error> listen(uint16_t port, int backlog) override
		{
			listen_fd_ = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
			if (listen_fd_ < 0)
			{
				return unexpected(Error::system(std::error_code(errno, std::system_category())));
			}

			int opt = 1;
			::setsockopt(listen_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
			if (reuse_port_ && ::setsockopt(listen_fd_, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt)) < 0)
			{
				auto err = Error::system(std::error_code(errno, std::system_category()));
				close();
				return unexpected(err);
			}

			sockaddr_in addr{};
			addr.sin_family = AF_INET;
			addr.sin_addr.s_addr = INADDR_ANY;
			addr.sin_port = htons(port);

			if (::bind(listen_fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0)
			{
				auto err = Error::system(std::error_code(errno, std::system_category()));
				close();
				return unexpected(err);
			}

			if (::listen(listen_fd_, backlog) < 0)
			{
				auto err = Error::system(std::error_code(errno, std::system_category()));
				close();
				return unexpected(err);
			}

			sockaddr_in bound{};
			socklen_t bound_len = sizeof(bound);
			if (::getsockname(listen_fd_, reinterpret_cast<sockaddr*>(&bound), &bound_len) == 0)
			{
				port_ = ntohs(bound.sin_port);
			}

			return {};
		}

		Task<AcceptResult> async_accept() override;

		void close() override
		{
			if (listen_fd_ >= 0)
			{
				ctx_.forget(listen_fd_);
				::close(listen_fd_);
				listen_fd_ = -1;
			}
		}

		bool is_listening() const noexcept override { return listen_fd_ >= 0; }

		uint16_t local_port() const noexcept override { return port_; }

		int fd() const noexcept { return listen_fd_; }
	};

	// ============================================================================
	// Connection
	// ============================================================================

	class EpollConnection : public Connection
	{
		EpollContext& ctx_;
		int fd_;
		std::chrono::milliseconds timeout_{30000};
		CancellationToken cancel_token_;
		std::string remote_addr_;
		uint16_t remote_port_ = 0;

	public:
		EpollConnection(EpollContext& ctx, int fd, const sockaddr_in& addr) : ctx_(ctx), fd_(fd)
		{
			std::array<char, INET_ADDRSTRLEN> ip{};
			if (::inet_ntop(AF_INET, &addr.sin_addr, ip.data(), ip.size()) != nullptr)
			{
				remote_addr_ = ip.data();
			}
			remote_port_ = ntohs(addr.sin_port);

			int one = 1;
			::setsockopt(fd_, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));
		}

		~EpollConnection() override { close(); }

		Task<ReadResult> async_read(void* buffer, size_t len) override;
		Task<ReadResult> async_read_until(void* buffer, size_t len, char delimiter) override;
		Task<WriteResult> async_write(const void* buffer, size_t len) override;
		Task<WriteResult> async_write_all(const void* buffer, size_t len) override;
		Task<TransmitResult> async_transmit_file(FileHandle file, size_t offset, size_t length) override;

		void close() override
		{
			if (fd_ >= 0)
			{
				ctx_.forget(fd_);
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
	// Multi-accept: one SO_REUSEPORT listener per worker
	// ============================================================================
	//
	// The same model io_uring uses, so the two Linux backends differ in the completion
	// mechanism and not in how work is spread. That costs one descriptor per worker,
	// against one in total for the shared-socket model Windows and macOS are obliged
	// to use, and buys connection-to-worker pinning from the kernel's 4-tuple hash.
	bool EpollContext::enable_multi_accept(uint16_t port, ConnectionHandler handler, int backlog)
	{
		std::vector<std::unique_ptr<Listener>> listeners;
		listeners.reserve(thread_count_);

		for (size_t i = 0; i < thread_count_; ++i)
		{
			auto listener = std::make_unique<EpollListener>(*this);
			listener->set_reuse_port(true);
			if (!listener->listen(port, backlog))
			{
				// Bind nothing on partial failure. The caller falls back to a single
				// listener, whereas a half-built set would quietly serve on fewer
				// workers than were asked for.
				return false;
			}
			listeners.push_back(std::move(listener));
		}

		multi_listeners_ = std::move(listeners);
		multi_accept_ = true;

		// Depth 1 per listener: the kernel already spreads connections across the
		// sockets, so a deeper pool would only contend for the same accept queue.
		for (auto& listener : multi_listeners_)
		{
			start_accept_pool(*this, *listener, handler, 1);
		}

		return true;
	}

	// ============================================================================
	// Async operations
	// ============================================================================

	Task<AcceptResult> EpollListener::async_accept()
	{
		if (!is_listening())
		{
			co_return unexpected(Error::io(IoError::InvalidArgument, "Not listening"));
		}

		EpollOperation op{EpollOpType::Accept};
		op.fd = listen_fd_;

		if (!co_await EpollArmAwaiter{op, [&] { return ctx_.arm_read(listen_fd_, &op); }})
		{
			co_return unexpected(Error::system(std::error_code(errno, std::system_category())));
		}

		if (op.error)
		{
			co_return unexpected(op.error);
		}
		if (op.accept_fd < 0)
		{
			co_return unexpected(Error::io(IoError::Unknown, "Accept failed"));
		}

		co_return AcceptResult(std::make_unique<EpollConnection>(ctx_, op.accept_fd, op.client_addr));
	}

	Task<ReadResult> EpollConnection::async_read(void* buffer, size_t len)
	{
		if (!is_open())
		{
			co_return unexpected(Error::io(IoError::ConnectionReset, "Connection closed"));
		}
		if (cancel_token_.is_cancelled())
		{
			co_return unexpected(Error::cancelled());
		}

		EpollOperation op{EpollOpType::Read};
		op.buffer = buffer;
		op.length = len;
		op.fd = fd_;

		if (!co_await EpollArmAwaiter{op, [&] { return ctx_.arm_read(fd_, &op); }})
		{
			co_return unexpected(Error::system(std::error_code(errno, std::system_category())));
		}

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

	Task<ReadResult> EpollConnection::async_read_until(void* buffer, size_t len, char delimiter)
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

	Task<WriteResult> EpollConnection::async_write(const void* buffer, size_t len)
	{
		if (!is_open())
		{
			co_return unexpected(Error::io(IoError::ConnectionReset, "Connection closed"));
		}
		if (cancel_token_.is_cancelled())
		{
			co_return unexpected(Error::cancelled());
		}

		EpollOperation op{EpollOpType::Write};
		op.buffer = const_cast<void*>(buffer);
		op.length = len;
		op.fd = fd_;

		if (!co_await EpollArmAwaiter{op, [&] { return ctx_.arm_write(fd_, &op); }})
		{
			co_return unexpected(Error::system(std::error_code(errno, std::system_category())));
		}

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

	Task<WriteResult> EpollConnection::async_write_all(const void* buffer, size_t len)
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

	Task<TransmitResult> EpollConnection::async_transmit_file(FileHandle file, size_t offset, size_t length)
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

			// Linux sendfile takes an in/out offset and returns the byte count, which
			// is the reverse of the macOS signature used by the kqueue backend.
			off_t pos = static_cast<off_t>(offset + total_sent);
			ssize_t sent = ::sendfile(fd_, file, &pos, length - total_sent);

			if (sent < 0)
			{
				if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)
				{
					// Wait for writability, then retry. length 0 tells the event loop
					// to report readiness without attempting a send of its own.
					EpollOperation op{EpollOpType::Write};
					op.fd = fd_;
					op.length = 0;
					if (!co_await EpollArmAwaiter{op, [&] { return ctx_.arm_write(fd_, &op); }})
					{
						co_return unexpected(Error::system(std::error_code(errno, std::system_category())));
					}
					continue;
				}
				co_return unexpected(Error::system(std::error_code(errno, std::system_category())));
			}

			if (sent == 0)
			{
				break;  // EOF on the file
			}
			total_sent += static_cast<size_t>(sent);
		}

		co_return total_sent;
	}

	// ============================================================================
	// Datagram socket
	// ============================================================================
	//
	// Scope note: neither UDP_GRO on receive nor multishot receive is used yet. GRO
	// coalesces several datagrams into one read, which then has to be split apart
	// again because QUIC must process each as its own packet, and the buffer sizing it
	// forces (64 KB per slot rather than 2 KB) is a real cost. Both are worth
	// revisiting once there is a benchmark to justify them. Neither changes this
	// interface, because the contract is already one entry per datagram.

	namespace
	{
		constexpr size_t kDatagramBatch = 32;
		constexpr size_t kDatagramBufSize = 2048;  // comfortably over any QUIC MTU

		const sockaddr* as_sockaddr(const Endpoint& ep) noexcept
		{
			return reinterpret_cast<const sockaddr*>(ep.bytes.data());
		}

		void store_endpoint(Endpoint& ep, const sockaddr* addr, socklen_t len) noexcept
		{
			if (addr == nullptr || len <= 0 || static_cast<size_t>(len) > Endpoint::capacity)
			{
				ep.len = 0;
				return;
			}
			std::memcpy(ep.bytes.data(), addr, static_cast<size_t>(len));
			ep.len = static_cast<std::uint32_t>(len);
		}
	}  // namespace

	class EpollDatagramSocket : public DatagramSocket
	{
		using RecvControl = std::array<std::uint8_t, CMSG_SPACE(sizeof(in_pktinfo)) + CMSG_SPACE(sizeof(int))>;

		EpollContext& ctx_;
		int fd_ = -1;
		uint16_t port_ = 0;
		bool gso_ = false;

		// Receive scratch, reused across calls. The Datagram spans handed back point
		// straight into buffers_, which is why they are valid only until the next
		// receive: copying every packet on the hottest path there is would be waste.
		std::vector<std::array<std::uint8_t, kDatagramBufSize>> buffers_{kDatagramBatch};
		std::vector<RecvControl> control_{kDatagramBatch};
		std::vector<mmsghdr> msgs_{kDatagramBatch};
		std::vector<iovec> iovs_{kDatagramBatch};
		std::vector<sockaddr_storage> peers_{kDatagramBatch};
		std::vector<Datagram> out_{kDatagramBatch};

	public:
		explicit EpollDatagramSocket(EpollContext& ctx) : ctx_(ctx) { }

		~EpollDatagramSocket() override { close(); }

		expected<void, Error> bind(uint16_t port, bool reuse_port) override
		{
			fd_ = ::socket(AF_INET, SOCK_DGRAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
			if (fd_ < 0)
			{
				return unexpected(Error::system(std::error_code(errno, std::system_category())));
			}

			int opt = 1;
			::setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
			if (reuse_port && ::setsockopt(fd_, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt)) < 0)
			{
				auto err = Error::system(std::error_code(errno, std::system_category()));
				close();
				return unexpected(err);
			}

			// Without this the local address of an inbound datagram is unknown, and a
			// wildcard-bound server on a multi-homed host replies from whichever
			// address the routing table prefers. A QUIC client discards a reply that
			// arrives from an address it never sent to.
			if (::setsockopt(fd_, IPPROTO_IP, IP_PKTINFO, &opt, sizeof(opt)) < 0)
			{
				auto err = Error::system(std::error_code(errno, std::system_category()));
				close();
				return unexpected(err);
			}

			// ECN, so congestion signalling reaches QUIC.
			::setsockopt(fd_, IPPROTO_IP, IP_RECVTOS, &opt, sizeof(opt));

			sockaddr_in addr{};
			addr.sin_family = AF_INET;
			addr.sin_addr.s_addr = INADDR_ANY;
			addr.sin_port = htons(port);

			if (::bind(fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0)
			{
				auto err = Error::system(std::error_code(errno, std::system_category()));
				close();
				return unexpected(err);
			}

			sockaddr_in bound{};
			socklen_t bound_len = sizeof(bound);
			if (::getsockname(fd_, reinterpret_cast<sockaddr*>(&bound), &bound_len) == 0)
			{
				port_ = ntohs(bound.sin_port);
			}

			// Probe segmentation offload once rather than per send. A kernel without
			// it simply refuses the option.
			int seg = 0;
			gso_ = ::setsockopt(fd_, IPPROTO_UDP, UDP_SEGMENT, &seg, sizeof(seg)) == 0;

			return {};
		}

		Task<expected<std::span<const Datagram>, Error>> async_recv_batch() override;
		Task<expected<size_t, Error>> async_send(std::span<const std::uint8_t> data, const Endpoint& peer,
		                                         const Endpoint& local, size_t gso_size) override;

		void close() override
		{
			if (fd_ >= 0)
			{
				ctx_.forget(fd_);
				::close(fd_);
				fd_ = -1;
			}
		}

		bool is_open() const noexcept override { return fd_ >= 0; }
		uint16_t local_port() const noexcept override { return port_; }
		bool has_segmentation_offload() const noexcept override { return gso_; }
	};

	Task<expected<std::span<const Datagram>, Error>> EpollDatagramSocket::async_recv_batch()
	{
		if (!is_open())
		{
			co_return unexpected(Error::io(IoError::InvalidArgument, "socket is closed"));
		}

		for (;;)
		{
			for (size_t i = 0; i < kDatagramBatch; ++i)
			{
				iovs_[i].iov_base = buffers_[i].data();
				iovs_[i].iov_len = buffers_[i].size();

				auto& hdr = msgs_[i].msg_hdr;
				hdr = {};
				hdr.msg_name = &peers_[i];
				hdr.msg_namelen = sizeof(sockaddr_storage);
				hdr.msg_iov = &iovs_[i];
				hdr.msg_iovlen = 1;
				hdr.msg_control = control_[i].data();
				hdr.msg_controllen = control_[i].size();
				msgs_[i].msg_len = 0;
			}

			int n = ::recvmmsg(fd_, msgs_.data(), static_cast<unsigned>(kDatagramBatch), 0, nullptr);
			if (n > 0)
			{
				for (size_t i = 0; i < static_cast<size_t>(n); ++i)
				{
					auto& hdr = msgs_[i].msg_hdr;
					auto& out = out_[i];

					out.data = std::span<const std::uint8_t>(buffers_[i].data(), msgs_[i].msg_len);
					store_endpoint(out.peer, reinterpret_cast<const sockaddr*>(hdr.msg_name),
					               static_cast<socklen_t>(hdr.msg_namelen));
					out.local = Endpoint{};
					out.ecn = 0;

					for (cmsghdr* cm = CMSG_FIRSTHDR(&hdr); cm != nullptr; cm = CMSG_NXTHDR(&hdr, cm))
					{
						if (cm->cmsg_level == IPPROTO_IP && cm->cmsg_type == IP_PKTINFO)
						{
							in_pktinfo info{};
							std::memcpy(&info, CMSG_DATA(cm), sizeof(info));
							sockaddr_in local{};
							local.sin_family = AF_INET;
							local.sin_addr = info.ipi_addr;
							local.sin_port = htons(port_);
							store_endpoint(out.local, reinterpret_cast<const sockaddr*>(&local), sizeof(local));
						}
						else if (cm->cmsg_level == IPPROTO_IP && cm->cmsg_type == IP_TOS)
						{
							std::uint8_t tos = 0;
							std::memcpy(&tos, CMSG_DATA(cm), sizeof(tos));
							out.ecn = tos & 0x03;
						}
					}
				}
				co_return std::span<const Datagram>(out_.data(), static_cast<size_t>(n));
			}

			if (n < 0 && errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR)
			{
				co_return unexpected(Error::system(std::error_code(errno, std::system_category())));
			}

			// Nothing queued: wait for readability, then retry the receive above.
			EpollOperation op{EpollOpType::Read};
			op.fd = fd_;
			op.length = 0;  // readiness only
			if (!co_await EpollArmAwaiter{op, [&] { return ctx_.arm_read(fd_, &op); }})
			{
				co_return unexpected(Error::system(std::error_code(errno, std::system_category())));
			}

			if (op.error)
			{
				co_return unexpected(op.error);
			}
		}
	}

	Task<expected<size_t, Error>> EpollDatagramSocket::async_send(std::span<const std::uint8_t> data,
	                                                              const Endpoint& peer, const Endpoint& local,
	                                                              size_t gso_size)
	{
		if (!is_open())
		{
			co_return unexpected(Error::io(IoError::InvalidArgument, "socket is closed"));
		}
		if (peer.empty())
		{
			co_return unexpected(Error::io(IoError::InvalidArgument, "no destination"));
		}

		const bool segment = gso_size > 0 && data.size() > gso_size;

		// Without offload, segmentation is emulated so callers observe one behaviour
		// whichever backend they are on.
		if (segment && !gso_)
		{
			size_t sent = 0;
			while (sent < data.size())
			{
				const size_t chunk = std::min(gso_size, data.size() - sent);
				auto one = co_await async_send(data.subspan(sent, chunk), peer, local, 0);
				if (!one)
				{
					co_return unexpected(one.error());
				}
				sent += chunk;
			}
			co_return sent;
		}

		std::array<std::uint8_t, CMSG_SPACE(sizeof(in_pktinfo)) + CMSG_SPACE(sizeof(std::uint16_t))> control{};

		iovec iov{};
		iov.iov_base = const_cast<std::uint8_t*>(data.data());
		iov.iov_len = data.size();

		msghdr hdr{};
		hdr.msg_name = const_cast<sockaddr*>(as_sockaddr(peer));
		hdr.msg_namelen = peer.len;
		hdr.msg_iov = &iov;
		hdr.msg_iovlen = 1;
		hdr.msg_control = control.data();
		hdr.msg_controllen = control.size();

		size_t used = 0;
		cmsghdr* cm = nullptr;

		if (!local.empty())
		{
			cm = CMSG_FIRSTHDR(&hdr);
			cm->cmsg_level = IPPROTO_IP;
			cm->cmsg_type = IP_PKTINFO;
			cm->cmsg_len = CMSG_LEN(sizeof(in_pktinfo));

			in_pktinfo info{};
			const auto* src = reinterpret_cast<const sockaddr_in*>(as_sockaddr(local));
			info.ipi_spec_dst = src->sin_addr;
			std::memcpy(CMSG_DATA(cm), &info, sizeof(info));
			used += CMSG_SPACE(sizeof(in_pktinfo));
		}

		if (segment)
		{
			cm = (cm == nullptr) ? CMSG_FIRSTHDR(&hdr) : CMSG_NXTHDR(&hdr, cm);
			if (cm != nullptr)
			{
				cm->cmsg_level = IPPROTO_UDP;
				cm->cmsg_type = UDP_SEGMENT;
				cm->cmsg_len = CMSG_LEN(sizeof(std::uint16_t));
				auto seg = static_cast<std::uint16_t>(gso_size);
				std::memcpy(CMSG_DATA(cm), &seg, sizeof(seg));
				used += CMSG_SPACE(sizeof(std::uint16_t));
			}
		}

		hdr.msg_controllen = used;

		for (;;)
		{
			ssize_t sent = ::sendmsg(fd_, &hdr, MSG_NOSIGNAL);
			if (sent >= 0)
			{
				co_return static_cast<size_t>(sent);
			}
			if (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR)
			{
				co_return unexpected(Error::system(std::error_code(errno, std::system_category())));
			}

			EpollOperation op{EpollOpType::Write};
			op.fd = fd_;
			op.length = 0;
			if (!co_await EpollArmAwaiter{op, [&] { return ctx_.arm_write(fd_, &op); }})
			{
				co_return unexpected(Error::system(std::error_code(errno, std::system_category())));
			}

			if (op.error)
			{
				co_return unexpected(op.error);
			}
		}
	}

	// ============================================================================
	// Factory Functions
	// ============================================================================

	std::unique_ptr<IoContext> IoContext::create(size_t thread_count)
	{
		return std::make_unique<EpollContext>(thread_count);
	}

	std::unique_ptr<Listener> Listener::create(IoContext& ctx)
	{
		return std::make_unique<EpollListener>(static_cast<EpollContext&>(ctx));
	}

	std::unique_ptr<DatagramSocket> DatagramSocket::create(IoContext& ctx)
	{
		return std::make_unique<EpollDatagramSocket>(static_cast<EpollContext&>(ctx));
	}

}  // namespace coroute::net

#endif  // COROUTE_BACKEND_EPOLL
