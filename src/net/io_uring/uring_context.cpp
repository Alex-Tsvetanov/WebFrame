#include "coroute/net/io_context.hpp"
#include "coroute/net/datagram.hpp"

#if defined(COROUTE_PLATFORM_LINUX)

#include <liburing.h>
#include <sys/socket.h>
#include <sys/eventfd.h>
#include <sys/sendfile.h>
#include <netinet/in.h>
#include <linux/udp.h>  // UDP_SEGMENT; netinet/udp.h clashes with it over struct udphdr
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>

#include <thread>
#include <array>
#include <vector>
#include <queue>
#include <mutex>
#include <atomic>
#include <memory>
#include <cstring>
#include <stdexcept>
#include <string>

namespace coroute::net
{

	// ============================================================================
	// io_uring Operation Types
	// ============================================================================

	enum class UringOpType
	{
		Accept,
		Read,
		Write,
		Connect,
		Timeout,
		Cancel
	};

	struct UringOperation
	{
		UringOpType type;
		std::coroutine_handle<> continuation;
		Error error;
		int result = 0;
		size_t ring_index = 0;  // Which ring this operation belongs to

		// For accept
		int accept_fd = -1;
		sockaddr_in client_addr{};
		socklen_t client_addr_len = sizeof(sockaddr_in);

		UringOperation(UringOpType t) : type(t) { }
	};

	// Custom awaiter that captures the continuation handle before suspending
	struct UringAwaiter
	{
		UringOperation& op;

		bool await_ready() const noexcept { return false; }

		void await_suspend(std::coroutine_handle<> h) noexcept { op.continuation = h; }

		void await_resume() const noexcept { }
	};

	// Submits only once the continuation has been recorded.
	//
	// UringAwaiter sets op.continuation in await_suspend, which runs after the caller
	// has already submitted the SQE. If the kernel completes the operation inside that
	// window, and for a datagram already sitting in the socket queue it routinely
	// does, the CQE handler finds a null continuation, consumes the completion with
	// cq_advance, and the coroutine is never resumed. The operation hangs forever.
	//
	// Doing the submission from inside await_suspend closes the window: the
	// continuation is in place before the kernel can possibly report completion.
	template <typename Submit>
	struct UringSubmitAwaiter
	{
		UringOperation& op;
		Submit submit;
		bool submitted = false;

		bool await_ready() const noexcept { return false; }

		bool await_suspend(std::coroutine_handle<> h)
		{
			op.continuation = h;

			// Set before submitting, not after.
			//
			// A submitted operation can complete on a worker thread and resume this
			// coroutine while submit() is still returning. Writing the result afterwards
			// is a race against that resumption, and it is a race the writer loses: the
			// coroutine reads the flag while it is still false and reports a submission
			// failure for an operation that was submitted perfectly well. Worse, the
			// assignment then lands in a frame that may already have been destroyed.
			//
			// So the optimistic value is published first, while this thread still owns
			// the frame, and only the failure path writes again. That path is safe by
			// construction: nothing was submitted, so nothing can be resuming us.
			submitted = true;
			if (!submit())
			{
				submitted = false;
				// Resume immediately. There will never be a completion to wake us.
				return false;
			}

			// Deliberately a literal rather than `return submitted`. The frame may
			// already be gone, so reading a member here would be a use-after-free.
			return true;
		}

		bool await_resume() const noexcept { return submitted; }
	};

	template <typename Submit>
	UringSubmitAwaiter(UringOperation&, Submit) -> UringSubmitAwaiter<Submit>;

	// ============================================================================
	// Per-Thread Ring - Each worker has its own io_uring instance and listener
	// ============================================================================

	struct WorkerRing
	{
		io_uring ring;
		int eventfd = -1;
		int listen_fd = -1;  // SO_REUSEPORT listener for this ring
		std::atomic<bool> initialized{false};

