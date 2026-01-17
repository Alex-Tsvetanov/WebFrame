//
//  TimePoint.hpp
//  valdi-pc
//
//  Created by Simon Corsin on 6/30/20.
//

#pragma once

#include <chrono>
#include <cstdlib>
#include <ostream>

#include "valdi_core/cpp/Utils/Duration.hpp"

namespace Valdi {

class TimePoint {
public:
    TimePoint();
    explicit TimePoint(TimeInterval time);

    Duration durationSince(const TimePoint& other) const;

    static TimePoint now();
    static TimePoint fromSeconds(double seconds);
    static TimePoint fromChrono(const std::chrono::steady_clock::time_point& chronoTimePoint);
    static TimePoint fromNanoSeconds(int64_t nanoSeconds);

    TimeInterval getTime() const;
    int64_t getTimeMs() const;

    bool isEmpty() const;

    Duration operator-(const TimePoint& other) const;
    TimePoint operator+(const Duration& duration) const;
    TimePoint& operator+=(const Duration& duration);

    bool operator<(const TimePoint& other) const;
    bool operator>(const TimePoint& other) const;
    bool operator<=(const TimePoint& other) const;
    bool operator>=(const TimePoint& other) const;
    bool operator==(const TimePoint& other) const;
    bool operator!=(const TimePoint& other) const;

private:
    TimeInterval _time = 0;
};

std::ostream& operator<<(std::ostream& os, const TimePoint& point);

} // namespace Valdi
