//
//  TimePoint.cpp
//  valdi-pc
//
//  Created by Simon Corsin on 6/30/20.
//

#include "valdi_core/cpp/Utils/TimePoint.hpp"

namespace Valdi {

TimePoint::TimePoint() = default;

TimePoint::TimePoint(TimeInterval time) : _time(time) {}

TimePoint TimePoint::now() {
    return fromChrono(std::chrono::steady_clock::now());
}

TimePoint TimePoint::fromChrono(const std::chrono::steady_clock::time_point& chronoTimePoint) {
    auto time = chronoTimePoint.time_since_epoch();

    return TimePoint(std::chrono::duration<double>(time).count());
}

TimePoint TimePoint::fromSeconds(double seconds) {
    return TimePoint(seconds);
}

TimePoint TimePoint::fromNanoSeconds(int64_t nanoSeconds) {
    auto frameTime = static_cast<double>(nanoSeconds) / 1000000000.0;
    return fromSeconds(frameTime);
}

Duration TimePoint::durationSince(const TimePoint& other) const {
    return Duration(_time - other._time);
}

TimeInterval TimePoint::getTime() const {
    return _time;
}

int64_t TimePoint::getTimeMs() const {
    return static_cast<int64_t>(_time * 1000.0);
}

bool TimePoint::isEmpty() const {
    return _time == 0;
}

Duration TimePoint::operator-(const TimePoint& other) const {
    return Duration(_time - other._time);
}

TimePoint TimePoint::operator+(const Duration& duration) const {
    return TimePoint(_time + duration.seconds());
}

TimePoint& TimePoint::operator+=(const Duration& duration) {
    _time += duration.seconds();

    return *this;
}

bool TimePoint::operator<(const TimePoint& other) const {
    return _time < other._time;
}

bool TimePoint::operator>(const TimePoint& other) const {
    return _time > other._time;
}

bool TimePoint::operator<=(const TimePoint& other) const {
    return _time <= other._time;
}

bool TimePoint::operator>=(const TimePoint& other) const {
    return _time >= other._time;
}

bool TimePoint::operator==(const TimePoint& other) const {
    return _time == other._time;
}

bool TimePoint::operator!=(const TimePoint& other) const {
    return _time != other._time;
}

std::ostream& operator<<(std::ostream& os, const TimePoint& point) {
    return os << "time :" << point.getTime();
}

} // namespace Valdi
