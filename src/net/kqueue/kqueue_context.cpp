#include "coroute/net/io_context.hpp"

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
#include <memory>
#include <cstring>
#include <unordered_map>

namespace coroute::net {

	// ============================================================================
	// kqueue Operation Types
	// ============================================================================

	enum class KqueueOpType { Accept, Read, Write, Connect };

	struct KqueueOperation {
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

		KqueueOperation(KqueueOpType t) : type(t) {}
	};

	// Custom awaiter that captures the continuation handle before suspending
	struct KqueueAwaiter {
		KqueueOperation& op;

		bool await_ready() const noexcept { return false; }

		void await_suspend(std::coroutine_handle<> h) noexcept { op.continuation = h; }

		void await_resume() const noexcept {}
	};

	// ============================================================================
	// kqueue Context Implementation
	// ============================================================================

	class KqueueContext : public IoContext {
		int kq_ = -1;
		std::vector<std::thread> workers_;
		std::atomic<bool> stopped_{false};
		size_t thread_count_;

		std::mutex callback_mutex_;
		std::queue<std::function<void()>> callbacks_;

		// Track pending operations by file descriptor
		std::mutex ops_mutex_;
		std::unordered_map<int, KqueueOperation*> read_ops_;
		std::unordered_map<int, KqueueOperation*> write_ops_;
		std::unordered_map<int, KqueueOperation*> accept_ops_;

	public:
		explicit KqueueContext(size_t thread_count) : thread_count_(thread_count) {
			kq_ = kqueue();
			if (kq_ < 0) {
				throw std::runtime_error("kqueue() failed");
			}
		}

		~KqueueContext() override {
			stop();

			for (auto& worker : workers_) {
				if (worker.joinable()) {
					worker.join();
				}
			}

			if (kq_ >= 0) {
				close(kq_);
			}
		}

		int kq() const noexcept { return kq_; }

		// Register operation for a file descriptor
		void register_read_op(int fd, KqueueOperation* op) {
			std::lock_guard lock(ops_mutex_);
			read_ops_[fd] = op;

			// Add kqueue filter for read events
			struct kevent ev;
			EV_SET(&ev, fd, EVFILT_READ, EV_ADD | EV_ONESHOT, 0, 0, nullptr);
			kevent(kq_, &ev, 1, nullptr, 0, nullptr);
		}

		void register_write_op(int fd, KqueueOperation* op) {
			std::lock_guard lock(ops_mutex_);
			write_ops_[fd] = op;

			// Add kqueue filter for write events
			struct kevent ev;
			EV_SET(&ev, fd, EVFILT_WRITE, EV_ADD | EV_ONESHOT, 0, 0, nullptr);
			kevent(kq_, &ev, 1, nullptr, 0, nullptr);
		}

		void register_accept_op(int fd, KqueueOperation* op) {
			std::lock_guard lock(ops_mutex_);
			accept_ops_[fd] = op;

			// Add kqueue filter for read events (accept uses read)
			struct kevent ev;
			EV_SET(&ev, fd, EVFILT_READ, EV_ADD | EV_ONESHOT, 0, 0, nullptr);
			kevent(kq_, &ev, 1, nullptr, 0, nullptr);
		}

		void run() override {
			stopped_ = false;

			for (size_t i = 0; i < thread_count_; ++i) {
				workers_.emplace_back([this] { worker_thread(); });
			}

			for (auto& worker : workers_) {
				if (worker.joinable()) {
					worker.join();
				}
			}
		}

		void run_one() override { process_events(); }

		void stop() override { stopped_ = true; }

		bool stopped() const noexcept override { return stopped_; }

		void post(std::function<void()> callback) override {
			std::lock_guard lock(callback_mutex_);
			callbacks_.push(std::move(callback));
		}

		void schedule(std::chrono::milliseconds delay, std::function<void()> callback) override {
			std::thread([this, delay, cb = std::move(callback)]() mutable {
				std::this_thread::sleep_for(delay);
				post(std::move(cb));
			}).detach();
		}

	private:
		void worker_thread() {
			while (!stopped_) {
				process_events();
				process_callbacks();
			}
		}

