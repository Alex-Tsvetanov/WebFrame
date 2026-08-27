#include "coroute/net/io_context.hpp"

#if defined(COROUTE_BACKEND_EPOLL)

#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/sendfile.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
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

	// Captures the continuation before suspending so the event loop can resume it.
	struct EpollAwaiter
	{
		EpollOperation& op;

		bool await_ready() const noexcept { return false; }
		void await_suspend(std::coroutine_handle<> h) noexcept { op.continuation = h; }
		void await_resume() const noexcept { }
	};

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
			stopped_ = false;

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

		if (!ctx_.arm_read(listen_fd_, &op))
		{
			co_return unexpected(Error::system(std::error_code(errno, std::system_category())));
		}

		co_await EpollAwaiter{op};

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

		if (!ctx_.arm_read(fd_, &op))
		{
			co_return unexpected(Error::system(std::error_code(errno, std::system_category())));
		}

		co_await EpollAwaiter{op};

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

		if (!ctx_.arm_write(fd_, &op))
		{
			co_return unexpected(Error::system(std::error_code(errno, std::system_category())));
		}

		co_await EpollAwaiter{op};

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
					if (!ctx_.arm_write(fd_, &op))
					{
						co_return unexpected(Error::system(std::error_code(errno, std::system_category())));
					}
					co_await EpollAwaiter{op};
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

}  // namespace coroute::net

#endif  // COROUTE_BACKEND_EPOLL
