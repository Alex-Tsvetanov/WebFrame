#if defined(_WIN32) || defined(_WIN64)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include <mswsock.h>
#include <windows.h>
#endif

#include "coroute/net/io_context.hpp"

#if defined(COROUTE_PLATFORM_WINDOWS)

#include <chrono>
#include <condition_variable>
#include <cstring>
#include <functional>
#include <thread>
#include <vector>
#include <queue>
#include <mutex>
#include <atomic>
#include <memory>
#include <stdexcept>
#include <string>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "mswsock.lib")

namespace coroute::net
{

	// ============================================================================
	// IOCP Operation Types
	// ============================================================================

	enum class IocpOpType
	{
		Accept,
		Read,
		Write,
		Connect
	};

	struct IocpOperation : OVERLAPPED
	{
		IocpOpType type;
		std::coroutine_handle<> continuation;
		Error error;
		DWORD bytes_transferred = 0;

		// For accept operations
		SOCKET accept_socket = INVALID_SOCKET;
		char accept_buffer[2 * (sizeof(sockaddr_in6) + 16)];

		IocpOperation(IocpOpType t) : type(t)
		{
			// Zero out OVERLAPPED
			Internal = 0;
			InternalHigh = 0;
			Offset = 0;
			OffsetHigh = 0;
			hEvent = nullptr;
		}
	};

	// ============================================================================
	// IOCP Context Implementation
	// ============================================================================

	class IocpContext : public IoContext
	{
		HANDLE completion_port_ = nullptr;
		std::vector<std::thread> workers_;
		std::atomic<bool> stopped_{false};
		size_t thread_count_;

		// Timer queue
		struct TimerEntry
		{
			std::chrono::system_clock::time_point expiry;
			std::function<void()> callback;

			bool operator>(const TimerEntry& other) const { return expiry > other.expiry; }
		};

		std::mutex timer_mutex_;
		std::condition_variable timer_cv_;
		std::priority_queue<TimerEntry, std::vector<TimerEntry>, std::greater<TimerEntry>> timers_;
		std::thread timer_thread_;

		// Posted callbacks
		std::mutex callback_mutex_;
		std::queue<std::function<void()>> callbacks_;

	public:
		explicit IocpContext(size_t thread_count) : thread_count_(thread_count)
		{
			// Initialize Winsock
			WSADATA wsa_data;
			int result = WSAStartup(MAKEWORD(2, 2), &wsa_data);
			if (result != 0)
			{
				throw std::runtime_error("WSAStartup failed: " + std::to_string(result));
			}

			// Create completion port
			completion_port_ =
				CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, static_cast<DWORD>(thread_count));
			if (!completion_port_)
			{
				WSACleanup();
				throw std::runtime_error("CreateIoCompletionPort failed: " + std::to_string(GetLastError()));
			}

			// Start timer thread
			timer_thread_ = std::thread([this] { timer_thread_proc(); });
		}

		~IocpContext() override
		{
			stop();

			// Wait for workers
			for (auto& worker : workers_)
			{
				if (worker.joinable())
				{
					worker.join();
				}
			}

			if (completion_port_)
			{
				CloseHandle(completion_port_);
			}

			WSACleanup();

			if (timer_thread_.joinable())
			{
				{
					std::lock_guard lock(timer_mutex_);
					stopped_ = true;
				}
				timer_cv_.notify_all();
				timer_thread_.join();
			}
		}

		HANDLE handle() const noexcept { return completion_port_; }

		void associate(HANDLE h) { CreateIoCompletionPort(h, completion_port_, 0, 0); }