		// io_uring's submission queue is single-producer. Nothing in liburing
		// serialises it, and two threads calling io_uring_get_sqe on one ring will
		// either be handed the same entry or corrupt the tail index.
		//
		// This ring has at least two producers whenever the context is running: the
		// worker thread polling it, and whichever thread submitted the I/O. The worker
		// counts as a producer because io_uring_wait_cqe_timeout takes an SQE for its
		// timeout on kernels without IORING_FEAT_EXT_ARG.
		//
		// No observed failure has been traced to this lock being absent, so it is
		// insurance rather than a fix. The contract is still the contract, and a data
		// race on a ring index is not something to leave in place because it has not
		// been caught misbehaving yet.
		std::mutex sq_mutex;

		// Work directed at this specific thread. Per ring rather than shared because a
		// QUIC connection belongs to exactly one worker, so a packet that lands on the
		// wrong one has to be handed to a named thread and not merely to whoever is
		// free.
		std::mutex callback_mutex;
		std::queue<std::function<void()>> callbacks;

		WorkerRing() = default;

		~WorkerRing()
		{
			if (listen_fd >= 0)
			{
				::close(listen_fd);
			}
			if (initialized)
			{
				if (eventfd >= 0)
				{
					::close(eventfd);
				}
				io_uring_queue_exit(&ring);
			}
		}

		// Non-copyable, non-movable
		WorkerRing(const WorkerRing&) = delete;
		WorkerRing& operator=(const WorkerRing&) = delete;

		bool init()
		{
			// Use larger queue size for better throughput
			io_uring_params params{};
			// SQPOLL can hurt performance for this workload, use regular mode
			int ret = io_uring_queue_init_params(8192, &ring, &params);
			if (ret < 0)
			{
				return false;
			}

			eventfd = ::eventfd(0, EFD_NONBLOCK);
			if (eventfd < 0)
			{
				io_uring_queue_exit(&ring);
				return false;
			}

			initialized = true;
			return true;
		}

		// Create SO_REUSEPORT listener for this ring
		bool create_listener(uint16_t port, int backlog)
		{
			listen_fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
			if (listen_fd < 0) return false;

			int opt = 1;
			setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
			setsockopt(listen_fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));

			sockaddr_in addr{};
			addr.sin_family = AF_INET;
			addr.sin_addr.s_addr = INADDR_ANY;
			addr.sin_port = htons(port);

			if (bind(listen_fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0)
			{
				::close(listen_fd);
				listen_fd = -1;
				return false;
			}

			if (::listen(listen_fd, backlog) < 0)
			{
				::close(listen_fd);
				listen_fd = -1;
				return false;
			}

			return true;
		}

		void wake()
		{
			if (eventfd >= 0)
			{
				uint64_t val = 1;
				::write(eventfd, &val, sizeof(val));
			}
		}
	};

	// ============================================================================
	// io_uring Context Implementation - Per-Thread Rings
	// ============================================================================

	// Forward declaration
	class UringConnection;

	// Connection handler callback
	using ConnectionHandler = std::function<void(std::unique_ptr<Connection>)>;

	class UringContext : public IoContext
	{
		std::vector<std::unique_ptr<WorkerRing>> rings_;
		std::vector<std::thread> workers_;
		std::atomic<bool> stopped_{false};
		std::atomic<size_t> next_ring_{0};
		size_t thread_count_;

		// SO_REUSEPORT multi-accept
		ConnectionHandler connection_handler_;
		uint16_t listen_port_ = 0;
		bool multi_accept_enabled_ = false;


	public:
		explicit UringContext(size_t thread_count) : thread_count_(thread_count > 0 ? thread_count : 1)
		{
			// Create per-thread rings
			rings_.reserve(thread_count_);
			for (size_t i = 0; i < thread_count_; ++i)
			{
				auto ring = std::make_unique<WorkerRing>();
				if (!ring->init())
				{
					throw std::runtime_error("Failed to initialize io_uring ring " + std::to_string(i));
				}
				rings_.push_back(std::move(ring));
			}
		}

		~UringContext() override
		{
			stop();

			for (auto& worker : workers_)
			{
				if (worker.joinable())
				{
					worker.join();
				}
			}
		}

		size_t ring_count() const noexcept { return rings_.size(); }

		WorkerRing* worker_ring(size_t index) noexcept { return rings_[index % rings_.size()].get(); }

