#pragma once

#include <chrono>
#include <ctime>

namespace snap::utils::time {

// A monotonically increasing clock that will return the time since boot, including
// time spent while the device is asleep.
//
// This clock reads from CLOCK_MONOTONIC_RAW.
//
// On iOS, this clock is equivalent to mach_continuous_time.
// On Android, this clock is equivalent to System.elapsedRealtime().
//
// This class conforms to the TrivialClock standard.
class BoottimeClock {
public:
    using duration = std::chrono::nanoseconds;
    using rep = duration::rep;
    using period = duration::period;
    using time_point = std::chrono::time_point<BoottimeClock, duration>;

    static constexpr bool is_steady = true; // NOLINT

    static time_point now() noexcept;
};

} // namespace snap::utils::time
