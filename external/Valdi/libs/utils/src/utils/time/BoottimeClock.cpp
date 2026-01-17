#include "utils/time/BoottimeClock.hpp"
#include "utils/platform/TargetPlatform.hpp"
#include <cerrno>
#include <chrono>
#include <ctime>
#include <system_error>

namespace snap::utils::time {

using std::chrono::nanoseconds;
using std::chrono::seconds;

// Remove the implementation of this symbol for wasm. We can let code include the header file, but this will ensure
// nobody tries to reference the symbol. Boot-time is somewhat possible in NodeJS, but specifically in browsers, they
// don't offer visibility into the OS to know when the machine was booted up. UptimeClock is supported however, and
// should meet most needs if the only requirement is a monotonic clock.
#ifndef SC_WASM
BoottimeClock::time_point BoottimeClock::now() noexcept {
    // This implementation is copied from the LLVM source.
    // https://github.com/llvm/llvm-project/blob/master/libcxx/src/chrono.cpp
    struct timespec tp;

#if __APPLE__
    // This should align with iOS mach_continuous_time.
    // https://opensource.apple.com/source/Libc/Libc-1158.1.2/gen/clock_gettime.c.auto.html
    if (0 != clock_gettime(CLOCK_MONOTONIC_RAW, &tp)) {
        std::__throw_system_error(errno, "clock_gettime(CLOCK_MONOTONIC_RAW) failed");
    }
#else
    // This should match the Android source for elapsedRealtime.
    // https://cs.android.com/android/platform/superproject/+/master:system/core/libutils/SystemClock.cpp
    if (0 != clock_gettime(CLOCK_BOOTTIME, &tp)) {
        std::__throw_system_error(errno, "clock_gettime(CLOCK_BOOTTIME) failed");
    }
#endif

    return time_point(seconds(tp.tv_sec) + nanoseconds(tp.tv_nsec));
}
#endif

} // namespace snap::utils::time