		// Get the ring for a specific index
		io_uring* ring(size_t index) noexcept { return &rings_[index % rings_.size()]->ring; }

		// Get the next ring index (round-robin)
		size_t next_ring_index() noexcept { return next_ring_.fetch_add(1, std::memory_order_relaxed) % rings_.size(); }

		// Enable SO_REUSEPORT multi-accept: each worker accepts on its own listener
		size_t worker_count() const noexcept override { return thread_count_; }

		bool enable_multi_accept(uint16_t port, ConnectionHandler handler, int backlog = 1024) override
		{
			// Create SO_REUSEPORT listeners on all rings
			for (auto& ring : rings_)
			{
				if (!ring->create_listener(port, backlog))
				{
					// Clean up on failure
					for (auto& r : rings_)
					{
						if (r->listen_fd >= 0)
						{
							::close(r->listen_fd);
							r->listen_fd = -1;
						}
					}
					return false;
				}
			}

			connection_handler_ = std::move(handler);
			listen_port_ = port;
			multi_accept_enabled_ = true;
			return true;
		}

		bool is_multi_accept_enabled() const noexcept override { return multi_accept_enabled_; }
		uint16_t listen_port() const noexcept { return listen_port_; }

		// Submit SQE to a specific ring
		template <typename PrepFunc>
		bool submit_sqe(size_t ring_index, UringOperation* op, PrepFunc prep_func)
		{
			auto* worker_ring = rings_[ring_index % rings_.size()].get();

			// Held across the whole sequence, not just the get: an SQE is only reserved
			// once it has been prepared and the tail advanced by submit.
			std::lock_guard lock(worker_ring->sq_mutex);

			io_uring_sqe* sqe = io_uring_get_sqe(&worker_ring->ring);
			if (sqe == nullptr)
			{
				// A null entry means the submission queue is full of entries that have
				// been prepared but not yet accepted by the kernel. That is a state to
				// clear, not an error: submitting hands them over and frees the space.
				// Failing here instead was reporting a transient queue condition as if
				// the operation itself were impossible.
				if (io_uring_submit(&worker_ring->ring) < 0)
				{
					return false;
				}
				sqe = io_uring_get_sqe(&worker_ring->ring);
				if (sqe == nullptr)
				{
					return false;
				}
			}

			prep_func(sqe);
			io_uring_sqe_set_data(sqe, op);
			op->ring_index = ring_index;

			// Checked rather than discarded. A failed submit leaves the entry sitting in
			// the queue, so ignoring it turns one refused submission into a queue that
			// fills up and starts refusing everything.
			return io_uring_submit(&worker_ring->ring) >= 0;
		}

		// Submit to ring 0 (default)
		template <typename PrepFunc>
		bool submit_sqe(UringOperation* op, PrepFunc prep_func)
		{
			return submit_sqe(0, op, prep_func);
		}

		void run() override
		{
			// Deliberately does NOT reset stopped_.
			//
			// It used to, which made stop() racy: a stop arriving before run() had been
			// scheduled was erased here, and the workers then spun forever. Restarting a
			// stopped context is not a supported operation anywhere, so clearing the flag
			// bought nothing and cost a hang.

			// Start one worker thread per ring
			for (size_t i = 0; i < thread_count_; ++i)
			{
				workers_.emplace_back([this, i] { worker_loop(i); });
			}

			// Wait for all workers
			for (auto& worker : workers_)
			{
				if (worker.joinable())
				{
					worker.join();
				}
			}
		}

		void run_one() override { poll_and_resume(0); }

		void stop() override
		{
			stopped_ = true;

			// Wake up all worker threads
			for (auto& ring : rings_)
			{
				ring->wake();
			}
		}

		bool stopped() const noexcept override { return stopped_; }

		void post(std::function<void()> callback) override
		{
			// Ring 0 by convention: post() promises only that the work runs on the
			// event loop, not where.
			enqueue_on(0, std::move(callback));
		}

		bool supports_worker_affinity() const noexcept override { return true; }

