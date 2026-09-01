#include "coroute/net/io_context.hpp"
#include "coroute/net/datagram.hpp"

#include "backend_factory.hpp"

#include <cerrno>
#include <cstring>
#include <stdexcept>
#include <string>

namespace coroute::net
{

	// ============================================================================
	// Backend selection
	// ============================================================================
	//
	// One translation unit, compiled on every platform, holding the three factory
	// entry points the whole tree calls. They used to be defined once per backend, so
	// exactly one backend could be linked and the choice was made by CMake picking
	// which .cpp to compile. Two backends in one binary is the point of the runtime
	// flag, and two definitions of IoContext::create is a link error, so the dispatch
	// moved here and each backend now exposes a maker instead.

	const char* io_backend_name(IoBackend backend) noexcept
	{
		switch (backend)
		{
			case IoBackend::IoUring: return "io_uring";
			case IoBackend::Epoll: return "epoll";
			case IoBackend::Default: break;
		}
		return "default";
	}

	bool parse_io_backend(std::string_view text, IoBackend& out) noexcept
	{
		if (text == "io_uring")
		{
			out = IoBackend::IoUring;
			return true;
		}
		if (text == "epoll")
		{
			out = IoBackend::Epoll;
			return true;
		}
		return false;
	}

	bool io_backend_compiled_in(IoBackend backend) noexcept
	{
		switch (backend)
		{
			case IoBackend::IoUring:
#if defined(COROUTE_BACKEND_IO_URING)
				return true;
#else
				return false;
#endif
			case IoBackend::Epoll:
#if defined(COROUTE_BACKEND_EPOLL)
				return true;
#else
				return false;
#endif
			case IoBackend::Default:
				// Every build has something to fall back on, or it would not link.
				return true;
		}
		return false;
	}

	int io_backend_probe(IoBackend backend) noexcept
	{
		if (!io_backend_compiled_in(backend))
		{
			return ENOSYS;
		}

		switch (backend)
		{
			case IoBackend::IoUring:
#if defined(COROUTE_BACKEND_IO_URING)
				return detail::probe_uring();
#else
				return ENOSYS;
#endif
			case IoBackend::Epoll:
				// epoll_create is not restricted the way io_uring_setup is: there is no
				// sysctl that takes it away from an unprivileged process, so a build that
				// contains it can use it. Nothing is gained by burning a descriptor to
				// confirm that.
				return 0;
			case IoBackend::Default:
				return 0;
		}
		return ENOSYS;
	}

	namespace
	{

		// The backend IoBackend::Default resolves to on this build and this host.
		//
		// On Linux with both arms compiled in this is where the fallback lives, and it
		// is a fallback rather than a failure on purpose: kernel.io_uring_disabled=1 is
		// a perfectly ordinary hardened-kernel setting, and a framework that threw on
		// such a host would be unusable there for the sake of a preference. A caller
		// that needs a specific arm names it and gets an exception instead.
		IoBackend resolve_default() noexcept
		{
#if defined(COROUTE_BACKEND_IO_URING) && defined(COROUTE_BACKEND_EPOLL)
			return detail::probe_uring() == 0 ? IoBackend::IoUring : IoBackend::Epoll;
#elif defined(COROUTE_BACKEND_IO_URING)
			return IoBackend::IoUring;
#elif defined(COROUTE_BACKEND_EPOLL)
			return IoBackend::Epoll;
#else
			// Windows and macOS have exactly one backend each, and Default is the only
			// value that names it.
			return IoBackend::Default;
#endif
		}

		[[noreturn]] void refuse(IoBackend backend, int err)
		{
			std::string message = "I/O backend '";
			message += io_backend_name(backend);
			message += "' ";
			if (err == ENOSYS)
			{
				message += "was not compiled into this binary; configure with "
				           "-DCOROUTE_IO_BACKEND=";
				message += io_backend_name(backend);
				message += " or -DCOROUTE_IO_BACKEND=dual";
			}
			else
			{
				message += "is not available on this host: ";
				message += std::strerror(err);
				if (err == EPERM)
				{
					// Named because it is the one that has a cause an operator can act
					// on, and the strerror text alone ("Operation not permitted") sends
					// people looking at file permissions.
					message += " (EPERM; check kernel.io_uring_disabled and "
					           "kernel.io_uring_group)";
				}
			}
			throw std::runtime_error(message);
		}

	}  // namespace

	std::unique_ptr<IoContext> IoContext::create(size_t thread_count, IoBackend backend)
	{
		if (backend == IoBackend::Default)
		{
			backend = resolve_default();
		}
		else
		{
			// Only an explicit request is checked. Default has already resolved to
			// something this host answered for, and on the single-backend platforms
			// there is nothing to check against.
			const int err = io_backend_probe(backend);
			if (err != 0)
			{
				refuse(backend, err);
			}
		}

		switch (backend)
		{
			case IoBackend::IoUring:
#if defined(COROUTE_BACKEND_IO_URING)
				return detail::make_uring_context(thread_count);
#else
				break;
#endif
			case IoBackend::Epoll:
#if defined(COROUTE_BACKEND_EPOLL)
				return detail::make_epoll_context(thread_count);
#else
				break;
#endif
			case IoBackend::Default:
#if defined(COROUTE_PLATFORM_WINDOWS)
				return detail::make_iocp_context(thread_count);
#elif defined(COROUTE_PLATFORM_MACOS)
				return detail::make_kqueue_context(thread_count);
#else
				break;
#endif
		}

		refuse(backend, ENOSYS);
	}

	// ============================================================================
	// Factory Functions
	// ============================================================================
	//
	// Thin forwarders onto the context's own virtuals, kept because every caller in the
	// tree and in the tests already spells them this way. What they no longer do is
	// downcast the context to the one backend the binary was built with, which was
	// unchecked when that was guaranteed to be the right type and undefined behaviour
	// as soon as it was not.

	std::unique_ptr<DatagramSocket> IoContext::make_datagram_socket(std::size_t worker_index)
	{
		(void)worker_index;
		return nullptr;
	}

	std::unique_ptr<Listener> Listener::create(IoContext& ctx)
	{
		return ctx.make_listener();
	}

	std::unique_ptr<DatagramSocket> DatagramSocket::create(IoContext& ctx, std::size_t worker_index)
	{
		return ctx.make_datagram_socket(worker_index);
	}

}  // namespace coroute::net