		void process_events() {
			struct kevent events[64];
			struct timespec ts = {0, 100000000};  // 100ms

			int n = kevent(kq_, nullptr, 0, events, 64, &ts);
			if (n < 0) {
				return;
			}

			for (int i = 0; i < n; ++i) {
				auto& ev = events[i];
				int fd = static_cast<int>(ev.ident);

				KqueueOperation* op = nullptr;

				// Check if it's a read/accept event
				if (ev.filter == EVFILT_READ) {
					std::lock_guard lock(ops_mutex_);

					// Check accept ops first
					auto accept_it = accept_ops_.find(fd);
					if (accept_it != accept_ops_.end()) {
						op = accept_it->second;
						accept_ops_.erase(accept_it);

						// Perform accept
						op->accept_fd = accept(fd, reinterpret_cast<sockaddr*>(&op->client_addr), &op->client_addr_len);
						if (op->accept_fd < 0) {
							op->error = Error::system(std::error_code(errno, std::system_category()));
						} else {
							// Set non-blocking
							int flags = fcntl(op->accept_fd, F_GETFL, 0);
							fcntl(op->accept_fd, F_SETFL, flags | O_NONBLOCK);
							op->result = op->accept_fd;
						}
					} else {
						// Regular read operation
						auto read_it = read_ops_.find(fd);
						if (read_it != read_ops_.end()) {
							op = read_it->second;
							read_ops_.erase(read_it);

							// Perform read
							ssize_t bytes = recv(fd, op->buffer, op->length, 0);
							if (bytes < 0) {
								op->error = Error::system(std::error_code(errno, std::system_category()));
								op->result = -1;
							} else {
								op->result = static_cast<int>(bytes);
							}
						}
					}
				}
				// Check if it's a write event
				else if (ev.filter == EVFILT_WRITE) {
					std::lock_guard lock(ops_mutex_);
					auto write_it = write_ops_.find(fd);
					if (write_it != write_ops_.end()) {
						op = write_it->second;
						write_ops_.erase(write_it);

						// Perform write
						ssize_t bytes = send(fd, op->buffer, op->length, 0);
						if (bytes < 0) {
							op->error = Error::system(std::error_code(errno, std::system_category()));
							op->result = -1;
						} else {
							op->result = static_cast<int>(bytes);
						}
					}
				}

				// Resume the coroutine
				if (op && op->continuation) {
					op->continuation.resume();
				}
			}
		}

		void process_callbacks() {
			std::function<void()> callback;
			{
				std::lock_guard lock(callback_mutex_);
				if (callbacks_.empty()) return;
				callback = std::move(callbacks_.front());
				callbacks_.pop();
			}
			if (callback) {
				callback();
			}
		}
	};

	// ============================================================================
	// kqueue Listener Implementation
	// ============================================================================

	class KqueueListener : public Listener {
		KqueueContext& ctx_;
		int listen_fd_ = -1;
		uint16_t port_ = 0;

	public:
		explicit KqueueListener(KqueueContext& ctx) : ctx_(ctx) {}

		~KqueueListener() override { close(); }