		void run_on_worker(size_t index, std::function<void()> fn) override
		{
			enqueue_on(index, std::move(fn));
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
		// Accept loop coroutine for multi-accept mode
		Task<void> accept_loop(size_t ring_index);

		void worker_loop(size_t ring_index)
		{
			// Start accept loop if multi-accept is enabled
			if (multi_accept_enabled_ && rings_[ring_index]->listen_fd >= 0)
			{
				accept_loop(ring_index).start_detached();
			}

			while (!stopped_)
			{
				poll_and_resume(ring_index);

				// Its own queue, not a shared one. A callback directed here has been
				// directed here on purpose.
				process_callbacks(ring_index);
			}
		}

		void poll_and_resume(size_t ring_index)
		{
			auto* worker_ring = rings_[ring_index].get();
			io_uring_cqe* cqe;

			// Use short timeout - balance between latency and CPU usage
			__kernel_timespec ts;
			ts.tv_sec = 0;
			ts.tv_nsec = 1000;  // 1µs timeout for low latency

			// The wait needs the submission lock because it takes an SQE for its timeout,
			// but it must not still hold it below: completions are resumed inline, and a
			// resumed coroutine's next act is usually to submit again. Holding the lock
			// across that would deadlock the worker against itself.
			//
			// The completion queue needs no lock. It has exactly one consumer, this
			// worker, which is what makes releasing here safe rather than merely
			// convenient.
			int ret = 0;
			{
				std::lock_guard lock(worker_ring->sq_mutex);
				ret = io_uring_wait_cqe_timeout(&worker_ring->ring, &cqe, &ts);
			}

			// Deliberately no early return on -ETIME.
			//
			// A wait that times out can still have completions sitting behind it: the
			// timeout only says none arrived before the deadline, not that the queue is
			// empty. Returning early skipped the drain and left them for the next turn,
			// which on a kernel that implements the timeout as a real operation also
			// meant leaving that operation's own completion behind every time.
			//
			// The drain below copes with an empty queue, so falling through costs
			// nothing when there is genuinely nothing to do, and costs one turn of
			// latency less when there is.
			if (ret < 0 && ret != -ETIME)
			{
				return;
			}

			// Process all available completions in batch
			unsigned head;
			unsigned processed = 0;
			io_uring_for_each_cqe(&worker_ring->ring, head, cqe)
			{
				auto* op = static_cast<UringOperation*>(io_uring_cqe_get_data(cqe));
				if (op)
				{
					op->result = cqe->res;
					if (cqe->res < 0)
					{
						op->error = Error::system(std::error_code(-cqe->res, std::system_category()));
					}

					// Resume coroutine inline
					if (op->continuation)
					{
						op->continuation.resume();
					}
				}
				processed++;
				if (processed >= 512) break;  // Limit batch size
			}
			io_uring_cq_advance(&worker_ring->ring, processed);
		}

		void enqueue_on(size_t index, std::function<void()> callback)
		{
			if (rings_.empty())
			{
				return;
			}
			auto* ring = rings_[index % rings_.size()].get();
			{
				std::lock_guard lock(ring->callback_mutex);
				ring->callbacks.push(std::move(callback));
			}
			// Woken because the worker may be parked in a wait with nothing else due.
			ring->wake();
		}

		void process_callbacks(size_t ring_index)
		{
			auto* ring = rings_[ring_index].get();

			// Bounded so a steady stream of callbacks cannot starve the completions this
			// worker is also responsible for.
			for (int i = 0; i < 32; ++i)
			{
				std::function<void()> callback;
				{
					std::lock_guard lock(ring->callback_mutex);
					if (ring->callbacks.empty()) return;
					callback = std::move(ring->callbacks.front());
					ring->callbacks.pop();
				}
				if (callback)
				{
					callback();
				}
			}
		}
	};

	// ============================================================================
	// io_uring Listener Implementation
	// ============================================================================

	class UringListener : public Listener
	{
		UringContext& ctx_;
		int listen_fd_ = -1;
		uint16_t port_ = 0;

	public:
		explicit UringListener(UringContext& ctx) : ctx_(ctx) { }

		~UringListener() override { close(); }

