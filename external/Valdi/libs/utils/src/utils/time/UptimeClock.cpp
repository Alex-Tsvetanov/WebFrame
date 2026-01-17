#include "utils/time/UptimeClock.hpp"

#include <chrono>
#include <ctime>
#include <system_error>

#include <time.h>

namespace snap::utils::time {

using std::chrono::nanoseconds;
using std::chrono::seconds;

UptimeClock::time_point UptimeClock::now() noexcept {
    // This implementation is copied from the LLVM source.
    // https://github.com/llvm/llvm-project/blob/master/libcxx/src/chrono.cpp
    struct timespec tp;
#ifdef __APPLE__
    // This should align with iOS mach_absolute_time.
    // https://opensource.apple.com/source/Libc/Libc-1158.1.2/gen/clock_gettime.c.auto.html
    if (0 != clock_gettime(CLOCK_UPTIME_RAW, &tp)) {
        std::__throw_system_error(errno, "clock_gettime(CLOCK_UPTIME_RAW) failed");
    }
#else
    // This should match the Android source for uptimeMillis.
    // https://cs.android.com/android/platform/superproject/+/master:system/core/libutils/SystemClock.cpp
    if (0 != clock_gettime(CLOCK_MONOTONIC, &tp)) {
        std::__throw_system_error(errno, "clock_gettime(CLOCK_MONOTONIC) failed");
    }
#endif
    return time_point(seconds(tp.tv_sec) + nanoseconds(tp.tv_nsec));
}

} // namespace snap::utils::time
