#pragma once

#include <chrono>
#include <ctime>

namespace snap::utils::time {

// A monotonically increasing clock that will return the time since boot, excluding
// time spent while the device is asleep.
//
// This clock reads from CLOCK_UPTIME_RAW.
//
// On iOS, this clock is equivalent to mach_absolute_time and CACurrentMediaTime.
// On Android, this clock is equivalent to System.uptimeMillis().
//
// This class conforms to the TrivialClock standard.
class UptimeClock {
public:
    using duration = std::chrono::nanoseconds;
    using rep = duration::rep;
    using period = duration::period;
    using time_point = std::chrono::time_point<UptimeClock, duration>;

    static constexpr bool is_steady = true; // NOLINT

    static time_point now() noexcept;
};

} // namespace snap::utils::time