		expected<void, Error> listen(uint16_t port, int backlog) override
		{
			listen_fd_ = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
			if (listen_fd_ < 0)
			{
				return unexpected(Error::system(std::error_code(errno, std::system_category())));
			}

			// Set SO_REUSEADDR
			int opt = 1;
			setsockopt(listen_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

			// Bind
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

			// Listen
			if (::listen(listen_fd_, backlog) < 0)
			{
				::close(listen_fd_);
				listen_fd_ = -1;
				return unexpected(Error::system(std::error_code(errno, std::system_category())));
			}

			// Get actual port
			sockaddr_in bound_addr{};
			socklen_t addr_len = sizeof(bound_addr);
			getsockname(listen_fd_, reinterpret_cast<sockaddr*>(&bound_addr), &addr_len);
			port_ = ntohs(bound_addr.sin_port);

			return {};
		}

		Task<AcceptResult> async_accept() override;

		UringContext& context() noexcept { return ctx_; }
		int fd() const noexcept { return listen_fd_; }

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
	};

	// ============================================================================
	// io_uring Connection Implementation
	// ============================================================================

	class UringConnection : public Connection
	{
		UringContext& ctx_;
		int fd_;
		size_t ring_index_;  // Which ring this connection is assigned to
		std::chrono::milliseconds timeout_{30000};
		CancellationToken cancel_token_;
		std::string remote_addr_;
		uint16_t remote_port_ = 0;

	public:
		UringConnection(UringContext& ctx, int fd, const sockaddr_in& addr, size_t ring_index = 0)
			: ctx_(ctx), fd_(fd), ring_index_(ring_index)
		{
			char ip[INET_ADDRSTRLEN];
			inet_ntop(AF_INET, &addr.sin_addr, ip, sizeof(ip));
			remote_addr_ = ip;
			remote_port_ = ntohs(addr.sin_port);
		}

		size_t ring_index() const noexcept { return ring_index_; }

		~UringConnection() override { close(); }

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

	Task<AcceptResult> UringListener::async_accept()
	{
		if (!is_listening())
		{
			co_return unexpected(Error::io(IoError::InvalidArgument, "Not listening"));
		}

		UringOperation op{UringOpType::Accept};
		int fd = listen_fd_;

		// Submit accept to ring 0. Submission happens inside the awaiter so the
		// continuation is recorded before the kernel can report completion.
		if (!co_await UringSubmitAwaiter{op, [&]
		                                 {
											 return ctx_.submit_sqe(
												 0, &op,
												 [fd, &op](io_uring_sqe* sqe)
												 {
													 io_uring_prep_accept(sqe, fd,
				                                                          reinterpret_cast<sockaddr*>(&op.client_addr),
				                                                          &op.client_addr_len, 0);
												 });
										 }})
		{
			co_return unexpected(Error::io(IoError::Unknown, "Failed to get SQE"));
		}

		if (op.error)
		{
			co_return unexpected(op.error);
		}

		if (op.result < 0)
		{
			co_return unexpected(Error::system(std::error_code(-op.result, std::system_category())));
		}

		// Connection on ring 0
		co_return AcceptResult(std::make_unique<UringConnection>(ctx_, op.result, op.client_addr, 0));
	}

	Task<ReadResult> UringConnection::async_read(void* buffer, size_t len)
	{
		if (!is_open())
		{
			co_return unexpected(Error::io(IoError::ConnectionReset, "Connection closed"));
		}

		if (cancel_token_.is_cancelled())
		{
			co_return unexpected(Error::cancelled());
		}

		UringOperation op{UringOpType::Read};
		int fd = fd_;

		if (!co_await UringSubmitAwaiter{op, [&]
		                                 {
											 return ctx_.submit_sqe(ring_index_, &op,
			                                                        [fd, buffer, len](io_uring_sqe* sqe)
			                                                        { io_uring_prep_recv(sqe, fd, buffer, len, 0); });
										 }})
		{
			co_return unexpected(Error::io(IoError::Unknown, "Failed to get SQE"));
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
			co_return unexpected(Error::system(std::error_code(-op.result, std::system_category())));
		}

		co_return static_cast<size_t>(op.result);
	}

