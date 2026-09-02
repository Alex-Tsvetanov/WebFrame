#pragma once

// The socket options every accepted connection gets, in one place.
//
// They were not in one place, and the difference was not visible in any number the
// harness recorded. io_uring set TCP_NODELAY, TCP_QUICKACK and a 256KB send buffer;
// epoll set TCP_NODELAY only; kqueue and IOCP set none of the three, so the macOS and
// Windows arms ran with Nagle enabled while Linux did not. A backend comparison across
// that is partly a comparison of socket options rather than of I/O mechanisms, which is
// the one thing these measurements exist to isolate.
//
// io_uring's set was adopted as the shared policy rather than the smallest common one,
// because io_uring's filed within-arm history stays behaviourally continuous that way;
// under a NODELAY-only policy every io_uring measurement already taken would become a
// pre-policy baseline.
//
// Two consequences a reader should be told rather than left to discover, since both
// weaken what the options appear to promise:
//
//   * Setting SO_SNDBUF explicitly turns OFF the kernel's send-buffer autotuning for
//     that socket. A fixed 256KB replaces a buffer the kernel would otherwise grow and
//     shrink with the connection, so it is not simply "more buffer": on a connection
//     that would have settled smaller it is also less adaptive.
//
//   * TCP_QUICKACK on Linux is a transient hint, not a mode. The kernel clears it as
//     the connection proceeds, so setting it once at accept does not disable delayed
//     acknowledgement for the connection's life. It is applied here at accept only, and
//     that is deliberately all it claims to be.

#ifdef _WIN32
	#include <winsock2.h>
#endif

namespace coroute::net
{

	/// The platform's accepted-socket handle: a descriptor everywhere except Windows.
	#ifdef _WIN32
	using NativeSocket = SOCKET;
	#else
	using NativeSocket = int;
	#endif

	/// Applies the shared accepted-socket policy to `fd`.
	///
	/// Called once per accepted connection, before the connection object reaches a
	/// handler. Every failure is ignored on purpose: each option is an optimisation and
	/// none is required for correctness, so a kernel that refuses one should serve the
	/// connection rather than drop it. Platform obligations that are not tuning, such as
	/// SO_NOSIGPIPE on kqueue and SO_UPDATE_ACCEPT_CONTEXT on IOCP, stay with their
	/// backends; this is only the policy the backends are supposed to share.
	void configure_accepted_socket(NativeSocket fd) noexcept;

}  // namespace coroute::net
