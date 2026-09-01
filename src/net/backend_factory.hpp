#pragma once

// Entry points into the backend translation units.
//
// Internal to src/net. Each backend defines exactly one context class, and that class
// is local to its own .cpp: EpollContext and UringContext are not declared in any
// header, so the dispatcher in io_context.cpp cannot name them. It does not need to.
// It needs one function per backend that returns an IoContext, and this is where those
// are declared.
//
// The alternative was to publish the context classes, which would put <liburing.h> and
// <sys/epoll.h> into a header reachable from most of the tree for the sake of a single
// call site.

#include <cstddef>
#include <memory>

#include "coroute/net/io_context.hpp"

namespace coroute::net::detail
{

#if defined(COROUTE_BACKEND_EPOLL)
	std::unique_ptr<IoContext> make_epoll_context(std::size_t thread_count);
#endif

#if defined(COROUTE_BACKEND_IO_URING)
	std::unique_ptr<IoContext> make_uring_context(std::size_t thread_count);

	// 0 when this kernel will give this process a ring, otherwise the errno it refused
	// with. See the definition in uring_context.cpp for why it has to ask rather than
	// infer.
	int probe_uring() noexcept;
#endif

#if defined(COROUTE_PLATFORM_WINDOWS)
	std::unique_ptr<IoContext> make_iocp_context(std::size_t thread_count);
#endif

#if defined(COROUTE_PLATFORM_MACOS)
	std::unique_ptr<IoContext> make_kqueue_context(std::size_t thread_count);
#endif

}  // namespace coroute::net::detail
