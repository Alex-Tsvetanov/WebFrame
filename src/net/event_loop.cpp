#include "coroute/net/io_context.hpp"

// Platform-specific includes
#if defined(COROUTE_PLATFORM_WINDOWS)
    // IOCP implementation in iocp/iocp_context.cpp
#elif defined(COROUTE_PLATFORM_LINUX)
    // io_uring implementation in io_uring/uring_context.cpp
#elif defined(COROUTE_PLATFORM_MACOS)
    // kqueue implementation in kqueue/kqueue_context.cpp
#endif

namespace coroute::net {

// Factory implementations are in platform-specific files

} // namespace coroute::net
