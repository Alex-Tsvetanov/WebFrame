#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <string>

namespace coroute::net
{

	// Shared-memory counters for the I/O path under measurement.
	//
	// Written by the server, read by the harness while the process is still alive.
	// Layout is fixed and little-endian so a Python struct can parse it without a
	// second protocol. Only the operations that are actual syscalls on that backend
	// are incremented as such: an io_uring SQE is not a syscall and must not be
	// counted as one, or the epoll-versus-io_uring comparison invents equality.

	struct IoStatsBlock
	{
		static constexpr std::uint64_t MAGIC = 0x434F524F55544501ull;  // "COROUTE\x01"

		std::atomic<std::uint64_t> magic{0};
		std::atomic<std::uint64_t> epoll_wait{0};
		std::atomic<std::uint64_t> epoll_ctl{0};
		std::atomic<std::uint64_t> io_uring_enter{0};
		std::atomic<std::uint64_t> accept{0};
		std::atomic<std::uint64_t> read{0};
		std::atomic<std::uint64_t> write{0};
		std::atomic<std::uint64_t> sendfile{0};
		// SQE count, not a syscall. Recorded so SEND_ZC traffic is visible without
		// being added into syscalls_total.
		std::atomic<std::uint64_t> send_zc{0};
	};

	inline void bump(std::atomic<std::uint64_t>* counter) noexcept
	{
		if (counter != nullptr)
		{
			counter->fetch_add(1, std::memory_order_relaxed);
		}
	}

	// Maps PATH as a shared IoStatsBlock, creating and zeroing the file. Returns
	// nullptr when the platform has no mmap or the file could not be opened.
	IoStatsBlock* map_io_stats(const std::string& path);

	void unmap_io_stats(IoStatsBlock* block) noexcept;

}  // namespace coroute::net
