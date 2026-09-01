#pragma once

// Which I/O backends a test case runs against.
//
// Under -DCOROUTE_IO_BACKEND=dual one binary contains both Linux backends, so a test
// that builds an IoContext is really two tests. Enumerating them here rather than in
// each test case keeps the set in one place, and keeps the skip rule in one place with
// it.
//
// The skip rule is the point. kernel.io_uring_disabled=1 is an ordinary hardened-kernel
// setting under which io_uring_setup returns EPERM, and on such a host the io_uring arm
// cannot run at all. That is a property of the machine, not a defect in the code, and a
// suite that reported it as a failure would be red on every hardened kernel and would
// train everyone to ignore it. Skipped says the same thing without the false alarm.

#include <catch2/catch_test_macros.hpp>

#include <coroute/net/io_context.hpp>

#include <cerrno>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace coroute::testing
{

	/// The backends to exercise: everything this binary compiled in, or just the one
	/// COROUTE_TEST_IO_BACKEND names.
	///
	/// The environment variable exists so ctest can register one test per backend and
	/// have each report separately, which a loop inside one test case cannot do.
	///
	/// Never empty. A value that names a backend this build lacks still comes back as
	/// that backend, so the run skips with ENOSYS and says which arm was missing,
	/// rather than passing silently having tested nothing.
	inline std::vector<net::IoBackend> io_backend_arms()
	{
		const char* requested = std::getenv("COROUTE_TEST_IO_BACKEND");
		if (requested != nullptr && *requested != '\0')
		{
			net::IoBackend one = net::IoBackend::Default;
			if (!net::parse_io_backend(requested, one))
			{
				// Loud, because the only thing that sets this is the build system, and
				// a typo there would otherwise quietly test the default twice.
				throw std::invalid_argument(
					std::string("COROUTE_TEST_IO_BACKEND is not a backend name: ") + requested);
			}
			return {one};
		}

		std::vector<net::IoBackend> arms;
		for (net::IoBackend backend : {net::IoBackend::IoUring, net::IoBackend::Epoll})
		{
			if (net::io_backend_compiled_in(backend))
			{
				arms.push_back(backend);
			}
		}
		if (arms.empty())
		{
			// IOCP and kqueue: one backend, and Default is the only name it has.
			arms.push_back(net::IoBackend::Default);
		}
		return arms;
	}

	/// Builds a context on `backend`, or skips this run when the host refuses it.
	inline std::unique_ptr<net::IoContext> context_or_skip(std::size_t threads, net::IoBackend backend)
	{
		const int err = net::io_backend_probe(backend);
		if (err != 0)
		{
			std::string reason = std::string("backend ") + net::io_backend_name(backend) + " is ";
			reason += err == ENOSYS ? "not compiled into this binary" : "unavailable on this host";
			reason += ": ";
			reason += std::strerror(err);
			if (err == EPERM)
			{
				reason += " (EPERM; check kernel.io_uring_disabled)";
			}
			SKIP(reason);
		}
		return net::IoContext::create(threads, backend);
	}

}  // namespace coroute::testing