	Task<ReadResult> UringConnection::async_read_until(void* buffer, size_t len, char delimiter)
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

	Task<WriteResult> UringConnection::async_write(const void* buffer, size_t len)
	{
		if (!is_open())
		{
			co_return unexpected(Error::io(IoError::ConnectionReset, "Connection closed"));
		}

		if (cancel_token_.is_cancelled())
		{
			co_return unexpected(Error::cancelled());
		}

		UringOperation op{UringOpType::Write};
		int fd = fd_;

		if (!co_await UringSubmitAwaiter{op, [&]
		                                 {
											 return ctx_.submit_sqe(ring_index_, &op,
			                                                        [fd, buffer, len](io_uring_sqe* sqe)
			                                                        { io_uring_prep_send(sqe, fd, buffer, len, 0); });
										 }})
		{
			co_return unexpected(Error::io(IoError::Unknown, "Failed to get SQE"));
		}

		if (op.error)
		{
			co_return unexpected(op.error);
		}

		if (op.result < 0)
		{
			co_return unexpected(Error::system(std::error_code(-op.result, std::system_category())));
		}

		co_return static_cast<size_t>(op.result);
	}

	Task<WriteResult> UringConnection::async_write_all(const void* buffer, size_t len)
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

	Task<TransmitResult> UringConnection::async_transmit_file(FileHandle file, size_t offset, size_t length)
	{
		if (!is_open())
		{
			co_return unexpected(Error::io(IoError::ConnectionReset, "Connection closed"));
		}

		if (cancel_token_.is_cancelled())
		{
			co_return unexpected(Error::cancelled());
		}

		// Use sendfile for zero-copy file transfer
		// Note: sendfile is blocking, but for large files we could use io_uring's splice
		// For now, use sendfile in a simple loop
		size_t total_sent = 0;
		off_t off = static_cast<off_t>(offset);

		while (total_sent < length)
		{
			ssize_t sent = sendfile(fd_, file, &off, length - total_sent);
			if (sent < 0)
			{
				if (errno == EAGAIN || errno == EWOULDBLOCK)
				{
					// Socket buffer full, need to wait for writability
					UringOperation wait_op{UringOpType::Write};
					int fd = fd_;
					if (!co_await UringSubmitAwaiter{wait_op, [&]
					                                 {
														 return ctx_.submit_sqe(ring_index_, &wait_op,
						                                                        [fd](io_uring_sqe* sqe)
						                                                        {
																					// Zero-length send: waits for
							                                                        // socket writability
																					io_uring_prep_send(sqe, fd, nullptr,
							                                                                           0, MSG_NOSIGNAL);
																				});
													 }})
					{
						co_return unexpected(Error::io(IoError::Unknown, "Failed to get SQE"));
					}
					continue;
				}
				co_return unexpected(Error::system(std::error_code(errno, std::system_category())));
			}
			if (sent == 0)
			{
				break;  // EOF on source file
			}
			total_sent += static_cast<size_t>(sent);
		}

		co_return total_sent;
	}

	// ============================================================================
	// Accept Loop Implementation (for SO_REUSEPORT multi-accept)
	// ============================================================================

	// Helper to set TCP optimizations on accepted sockets
	static void set_tcp_opts(int fd)
	{
		int opt = 1;
		setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt));
#ifdef TCP_QUICKACK
		setsockopt(fd, IPPROTO_TCP, TCP_QUICKACK, &opt, sizeof(opt));
#endif
		// Increase send buffer for better throughput
		int sndbuf = 256 * 1024;
		setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &sndbuf, sizeof(sndbuf));
	}

