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
#include <chrono>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>
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

	/// How long to wait for the previous test's io_uring rings to be given back.
	///
	/// io_uring charges its ring memory against RLIMIT_MEMLOCK, and two things about
	/// that accounting are not what the limit's name suggests. It is charged **per
	/// user**, not per process: measured on Linux 7.2 by creating 8192-entry rings
	/// until ENOMEM, one process alone gets nine, and a second process started while
	/// the first holds five gets exactly four. And it is released **asynchronously**,
	/// 7 to 14 ms after the holding process exits.
	///
	/// ctest starts the next test in far less than 7 ms, so consecutive io_uring tests
	/// overlap in accounting while never overlapping in time. That is why this is not a
	/// parallelism bug and why serialising the arms does not fix it: at -j1, with no
	/// concurrency at all, two of the io_uring cases failed on every run. Inserting a
	/// gap does fix it, which is the experiment that identified the cause. Running the
	/// four io_uring cases back to back, three repetitions:
	///
	///     gap   0 ms  ->  8 failures / 12
	///     gap  50 ms  ->  1 failure  / 12
	///     gap 200 ms  ->  0 failures / 12
	///
	/// 250 ms is that ladder plus margin. The wait is spent here, in the tests, rather
	/// than by retrying inside ring init, because ring init is the code under
	/// measurement and adding a retry to it would change the startup behaviour of the
	/// binary the campaign is timing. A retry there may be right for a production
	/// server on a busy host; that is a separate decision, deliberately not taken here.
	constexpr int kMemlockSettleMs = 250;
	constexpr int kMemlockStepMs = 5;

	/// Builds a context on `backend`, or skips this run when the host refuses it.
	///
	/// Skips only where the host will never allow this backend. Any other errno is a
	/// failure: EMFILE means the suite is leaking descriptors and EINVAL means the probe
	/// itself is wrong, and both are defects that a skip would hide behind a green run
	/// for as long as the hardened-kernel skip is also expected.
	///
	/// ENOMEM is the exception. On this path it rarely means the machine is out of
	/// memory; it usually means the previous test's rings have not been handed back yet,
	/// so it is waited out rather than failed at once. That applies to the probe as well
	/// as to the construction: io_backend_probe() creates and destroys a ring of its own,
	/// and on a starved budget it is as likely to be the thing that fails first.
	///
	/// The retry does not inspect the failure, because it cannot. Construction reports
	/// failure as a runtime_error carrying a message and no errno, so this retries any
	/// ring-init failure for kMemlockSettleMs. What keeps that honest is the bound: the
	/// last error is rethrown when the budget runs out, so a persistent failure is still
	/// loud and still carries its own message. It just arrives 250 ms later.
	inline std::unique_ptr<net::IoContext> context_or_skip(std::size_t threads, net::IoBackend backend)
	{
		const int err = net::io_backend_probe(backend);
		if (err != 0)
		{
			// Asked of the build rather than read off the errno: a real kernel or
			// seccomp ENOSYS is not the same thing as an arm that was never compiled in.
			const bool absent = !net::io_backend_compiled_in(backend);
			std::string reason = std::string("backend ") + net::io_backend_name(backend) + " is ";
			reason += absent ? "not compiled into this binary" : "unavailable on this host";
			reason += ": ";
			reason += std::strerror(err);
			if (err == EPERM)
			{
				reason += " (EPERM; check kernel.io_uring_disabled)";
			}
			if (err == EPERM || err == EACCES || absent)
			{
				SKIP(reason);
			}
			// ENOMEM here is the memlock budget, not a broken host, so it falls through
			// to the retry below rather than failing on the spot. Failing here made the
			// probe the one place the wait could not help.
			if (err != ENOMEM)
			{
				FAIL(reason);
			}
		}
		for (int waited = 0;; waited += kMemlockStepMs)
		{
			try
			{
				return net::IoContext::create(threads, backend);
			}
			catch (const std::runtime_error&)
			{
				if (waited >= kMemlockSettleMs)
				{
					throw;
				}
				std::this_thread::sleep_for(std::chrono::milliseconds(kMemlockStepMs));
			}
		}
	}

}  // namespace coroute::testing
