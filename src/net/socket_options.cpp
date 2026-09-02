#include "coroute/net/socket_options.hpp"

#ifdef _WIN32
	#include <winsock2.h>
	#include <ws2tcpip.h>
#else
	#include <netinet/in.h>
	#include <netinet/tcp.h>
	#include <sys/socket.h>
#endif

namespace coroute::net
{

	namespace
	{
		// Windows takes the option value as char*, POSIX as void*. One wrapper rather
		// than an #ifdef at each call, so the policy below reads as a list of options
		// and not as a list of platform differences.
		inline void set_opt(NativeSocket fd, int level, int name, int value) noexcept
		{
#ifdef _WIN32
			::setsockopt(fd, level, name, reinterpret_cast<const char*>(&value), sizeof(value));
#else
			::setsockopt(fd, level, name, &value, sizeof(value));
#endif
		}
	}  // namespace

	void configure_accepted_socket(NativeSocket fd) noexcept
	{
		// Return values are deliberately unchecked. See the header: every option here is
		// an optimisation, and a kernel that refuses one should still serve the
		// connection.

		// Send the response as soon as it is written. Without this a small response can
		// wait on Nagle for an acknowledgement that delayed-ack is in no hurry to send.
		set_opt(fd, IPPROTO_TCP, TCP_NODELAY, 1);

#ifdef TCP_QUICKACK
		// Linux only, and transient: the kernel clears it again as the connection
		// proceeds. Applied at accept, which is all it claims to be.
		set_opt(fd, IPPROTO_TCP, TCP_QUICKACK, 1);
#endif

		// 256KB. Note this disables the kernel's send-buffer autotuning for this socket
		// rather than merely raising a ceiling.
		set_opt(fd, SOL_SOCKET, SO_SNDBUF, 256 * 1024);
	}

}  // namespace coroute::net