	Task<void> UringContext::accept_loop(size_t ring_index)
	{
		auto* worker_ring = rings_[ring_index].get();

		while (!stopped_ && worker_ring->listen_fd >= 0)
		{
			UringOperation op{UringOpType::Accept};
			int fd = worker_ring->listen_fd;

			if (!co_await UringSubmitAwaiter{op, [&]
			                                 {
												 return submit_sqe(ring_index, &op,
				                                                   [fd, &op](io_uring_sqe* sqe)
				                                                   {
																	   io_uring_prep_accept(
																		   sqe, fd,
																		   reinterpret_cast<sockaddr*>(&op.client_addr),
																		   &op.client_addr_len, SOCK_NONBLOCK);
																   });
											 }})
			{
				continue;
			}

			if (stopped_) break;

			if (op.result < 0)
			{
				// Accept error - continue unless stopped
				continue;
			}

			// Set TCP optimizations
			set_tcp_opts(op.result);

			// Create connection on this ring
			auto conn = std::make_unique<UringConnection>(*this, op.result, op.client_addr, ring_index);

			// Call the connection handler
			if (connection_handler_)
			{
				connection_handler_(std::move(conn));
			}
			else
			{
				conn->close();
			}
		}
	}

	// ============================================================================
	// Datagram socket
	// ============================================================================
	//
	// Receive is completion-based for the packet that is waited on: io_uring_prep_recvmsg
	// submits the receive and the kernel performs it, which is the whole point of the
	// backend. Once that completes, any further datagrams already queued are drained
	// with a non-blocking recvmmsg so a busy socket still returns a full batch rather
	// than one packet per round trip.
	//
	// Multishot receive with a provided-buffer ring would remove the drain syscall
	// entirely, but it delivers many completions per submission, which the operation
	// model here (one UringOperation per awaited completion, living on the coroutine
	// frame) cannot express. That is a change to the operation lifetime, not to this
	// interface, and it is worth making only once a benchmark asks for it.

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

		// Pulls the local address and ECN codepoint out of the control messages.
		// Without IP_PKTINFO a wildcard-bound server cannot reply from the address the
		// client sent to, and a QUIC client discards a reply from anywhere else.
		void read_control(msghdr& hdr, Datagram& out, uint16_t port)
		{
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
					local.sin_port = htons(port);
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
	}  // namespace

	class UringDatagramSocket : public DatagramSocket
	{
		using RecvControl = std::array<std::uint8_t, CMSG_SPACE(sizeof(in_pktinfo)) + CMSG_SPACE(sizeof(int))>;

		UringContext& ctx_;
		int fd_ = -1;
		uint16_t port_ = 0;
		bool gso_ = false;

		// The ring this socket's operations are submitted to, and therefore the worker
		// whose thread will process their completions.
		//
		// Not always ring 0. QUIC sharding gives every worker its own socket on one
		// shared UDP port, and if all of them submitted to ring 0 then one thread would
		// perform every receive and every send for the whole server. The sockets would
		// be split and the work would not be.
		size_t ring_index_ = 0;

		// Receive scratch, reused across calls. The Datagram spans handed back point
		// straight into buffers_, so they are valid only until the next receive.
		std::vector<std::array<std::uint8_t, kDatagramBufSize>> buffers_{kDatagramBatch};
		std::vector<RecvControl> control_{kDatagramBatch};
		std::vector<mmsghdr> msgs_{kDatagramBatch};
		std::vector<iovec> iovs_{kDatagramBatch};
		std::vector<sockaddr_storage> peers_{kDatagramBatch};
		std::vector<Datagram> out_{kDatagramBatch};

	public:
		explicit UringDatagramSocket(UringContext& ctx, size_t worker_index = 0)
		    : ctx_(ctx), ring_index_(worker_index)
		{
		}

		~UringDatagramSocket() override { close(); }

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

