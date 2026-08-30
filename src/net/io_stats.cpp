#include "coroute/net/io_stats.hpp"

#include <cstring>
#include <string>

#if defined(COROUTE_PLATFORM_LINUX)
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

namespace coroute::net
{

	IoStatsBlock* map_io_stats(const std::string& path)
	{
#if defined(COROUTE_PLATFORM_LINUX)
		if (path.empty())
		{
			return nullptr;
		}

		const int fd = ::open(path.c_str(), O_RDWR | O_CREAT, 0644);
		if (fd < 0)
		{
			return nullptr;
		}

		if (::ftruncate(fd, static_cast<off_t>(sizeof(IoStatsBlock))) != 0)
		{
			::close(fd);
			return nullptr;
		}

		void* mapped = ::mmap(nullptr, sizeof(IoStatsBlock), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
		::close(fd);
		if (mapped == MAP_FAILED)
		{
			return nullptr;
		}

		std::memset(mapped, 0, sizeof(IoStatsBlock));
		auto* block = new (mapped) IoStatsBlock();
		block->magic.store(IoStatsBlock::MAGIC, std::memory_order_relaxed);
		return block;
#else
		(void)path;
		return nullptr;
#endif
	}

	void unmap_io_stats(IoStatsBlock* block) noexcept
	{
#if defined(COROUTE_PLATFORM_LINUX)
		if (block != nullptr)
		{
			block->~IoStatsBlock();
			::munmap(block, sizeof(IoStatsBlock));
		}
#else
		(void)block;
#endif
	}

}  // namespace coroute::net