		void run() override
		{
			stopped_ = false;

			// Start worker threads
			for (size_t i = 0; i < thread_count_; ++i)
			{
				workers_.emplace_back([this] { worker_thread(); });
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

		void run_one() override { process_one_completion(INFINITE); }

		void stop() override
		{
			stopped_ = true;

			// Post completion to wake up workers
			for (size_t i = 0; i < thread_count_; ++i)
			{
				PostQueuedCompletionStatus(completion_port_, 0, 0, nullptr);
			}

			timer_cv_.notify_all();
		}

		bool stopped() const noexcept override { return stopped_; }

		void post(std::function<void()> callback) override
		{
			{
				std::lock_guard lock(callback_mutex_);
				callbacks_.push(std::move(callback));
			}
			// Wake up a worker
			PostQueuedCompletionStatus(completion_port_, 0, 1, nullptr);
		}

		void schedule(std::chrono::milliseconds delay, std::function<void()> callback) override
		{
			auto expiry = std::chrono::system_clock::now() + delay;
			{
				std::lock_guard lock(timer_mutex_);
				timers_.push({expiry, std::move(callback)});
			}
			timer_cv_.notify_all();
		}

		// Bind a UDP socket to `port` and associate it with this completion
		// port. Defined out-of-line, after IocpUdpSocket.
		expected<std::unique_ptr<UdpSocket>, Error> bind_udp(uint16_t port) override;

	private:
		void timer_thread_proc()
		{
			while (!stopped_)
			{
				std::unique_lock lock(timer_mutex_);
				if (timers_.empty())
				{
					timer_cv_.wait(lock, [this] { return stopped_ || !timers_.empty(); });
				}
				else
				{
					auto now = std::chrono::system_clock::now();
					auto& top = timers_.top();
					if (top.expiry <= now)
					{
						auto cb = std::move(const_cast<TimerEntry&>(top).callback);
						timers_.pop();
						lock.unlock();
						post(std::move(cb));
					}
					else
					{
						timer_cv_.wait_until(lock, top.expiry);
					}
				}
			}
		}

		void worker_thread()
		{
			while (!stopped_)
			{
				process_one_completion(100);  // 100ms timeout to check stopped flag
			}
		}

		void process_one_completion(DWORD timeout)
		{
			DWORD bytes_transferred = 0;
			ULONG_PTR completion_key = 0;
			OVERLAPPED* overlapped = nullptr;

			BOOL success =
				GetQueuedCompletionStatus(completion_port_, &bytes_transferred, &completion_key, &overlapped, timeout);

			if (!overlapped)
			{
				if (completion_key == 1)
				{
					// Posted callback
					std::function<void()> callback;
					{
						std::lock_guard lock(callback_mutex_);
						if (!callbacks_.empty())
						{
							callback = std::move(callbacks_.front());
							callbacks_.pop();
						}
					}
					if (callback)
					{
						callback();
					}
				}
				return;
			}

			auto* op = static_cast<IocpOperation*>(overlapped);

			if (!success)
			{
				DWORD error = GetLastError();
				op->error = Error::system(std::error_code(static_cast<int>(error), std::system_category()));
			}

			op->bytes_transferred = bytes_transferred;

			// Resume the coroutine
			if (op->continuation)
			{
				op->continuation.resume();
			}
		}
	};

	// ============================================================================
	// IOCP Listener Implementation
	// ============================================================================

	class IocpListener : public Listener
	{
		IocpContext& ctx_;
		SOCKET listen_socket_ = INVALID_SOCKET;
		uint16_t port_ = 0;
		LPFN_ACCEPTEX AcceptEx_ = nullptr;
		LPFN_GETACCEPTEXSOCKADDRS GetAcceptExSockaddrs_ = nullptr;

	public:
		explicit IocpListener(IocpContext& ctx) : ctx_(ctx) { }

		~IocpListener() override { close(); }

		expected<void, Error> listen(uint16_t port, int backlog) override
		{
			// Create IPv6 socket (dual-stack: accepts both IPv4 and IPv6)
			listen_socket_ = WSASocketW(AF_INET6, SOCK_STREAM, IPPROTO_TCP, nullptr, 0, WSA_FLAG_OVERLAPPED);
			if (listen_socket_ == INVALID_SOCKET)
			{
				return unexpected(Error::system(std::error_code(WSAGetLastError(), std::system_category())));
			}

			// Disable IPV6_V6ONLY to allow IPv4 connections on this socket (dual-stack)
			DWORD v6only = 0;
			setsockopt(listen_socket_, IPPROTO_IPV6, IPV6_V6ONLY, reinterpret_cast<const char*>(&v6only),
			           sizeof(v6only));

			// Associate with IOCP
			ctx_.associate(reinterpret_cast<HANDLE>(listen_socket_));

			// Load AcceptEx function
			GUID accept_ex_guid = WSAID_ACCEPTEX;
			DWORD bytes = 0;
			int result = WSAIoctl(listen_socket_, SIO_GET_EXTENSION_FUNCTION_POINTER, &accept_ex_guid,
			                      sizeof(accept_ex_guid), &AcceptEx_, sizeof(AcceptEx_), &bytes, nullptr, nullptr);
			if (result == SOCKET_ERROR)
			{
				close();
				return unexpected(Error::system(std::error_code(WSAGetLastError(), std::system_category())));
			}

			// Load GetAcceptExSockaddrs
			GUID get_addrs_guid = WSAID_GETACCEPTEXSOCKADDRS;
			result =
				WSAIoctl(listen_socket_, SIO_GET_EXTENSION_FUNCTION_POINTER, &get_addrs_guid, sizeof(get_addrs_guid),
			             &GetAcceptExSockaddrs_, sizeof(GetAcceptExSockaddrs_), &bytes, nullptr, nullptr);
			if (result == SOCKET_ERROR)
			{
				close();
				return unexpected(Error::system(std::error_code(WSAGetLastError(), std::system_category())));
			}

			// Bind to IPv6 any address (dual-stack accepts IPv4 too)
			sockaddr_in6 addr{};
			addr.sin6_family = AF_INET6;
			addr.sin6_addr = in6addr_any;
			addr.sin6_port = htons(port);

			if (bind(listen_socket_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR)
			{
				close();
				return unexpected(Error::system(std::error_code(WSAGetLastError(), std::system_category())));
			}

			// Listen
			if (::listen(listen_socket_, backlog) == SOCKET_ERROR)
			{
				close();
				return unexpected(Error::system(std::error_code(WSAGetLastError(), std::system_category())));
			}

			// Get actual port (in case port was 0)
			sockaddr_in6 bound_addr{};
			int addr_len = sizeof(bound_addr);
			getsockname(listen_socket_, reinterpret_cast<sockaddr*>(&bound_addr), &addr_len);
			port_ = ntohs(bound_addr.sin6_port);

			return {};
		}

		Task<AcceptResult> async_accept() override;

		void close() override
		{
			if (listen_socket_ != INVALID_SOCKET)
			{
				closesocket(listen_socket_);
				listen_socket_ = INVALID_SOCKET;
			}
		}

		bool is_listening() const noexcept override { return listen_socket_ != INVALID_SOCKET; }

		uint16_t local_port() const noexcept override { return port_; }
	};

	// ============================================================================
	// IOCP Connection Implementation
	// ============================================================================

	class IocpConnection : public Connection
	{
		IocpContext& ctx_;
		SOCKET socket_;
		std::chrono::milliseconds timeout_{30000};
		CancellationToken cancel_token_;
		std::string remote_addr_;
		uint16_t remote_port_ = 0;

		// Buffered reading for async_read_until
		std::vector<char> buffer_;
		size_t buffer_pos_ = 0;
		size_t buffer_end_ = 0;

	public:
		IocpConnection(IocpContext& ctx, SOCKET socket) : ctx_(ctx), socket_(socket)
		{
			ctx_.associate(reinterpret_cast<HANDLE>(socket_));

			// Get remote address
			sockaddr_storage addr{};
			int addr_len = sizeof(addr);
			if (getpeername(socket_, reinterpret_cast<sockaddr*>(&addr), &addr_len) == 0)
			{
				char ip[INET6_ADDRSTRLEN];
				if (addr.ss_family == AF_INET)
				{
					auto* s4 = reinterpret_cast<sockaddr_in*>(&addr);
					inet_ntop(AF_INET, &s4->sin_addr, ip, sizeof(ip));
					remote_addr_ = ip;
					remote_port_ = ntohs(s4->sin_port);
				}
				else if (addr.ss_family == AF_INET6)
				{
					auto* s6 = reinterpret_cast<sockaddr_in6*>(&addr);
					inet_ntop(AF_INET6, &s6->sin6_addr, ip, sizeof(ip));
					remote_addr_ = ip;
					remote_port_ = ntohs(s6->sin6_port);
				}
			}
		}

		~IocpConnection() override { close(); }

		Task<ReadResult> async_read(void* buffer, size_t len) override;
		Task<ReadResult> async_read_until(void* buffer, size_t len, char delimiter) override;
		Task<WriteResult> async_write(const void* buffer, size_t len) override;
		Task<WriteResult> async_write_all(const void* buffer, size_t len) override;
		Task<TransmitResult> async_transmit_file(FileHandle file, size_t offset, size_t length) override;

		void close() override
		{
			if (socket_ != INVALID_SOCKET)
			{
				closesocket(socket_);
				socket_ = INVALID_SOCKET;
			}
		}

		bool is_open() const noexcept override { return socket_ != INVALID_SOCKET; }

		void set_timeout(std::chrono::milliseconds timeout) override { timeout_ = timeout; }

		std::string remote_address() const override { return remote_addr_; }

		uint16_t remote_port() const noexcept override { return remote_port_; }

		void set_cancellation_token(CancellationToken token) override { cancel_token_ = std::move(token); }

		SOCKET socket() const noexcept { return socket_; }
	};

	// ============================================================================
	// Async Operation Awaiters - Proper coroutine integration with IOCP
	// ============================================================================

	// Generic IOCP awaiter that properly wires coroutine handle to completion
	template <typename ResultT>
	struct IocpAwaiter
	{
		IocpOperation* op_;
		std::function<ResultT()> get_result_;

		IocpAwaiter(IocpOperation* op, std::function<ResultT()> get_result)
			: op_(op), get_result_(std::move(get_result))
		{
		}

		bool await_ready() const noexcept
		{
			// Check if operation completed synchronously
			return false;
		}

		void await_suspend(std::coroutine_handle<> h)
		{
			// Store the coroutine handle - IOCP completion will resume it
			op_->continuation = h;
		}

		ResultT await_resume() { return get_result_(); }
	};

	// Accept awaiter - uses heap-allocated operation to ensure it survives until completion
	struct AcceptAwaiter
	{
		IocpContext& ctx_;
		std::unique_ptr<IocpOperation> op_;
		SOCKET listen_socket_;
		LPFN_ACCEPTEX AcceptEx_;

		AcceptAwaiter(IocpContext& ctx, SOCKET listen_socket, LPFN_ACCEPTEX accept_ex)
			: ctx_(ctx),
			  op_(std::make_unique<IocpOperation>(IocpOpType::Accept)),
			  listen_socket_(listen_socket),
			  AcceptEx_(accept_ex)
		{
			// Create accept socket (IPv6 dual-stack to match listener)
			op_->accept_socket = WSASocketW(AF_INET6, SOCK_STREAM, IPPROTO_TCP, nullptr, 0, WSA_FLAG_OVERLAPPED);
		}

		// Non-copyable, non-movable to prevent issues
		AcceptAwaiter(const AcceptAwaiter&) = delete;
		AcceptAwaiter& operator=(const AcceptAwaiter&) = delete;
		AcceptAwaiter(AcceptAwaiter&&) = delete;
		AcceptAwaiter& operator=(AcceptAwaiter&&) = delete;

		bool await_ready() const noexcept { return false; }

		bool await_suspend(std::coroutine_handle<> h)
		{
			op_->continuation = h;

			if (op_->accept_socket == INVALID_SOCKET)
			{
				op_->error = Error::system(std::error_code(WSAGetLastError(), std::system_category()));
				return false;
			}

			DWORD bytes = 0;
			BOOL success = AcceptEx_(listen_socket_, op_->accept_socket, op_->accept_buffer, 0,
			                         sizeof(sockaddr_in6) + 16, sizeof(sockaddr_in6) + 16, &bytes, op_.get());

			if (!success)
			{
				DWORD err = WSAGetLastError();
				if (err != ERROR_IO_PENDING)
				{
					op_->error = Error::system(std::error_code(static_cast<int>(err), std::system_category()));
					return false;
				}
			}

			return true;
		}

		AcceptResult await_resume()
		{
			if (op_->error)
			{
				if (op_->accept_socket != INVALID_SOCKET)
				{
					closesocket(op_->accept_socket);
				}
				return unexpected(op_->error);
			}

			// Update the accept socket to inherit listen socket properties
			setsockopt(op_->accept_socket, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT,
			           reinterpret_cast<char*>(&listen_socket_), sizeof(listen_socket_));

			return std::make_unique<IocpConnection>(ctx_, op_->accept_socket);
		}
	};

	Task<AcceptResult> IocpListener::async_accept()
	{
		if (!is_listening())
		{
			co_return unexpected(Error::io(IoError::InvalidArgument, "Not listening"));
		}

		AcceptAwaiter awaiter(ctx_, listen_socket_, AcceptEx_);
		co_return co_await awaiter;
	}

	// Read awaiter - uses heap-allocated operation
	struct ReadAwaiter
	{
		std::unique_ptr<IocpOperation> op_;
		SOCKET socket_;
		WSABUF wsabuf_;
		DWORD flags_ = 0;

		ReadAwaiter(SOCKET socket, void* buffer, size_t len)
			: op_(std::make_unique<IocpOperation>(IocpOpType::Read)), socket_(socket)
		{
			wsabuf_.buf = static_cast<char*>(buffer);
			wsabuf_.len = static_cast<ULONG>(len);
		}

		ReadAwaiter(const ReadAwaiter&) = delete;
		ReadAwaiter& operator=(const ReadAwaiter&) = delete;
		ReadAwaiter(ReadAwaiter&&) = delete;
		ReadAwaiter& operator=(ReadAwaiter&&) = delete;

		bool await_ready() const noexcept { return false; }

		bool await_suspend(std::coroutine_handle<> h)
		{
			op_->continuation = h;

			int result = WSARecv(socket_, &wsabuf_, 1, nullptr, &flags_, op_.get(), nullptr);

			if (result == SOCKET_ERROR)
			{
				DWORD err = WSAGetLastError();
				if (err != WSA_IO_PENDING)
				{
					op_->error = Error::system(std::error_code(static_cast<int>(err), std::system_category()));
					return false;
				}
			}

			return true;
		}

		ReadResult await_resume()
		{
			if (op_->error)
			{
				return unexpected(op_->error);
			}

			if (op_->bytes_transferred == 0)
			{
				return unexpected(Error::io(IoError::EndOfStream, "Connection closed by peer"));
			}

			return static_cast<size_t>(op_->bytes_transferred);
		}
	};

	Task<ReadResult> IocpConnection::async_read(void* buffer, size_t len)
	{
		if (!is_open())
		{
			co_return unexpected(Error::io(IoError::ConnectionReset, "Connection closed"));
		}

		if (cancel_token_.is_cancelled())
		{
			co_return unexpected(Error::cancelled());
		}

		co_return co_await ReadAwaiter(socket_, buffer, len);
	}

	Task<ReadResult> IocpConnection::async_read_until(void* buffer, size_t len, char delimiter)
	{
		if (!is_open())
		{
			co_return unexpected(Error::io(IoError::ConnectionReset, "Connection closed"));
		}

		char* out = static_cast<char*>(buffer);
		size_t total = 0;

		while (total < len)
		{
			// Check internal buffer first
			if (buffer_pos_ < buffer_end_)
			{
				char c = buffer_[buffer_pos_++];
				out[total++] = c;
				if (c == delimiter)
				{
					co_return total;
				}
				continue;
			}

			// Buffer empty, read more from socket
			if (buffer_.empty())
			{
				buffer_.resize(4096);
			}

			auto result = co_await async_read(buffer_.data(), buffer_.size());
			if (!result)
			{
				co_return unexpected(result.error());
			}

			buffer_pos_ = 0;
			buffer_end_ = *result;

			// Continue with the loop to consume from the newsly filled buffer
		}

		co_return total;
	}

	// Write awaiter - uses heap-allocated operation
	struct WriteAwaiter
	{
		std::unique_ptr<IocpOperation> op_;
		SOCKET socket_;
		WSABUF wsabuf_;

		WriteAwaiter(SOCKET socket, const void* buffer, size_t len)
			: op_(std::make_unique<IocpOperation>(IocpOpType::Write)), socket_(socket)
		{
			wsabuf_.buf = const_cast<char*>(static_cast<const char*>(buffer));
			wsabuf_.len = static_cast<ULONG>(len);
		}

		WriteAwaiter(const WriteAwaiter&) = delete;
		WriteAwaiter& operator=(const WriteAwaiter&) = delete;
		WriteAwaiter(WriteAwaiter&&) = delete;
		WriteAwaiter& operator=(WriteAwaiter&&) = delete;

		bool await_ready() const noexcept { return false; }

		bool await_suspend(std::coroutine_handle<> h)
		{
			op_->continuation = h;

			int result = WSASend(socket_, &wsabuf_, 1, nullptr, 0, op_.get(), nullptr);

			if (result == SOCKET_ERROR)
			{
				DWORD err = WSAGetLastError();
				if (err != WSA_IO_PENDING)
				{
					op_->error = Error::system(std::error_code(static_cast<int>(err), std::system_category()));
					return false;
				}
			}

			return true;
		}

		WriteResult await_resume()
		{
			if (op_->error)
			{
				return unexpected(op_->error);
			}

			return static_cast<size_t>(op_->bytes_transferred);
		}
	};

	Task<WriteResult> IocpConnection::async_write(const void* buffer, size_t len)
	{
		if (!is_open())
		{
			co_return unexpected(Error::io(IoError::ConnectionReset, "Connection closed"));
		}

		if (cancel_token_.is_cancelled())
		{
			co_return unexpected(Error::cancelled());
		}

		co_return co_await WriteAwaiter(socket_, buffer, len);
	}

	Task<WriteResult> IocpConnection::async_write_all(const void* buffer, size_t len)
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

	// TransmitFile awaiter - zero-copy file transfer, uses heap-allocated operation
	struct TransmitFileAwaiter
	{
		std::unique_ptr<IocpOperation> op_;
		SOCKET socket_;
		HANDLE file_;
		LARGE_INTEGER offset_;
		DWORD length_;

		TransmitFileAwaiter(SOCKET socket, HANDLE file, size_t offset, size_t length)
			: op_(std::make_unique<IocpOperation>(IocpOpType::Write)),
			  socket_(socket),
			  file_(file),
			  length_(static_cast<DWORD>(length))
		{
			offset_.QuadPart = static_cast<LONGLONG>(offset);
		}

		TransmitFileAwaiter(const TransmitFileAwaiter&) = delete;
		TransmitFileAwaiter& operator=(const TransmitFileAwaiter&) = delete;
		TransmitFileAwaiter(TransmitFileAwaiter&&) = delete;
		TransmitFileAwaiter& operator=(TransmitFileAwaiter&&) = delete;

		bool await_ready() const noexcept { return false; }

		bool await_suspend(std::coroutine_handle<> h)
		{
			op_->continuation = h;
			op_->Offset = offset_.LowPart;
			op_->OffsetHigh = offset_.HighPart;

			BOOL success = TransmitFile(socket_, file_, length_, 0, op_.get(), nullptr, 0);

			if (!success)
			{
				DWORD err = WSAGetLastError();
				if (err != WSA_IO_PENDING && err != ERROR_IO_PENDING)
				{
					op_->error = Error::system(std::error_code(static_cast<int>(err), std::system_category()));
					return false;
				}
			}

			return true;
		}

		TransmitResult await_resume()
		{
			if (op_->error)
			{
				return unexpected(op_->error);
			}

			return static_cast<size_t>(op_->bytes_transferred);
		}
	};

	Task<TransmitResult> IocpConnection::async_transmit_file(FileHandle file, size_t offset, size_t length)
	{
		if (!is_open())
		{
			co_return unexpected(Error::io(IoError::ConnectionReset, "Connection closed"));
		}

		if (cancel_token_.is_cancelled())
		{
			co_return unexpected(Error::cancelled());
		}

		HANDLE hFile = static_cast<HANDLE>(file);
		if (hFile == INVALID_HANDLE_VALUE)
		{
			co_return unexpected(Error::io(IoError::InvalidArgument, "Invalid file handle"));
		}

		co_return co_await TransmitFileAwaiter(socket_, hFile, offset, length);
	}

	// ============================================================================
	// IOCP UDP Socket Implementation
	//
	// Additive to the existing TCP path above; mirrors ReadAwaiter/WriteAwaiter's
	// simpler style (pre-check cancellation, no live CancelIoEx of an in-flight
	// op — coroute's CancellationToken has no on-cancel unregister handle here).
	// ============================================================================

	// Awaiter for WSARecvFrom.
	struct RecvFromAwaiter
	{
		std::unique_ptr<IocpOperation> op_;
		SOCKET socket_;
		WSABUF wsabuf_;
		DWORD flags_ = 0;
		sockaddr_storage src_addr_{};
		INT src_addr_len_ = sizeof(src_addr_);

		RecvFromAwaiter(SOCKET socket, void* buffer, size_t len)
			: op_(std::make_unique<IocpOperation>(IocpOpType::Read)), socket_(socket)
		{
			wsabuf_.buf = static_cast<char*>(buffer);
			wsabuf_.len = static_cast<ULONG>(len);
		}

		RecvFromAwaiter(const RecvFromAwaiter&) = delete;
		RecvFromAwaiter& operator=(const RecvFromAwaiter&) = delete;
		RecvFromAwaiter(RecvFromAwaiter&&) = delete;
		RecvFromAwaiter& operator=(RecvFromAwaiter&&) = delete;

		bool await_ready() const noexcept { return false; }

		bool await_suspend(std::coroutine_handle<> h)
		{
			op_->continuation = h;

			int result = WSARecvFrom(socket_, &wsabuf_, 1, nullptr, &flags_,
			                         reinterpret_cast<sockaddr*>(&src_addr_), &src_addr_len_,
			                         op_.get(), nullptr);

			if (result == SOCKET_ERROR)
			{
				DWORD err = WSAGetLastError();
				if (err != WSA_IO_PENDING)
				{
					op_->error = Error::system(std::error_code(static_cast<int>(err), std::system_category()));
					return false;
				}
			}

			return true;
		}

		UdpReceiveResult await_resume()
		{
			if (op_->error)
			{
				return unexpected(op_->error);
			}

			UdpEndpoint source;
			char ip[INET6_ADDRSTRLEN] = {};
			if (src_addr_.ss_family == AF_INET)
			{
				auto* s4 = reinterpret_cast<sockaddr_in*>(&src_addr_);
				inet_ntop(AF_INET, &s4->sin_addr, ip, sizeof(ip));
				source.address = ip;
				source.port = ntohs(s4->sin_port);
			}
			else if (src_addr_.ss_family == AF_INET6)
			{
				auto* s6 = reinterpret_cast<sockaddr_in6*>(&src_addr_);
				if (IN6_IS_ADDR_V4MAPPED(&s6->sin6_addr))
				{
					// bind_udp() creates a dual-stack (V6ONLY=0) socket, so an IPv4
					// peer's source address arrives as an IPv4-mapped IPv6 address
					// (::ffff:a.b.c.d). Report it in its plain IPv4 form so callers
					// comparing against a literal IPv4 address (e.g. "127.0.0.1")
					// see what they expect instead of the mapped form.
					in_addr v4{};
					std::memcpy(&v4, &s6->sin6_addr.s6_addr[12], sizeof(v4));
					inet_ntop(AF_INET, &v4, ip, sizeof(ip));
				}
				else
				{
					inet_ntop(AF_INET6, &s6->sin6_addr, ip, sizeof(ip));
				}
				source.address = ip;
				source.port = ntohs(s6->sin6_port);
			}

			return std::make_pair(static_cast<size_t>(op_->bytes_transferred), source);
		}
	};

	// Awaiter for WSASendTo.
	struct SendToAwaiter
	{
		std::unique_ptr<IocpOperation> op_;
		SOCKET socket_;
		WSABUF wsabuf_;
		sockaddr_storage dest_addr_{};
		int dest_addr_len_{};

		SendToAwaiter(SOCKET socket, const void* buffer, size_t len, const UdpEndpoint& dest)
			: op_(std::make_unique<IocpOperation>(IocpOpType::Write)), socket_(socket)
		{
			wsabuf_.buf = const_cast<char*>(static_cast<const char*>(buffer));
			wsabuf_.len = static_cast<ULONG>(len);

			// Match the destination's address family to the SOCKET's family. A
			// pure-IPv4 sockaddr_in passed to WSASendTo on an IPv6 (dual-stack)
			// socket is rejected with WSAEINVAL — bind_udp() below creates
			// IPv6-with-V6ONLY=0 sockets, so an IPv4 literal destination has to
			// be sent as a v4-mapped IPv6 address.
			sockaddr_storage self{};
			int self_len = sizeof(self);
			::getsockname(socket_, reinterpret_cast<sockaddr*>(&self), &self_len);
			const bool sock_is_ipv6 = (self.ss_family == AF_INET6);
			const bool dest_is_ipv6 = (dest.address.find(':') != std::string::npos);

			if (dest_is_ipv6 || sock_is_ipv6)
			{
				auto& a6 = reinterpret_cast<sockaddr_in6&>(dest_addr_);
				a6.sin6_family = AF_INET6;
				a6.sin6_port   = htons(dest.port);
				if (dest_is_ipv6)
				{
					inet_pton(AF_INET6, dest.address.c_str(), &a6.sin6_addr);
				}
				else
				{
					in_addr v4{};
					inet_pton(AF_INET, dest.address.c_str(), &v4);
					std::memset(&a6.sin6_addr, 0, sizeof(a6.sin6_addr));
					a6.sin6_addr.s6_addr[10] = 0xFF;
					a6.sin6_addr.s6_addr[11] = 0xFF;
					std::memcpy(&a6.sin6_addr.s6_addr[12], &v4.s_addr, 4);
				}
				dest_addr_len_ = sizeof(sockaddr_in6);
			}
			else
			{
				auto& a4 = reinterpret_cast<sockaddr_in&>(dest_addr_);
				a4.sin_family = AF_INET;
				a4.sin_port   = htons(dest.port);
				inet_pton(AF_INET, dest.address.c_str(), &a4.sin_addr);
				dest_addr_len_ = sizeof(sockaddr_in);
			}
		}

		SendToAwaiter(const SendToAwaiter&) = delete;
		SendToAwaiter& operator=(const SendToAwaiter&) = delete;
		SendToAwaiter(SendToAwaiter&&) = delete;
		SendToAwaiter& operator=(SendToAwaiter&&) = delete;

		bool await_ready() const noexcept { return false; }

		bool await_suspend(std::coroutine_handle<> h)
		{
			op_->continuation = h;

			int result = WSASendTo(socket_, &wsabuf_, 1, nullptr, 0,
			                       reinterpret_cast<const sockaddr*>(&dest_addr_), dest_addr_len_,
			                       op_.get(), nullptr);

			if (result == SOCKET_ERROR)
			{
				DWORD err = WSAGetLastError();
				if (err != WSA_IO_PENDING)
				{
					op_->error = Error::system(std::error_code(static_cast<int>(err), std::system_category()));
					return false;
				}
			}

			return true;
		}

		UdpSendResult await_resume()
		{
			if (op_->error)
			{
				return unexpected(op_->error);
			}
			return static_cast<size_t>(op_->bytes_transferred);
		}
	};

	class IocpUdpSocket : public UdpSocket
	{
		IocpContext& ctx_;
		SOCKET socket_ = INVALID_SOCKET;
		uint16_t local_port_ = 0;
		CancellationToken cancel_token_;

	public:
		// Two-phase: construct empty, then `bind()` allocates + associates the
		// SOCKET. Mirrors `UdpSocket::create(ctx); sock->bind(0);`.
		explicit IocpUdpSocket(IocpContext& ctx) : ctx_(ctx) { }

		// Direct-bound constructor (used by `IocpContext::bind_udp`).
		IocpUdpSocket(IocpContext& ctx, SOCKET socket, uint16_t port)
			: ctx_(ctx), socket_(socket), local_port_(port)
		{
			ctx_.associate(reinterpret_cast<HANDLE>(socket_));
		}

		~IocpUdpSocket() override { close(); }

		void set_cancellation_token(CancellationToken token) override
		{
			cancel_token_ = std::move(token);
		}

		Task<UdpReceiveResult> async_recv_from(void* buffer, size_t length) override
		{
			if (socket_ == INVALID_SOCKET)
			{
				co_return unexpected(Error::io(IoError::InvalidArgument, "Socket closed"));
			}
			if (cancel_token_.is_cancelled())
			{
				co_return unexpected(Error::cancelled());
			}

			co_return co_await RecvFromAwaiter(socket_, buffer, length);
		}

		Task<UdpSendResult> async_send_to(const void* buffer, size_t length, const UdpEndpoint& dest) override
		{
			if (socket_ == INVALID_SOCKET)
			{
				co_return unexpected(Error::io(IoError::InvalidArgument, "Socket closed"));
			}
			if (cancel_token_.is_cancelled())
			{
				co_return unexpected(Error::cancelled());
			}

			co_return co_await SendToAwaiter(socket_, buffer, length, dest);
		}

		expected<void, Error> bind(uint16_t port) override
		{
			if (socket_ != INVALID_SOCKET)
				return unexpected(Error::io(IoError::InvalidArgument, "Socket already bound"));

			socket_ = WSASocketW(AF_INET, SOCK_DGRAM, IPPROTO_UDP, nullptr, 0, WSA_FLAG_OVERLAPPED);
			if (socket_ == INVALID_SOCKET)
			{
				return unexpected(Error::system(std::error_code(WSAGetLastError(), std::system_category())));
			}

			BOOL yes = TRUE;
			setsockopt(socket_, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&yes), sizeof(yes));

			sockaddr_in addr{};
			addr.sin_family      = AF_INET;
			addr.sin_addr.s_addr = INADDR_ANY;
			addr.sin_port        = htons(port);
			if (::bind(socket_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR)
			{
				const int err = WSAGetLastError();
				closesocket(socket_);
				socket_ = INVALID_SOCKET;
				return unexpected(Error::system(std::error_code(err, std::system_category())));
			}

			sockaddr_in bound{};
			int blen = sizeof(bound);
			::getsockname(socket_, reinterpret_cast<sockaddr*>(&bound), &blen);
			local_port_ = ntohs(bound.sin_port);

			ctx_.associate(reinterpret_cast<HANDLE>(socket_));
			return {};
		}

		expected<void, Error> bind(const std::string& address, uint16_t port) override
		{
			if (socket_ != INVALID_SOCKET)
				return unexpected(Error::io(IoError::InvalidArgument, "Socket already bound"));

			socket_ = WSASocketW(AF_INET, SOCK_DGRAM, IPPROTO_UDP, nullptr, 0, WSA_FLAG_OVERLAPPED);
			if (socket_ == INVALID_SOCKET)
			{
				return unexpected(Error::system(std::error_code(WSAGetLastError(), std::system_category())));
			}

			BOOL yes = TRUE;
			setsockopt(socket_, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&yes), sizeof(yes));

			sockaddr_in addr{};
			addr.sin_family = AF_INET;
			addr.sin_port   = htons(port);
			if (inet_pton(AF_INET, address.c_str(), &addr.sin_addr) != 1)
			{
				closesocket(socket_);
				socket_ = INVALID_SOCKET;
				return unexpected(Error::io(IoError::InvalidArgument, "invalid IPv4 address: " + address));
			}

			if (::bind(socket_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR)
			{
				const int err = WSAGetLastError();
				closesocket(socket_);
				socket_ = INVALID_SOCKET;
				return unexpected(Error::system(std::error_code(err, std::system_category())));
			}

			sockaddr_in bound{};
			int blen = sizeof(bound);
			::getsockname(socket_, reinterpret_cast<sockaddr*>(&bound), &blen);
			local_port_ = ntohs(bound.sin_port);

			ctx_.associate(reinterpret_cast<HANDLE>(socket_));
			return {};
		}

		void close() override
		{
			if (socket_ != INVALID_SOCKET)
			{
				closesocket(socket_);
				socket_ = INVALID_SOCKET;
			}
		}

		bool is_open() const noexcept override { return socket_ != INVALID_SOCKET; }

		uint16_t local_port() const noexcept override { return local_port_; }
	};

	expected<std::unique_ptr<UdpSocket>, Error> IocpContext::bind_udp(uint16_t port)
	{
		// Dual-stack (IPv6, V6ONLY=0) so both IPv4 and IPv6 peers can reach it,
		// matching the TCP listener's dual-stack setup above.
		SOCKET sock = WSASocketW(AF_INET6, SOCK_DGRAM, IPPROTO_UDP, nullptr, 0, WSA_FLAG_OVERLAPPED);
		if (sock == INVALID_SOCKET)
		{
			return unexpected(Error::system(std::error_code(WSAGetLastError(), std::system_category())));
		}

		int opt = 1;
		setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&opt), sizeof(opt));
		DWORD v6only = 0;
		setsockopt(sock, IPPROTO_IPV6, IPV6_V6ONLY, reinterpret_cast<const char*>(&v6only), sizeof(v6only));

		sockaddr_in6 addr{};
		addr.sin6_family = AF_INET6;
		addr.sin6_addr = in6addr_any;
		addr.sin6_port = htons(port);

		if (bind(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR)
		{
			closesocket(sock);
			return unexpected(Error::system(std::error_code(WSAGetLastError(), std::system_category())));
		}

		sockaddr_in6 bound_addr{};
		int addr_len = sizeof(bound_addr);
		getsockname(sock, reinterpret_cast<sockaddr*>(&bound_addr), &addr_len);

		return std::make_unique<IocpUdpSocket>(*this, sock, ntohs(bound_addr.sin6_port));
	}

	std::unique_ptr<UdpSocket> UdpSocket::create(IoContext& ctx)
	{
		return std::make_unique<IocpUdpSocket>(static_cast<IocpContext&>(ctx));
	}

	// ============================================================================
	// Factory Functions
	// ============================================================================

	std::unique_ptr<IoContext> IoContext::create(size_t thread_count)
	{
		return std::make_unique<IocpContext>(thread_count);
	}

	std::unique_ptr<Listener> Listener::create(IoContext& ctx)
	{
		return std::make_unique<IocpListener>(static_cast<IocpContext&>(ctx));
	}

}  // namespace coroute::net

#endif  // coroute_PLATFORM_WINDOWS