			if (::setsockopt(fd_, IPPROTO_IP, IP_PKTINFO, &opt, sizeof(opt)) < 0)
			{
				auto err = Error::system(std::error_code(errno, std::system_category()));
				close();
				return unexpected(err);
			}

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
				::close(fd_);
				fd_ = -1;
			}
		}

		bool is_open() const noexcept override { return fd_ >= 0; }
		uint16_t local_port() const noexcept override { return port_; }
		bool has_segmentation_offload() const noexcept override { return gso_; }

	private:
		void prepare_slot(size_t i)
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
	};

	Task<expected<std::span<const Datagram>, Error>> UringDatagramSocket::async_recv_batch()
	{
		if (!is_open())
		{
			co_return unexpected(Error::io(IoError::InvalidArgument, "socket is closed"));
		}

		for (;;)
		{
			prepare_slot(0);

			UringOperation op{UringOpType::Read};
			int fd = fd_;
			msghdr* hdr0 = &msgs_[0].msg_hdr;

			if (!co_await UringSubmitAwaiter{op, [&]
			                                 {
												 return ctx_.submit_sqe(ring_index_, &op,
				                                                    [fd, hdr0](io_uring_sqe* sqe)
				                                                    { io_uring_prep_recvmsg(sqe, fd, hdr0, 0); });
											 }})
			{
				co_return unexpected(Error::io(IoError::Unknown, "Failed to get SQE"));
			}

			if (op.error)
			{
				co_return unexpected(op.error);
			}
			if (op.result < 0)
			{
				const int err = -op.result;
				if (err == EAGAIN || err == EINTR)
				{
					continue;
				}
				co_return unexpected(Error::system(std::error_code(err, std::system_category())));
			}

			out_[0].data = std::span<const std::uint8_t>(buffers_[0].data(), static_cast<size_t>(op.result));
			store_endpoint(out_[0].peer, reinterpret_cast<const sockaddr*>(msgs_[0].msg_hdr.msg_name),
			               static_cast<socklen_t>(msgs_[0].msg_hdr.msg_namelen));
			read_control(msgs_[0].msg_hdr, out_[0], port_);

			// Drain whatever else is already queued. The socket is non-blocking, so
			// this returns immediately when there is nothing more.
			size_t count = 1;
			for (size_t i = 1; i < kDatagramBatch; ++i)
			{
				prepare_slot(i);
			}

			int extra = ::recvmmsg(fd_, msgs_.data() + 1, static_cast<unsigned>(kDatagramBatch - 1), 0, nullptr);
			if (extra > 0)
			{
				for (size_t i = 1; i <= static_cast<size_t>(extra); ++i)
				{
					out_[i].data = std::span<const std::uint8_t>(buffers_[i].data(), msgs_[i].msg_len);
					store_endpoint(out_[i].peer, reinterpret_cast<const sockaddr*>(msgs_[i].msg_hdr.msg_name),
					               static_cast<socklen_t>(msgs_[i].msg_hdr.msg_namelen));
					read_control(msgs_[i].msg_hdr, out_[i], port_);
				}
				count += static_cast<size_t>(extra);
			}

			co_return std::span<const Datagram>(out_.data(), count);
		}
	}

	Task<expected<size_t, Error>> UringDatagramSocket::async_send(std::span<const std::uint8_t> data,
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
			UringOperation op{UringOpType::Write};
			int fd = fd_;
			msghdr* out = &hdr;

			if (!co_await UringSubmitAwaiter{op, [&]
			                                 {
												 return ctx_.submit_sqe(
													 ring_index_, &op, [fd, out](io_uring_sqe* sqe)
													 { io_uring_prep_sendmsg(sqe, fd, out, MSG_NOSIGNAL); });
											 }})
			{
				co_return unexpected(Error::io(IoError::Unknown, "Failed to get SQE"));
			}

			if (op.error)
			{
				co_return unexpected(op.error);
			}
			if (op.result >= 0)
			{
				co_return static_cast<size_t>(op.result);
			}

			const int err = -op.result;
			if (err != EAGAIN && err != EINTR)
			{
				co_return unexpected(Error::system(std::error_code(err, std::system_category())));
			}
		}
	}

	// ============================================================================
	// Factory Functions
	// ============================================================================

	std::unique_ptr<IoContext> IoContext::create(size_t thread_count)
	{
		return std::make_unique<UringContext>(thread_count);
	}

	std::unique_ptr<Listener> Listener::create(IoContext& ctx)
	{
		return std::make_unique<UringListener>(static_cast<UringContext&>(ctx));
	}

	std::unique_ptr<DatagramSocket> DatagramSocket::create(IoContext& ctx, std::size_t worker_index)
	{
		return std::make_unique<UringDatagramSocket>(static_cast<UringContext&>(ctx), worker_index);
	}

}  // namespace coroute::net

#endif  // coroute_PLATFORM_LINUX