		expected<void, Error> listen(uint16_t port, int backlog) override {
			listen_fd_ = socket(AF_INET, SOCK_STREAM, 0);
			if (listen_fd_ < 0) {
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

			if (bind(listen_fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
				::close(listen_fd_);
				listen_fd_ = -1;
				return unexpected(Error::system(std::error_code(errno, std::system_category())));
			}

			if (::listen(listen_fd_, backlog) < 0) {
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

		void close() override {
			if (listen_fd_ >= 0) {
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

	class KqueueConnection : public Connection {
		KqueueContext& ctx_;
		int fd_;
		std::chrono::milliseconds timeout_{30000};
		CancellationToken cancel_token_;
		std::string remote_addr_;
		uint16_t remote_port_ = 0;

	public:
		KqueueConnection(KqueueContext& ctx, int fd, const sockaddr_in& addr) : ctx_(ctx), fd_(fd) {
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

		void close() override {
			if (fd_ >= 0) {
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

	Task<AcceptResult> KqueueListener::async_accept() {
		if (!is_listening()) {
			co_return unexpected(Error::io(IoError::InvalidArgument, "Not listening"));
		}

		KqueueOperation op{KqueueOpType::Accept};
		int fd = listen_fd_;

		// Register the accept operation
		ctx_.register_accept_op(fd, &op);

		// Suspend and wait for the event
		co_await KqueueAwaiter{op};

		if (op.error) {
			co_return unexpected(op.error);
		}

		if (op.accept_fd < 0) {
			co_return unexpected(Error::io(IoError::Unknown, "Accept failed"));
		}

		co_return AcceptResult(std::make_unique<KqueueConnection>(ctx_, op.accept_fd, op.client_addr));
	}

	Task<ReadResult> KqueueConnection::async_read(void* buffer, size_t len) {
		if (!is_open()) {
			co_return unexpected(Error::io(IoError::ConnectionReset, "Connection closed"));
		}

		if (cancel_token_.is_cancelled()) {
			co_return unexpected(Error::cancelled());
		}

		KqueueOperation op{KqueueOpType::Read};
		op.buffer = buffer;
		op.length = len;
		int fd = fd_;

		// Register the read operation
		ctx_.register_read_op(fd, &op);

		// Suspend and wait for the event
		co_await KqueueAwaiter{op};

		if (op.error) {
			co_return unexpected(op.error);
		}

		if (op.result == 0) {
			co_return unexpected(Error::io(IoError::EndOfStream, "Connection closed by peer"));
		}

		if (op.result < 0) {
			co_return unexpected(Error::io(IoError::Unknown, "Read failed"));
		}

		co_return static_cast<size_t>(op.result);
	}

	Task<ReadResult> KqueueConnection::async_read_until(void* buffer, size_t len, char delimiter) {
		char* buf = static_cast<char*>(buffer);
		size_t total = 0;

		while (total < len) {
			auto result = co_await async_read(buf + total, 1);
			if (!result) {
				co_return unexpected(result.error());
			}

			total += *result;
			if (buf[total - 1] == delimiter) {
				break;
			}
		}

		co_return total;
	}

	Task<WriteResult> KqueueConnection::async_write(const void* buffer, size_t len) {
		if (!is_open()) {
			co_return unexpected(Error::io(IoError::ConnectionReset, "Connection closed"));
		}

		if (cancel_token_.is_cancelled()) {
			co_return unexpected(Error::cancelled());
		}

		KqueueOperation op{KqueueOpType::Write};
		op.buffer = const_cast<void*>(buffer);
		op.length = len;
		int fd = fd_;

		// Register the write operation
		ctx_.register_write_op(fd, &op);

		// Suspend and wait for the event
		co_await KqueueAwaiter{op};

		if (op.error) {
			co_return unexpected(op.error);
		}

		if (op.result < 0) {
			co_return unexpected(Error::io(IoError::Unknown, "Write failed"));
		}

		co_return static_cast<size_t>(op.result);
	}

	Task<WriteResult> KqueueConnection::async_write_all(const void* buffer, size_t len) {
		const char* buf = static_cast<const char*>(buffer);
		size_t total = 0;

		while (total < len) {
			auto result = co_await async_write(buf + total, len - total);
			if (!result) {
				co_return unexpected(result.error());
			}
			total += *result;
		}

		co_return total;
	}

	Task<TransmitResult> KqueueConnection::async_transmit_file(FileHandle file, size_t offset, size_t length) {
		if (!is_open()) {
			co_return unexpected(Error::io(IoError::ConnectionReset, "Connection closed"));
		}

		if (cancel_token_.is_cancelled()) {
			co_return unexpected(Error::cancelled());
		}

		// macOS sendfile has different signature than Linux
		// Use a simple read/write loop for now
		// A production implementation could use sendfile with proper offset handling

		size_t total_sent = 0;
		off_t off = static_cast<off_t>(offset);

		// Set file position
		if (lseek(file, off, SEEK_SET) < 0) {
			co_return unexpected(Error::system(std::error_code(errno, std::system_category())));
		}

		// Read and write in chunks
		char buf[8192];
		while (total_sent < length) {
			size_t to_read = std::min(sizeof(buf), length - total_sent);
			ssize_t bytes_read = read(file, buf, to_read);

			if (bytes_read < 0) {
				co_return unexpected(Error::system(std::error_code(errno, std::system_category())));
			}
			if (bytes_read == 0) {
				break;  // EOF
			}

			auto write_result = co_await async_write_all(buf, bytes_read);
			if (!write_result) {
				co_return unexpected(write_result.error());
			}

			total_sent += bytes_read;
		}

		co_return total_sent;
	}

	// ============================================================================
	// Factory Functions
	// ============================================================================

	std::unique_ptr<IoContext> IoContext::create(size_t thread_count) {
		return std::make_unique<KqueueContext>(thread_count);
	}

	std::unique_ptr<Listener> Listener::create(IoContext& ctx) {
		return std::make_unique<KqueueListener>(static_cast<KqueueContext&>(ctx));
	}

}  // namespace coroute::net

#endif  // COROUTE_PLATFORM_MACOS
