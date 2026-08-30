#include "coroute/net/io_context.hpp"
#include "coroute/net/datagram.hpp"

#include <stdexcept>
#include <string>

namespace coroute::net
{

	// Provided by the backend translation units that are linked into this binary.
#if defined(COROUTE_PLATFORM_LINUX)
	std::unique_ptr<IoContext> create_epoll_context(size_t thread_count);
	std::unique_ptr<Listener> create_epoll_listener(IoContext& ctx);
	std::unique_ptr<DatagramSocket> create_epoll_datagram(IoContext& ctx, std::size_t worker_index);

	std::unique_ptr<IoContext> create_uring_context(size_t thread_count);
	std::unique_ptr<Listener> create_uring_listener(IoContext& ctx);
	std::unique_ptr<DatagramSocket> create_uring_datagram(IoContext& ctx, std::size_t worker_index);
#endif

#if defined(COROUTE_PLATFORM_LINUX)

	std::unique_ptr<IoContext> IoContext::create(size_t thread_count, IoBackend backend)
	{
		if (backend == IoBackend::Default)
		{
			backend = platform_default_io_backend();
		}

		switch (backend)
		{
			case IoBackend::Epoll:
				return create_epoll_context(thread_count);
			case IoBackend::IoUring:
				return create_uring_context(thread_count);
			default:
				throw std::runtime_error(
					std::string("Linux build cannot create I/O backend '") + io_backend_name(backend) + "'");
		}
	}

	std::unique_ptr<Listener> Listener::create(IoContext& ctx)
	{
		const auto name = ctx.backend_name();
		if (name == "epoll")
		{
			return create_epoll_listener(ctx);
		}
		if (name == "io_uring")
		{
			return create_uring_listener(ctx);
		}
		throw std::logic_error("Listener::create: unknown Linux backend");
	}

	std::unique_ptr<DatagramSocket> DatagramSocket::create(IoContext& ctx, std::size_t worker_index)
	{
		const auto name = ctx.backend_name();
		if (name == "epoll")
		{
			return create_epoll_datagram(ctx, worker_index);
		}
		if (name == "io_uring")
		{
			return create_uring_datagram(ctx, worker_index);
		}
		throw std::logic_error("DatagramSocket::create: unknown Linux backend");
	}

#endif

}  // namespace coroute::net
