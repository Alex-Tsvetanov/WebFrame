//
//  Duration.cpp
//  valdi-pc
//
//  Created by Simon Corsin on 6/30/20.
//

#include "valdi_core/cpp/Utils/Duration.hpp"
#include <cmath>

namespace Valdi {

double Duration::milliseconds() const {
    return _duration * 1000;
}

double Duration::seconds() const {
    return _duration;
}

Duration Duration::fromMilliseconds(double milliseconds) {
    return Duration(milliseconds / 1000);
}

Duration Duration::fromSeconds(double seconds) {
    return Duration(seconds);
}

Duration& Duration::operator-=(const Duration& other) {
    _duration -= other._duration;
    return *this;
}

Duration& Duration::operator+=(const Duration& other) {
    _duration += other._duration;
    return *this;
}

bool Duration::operator<(const Duration& other) const {
    return _duration < other._duration;
}

bool Duration::operator>(const Duration& other) const {
    return _duration > other._duration;
}

bool Duration::operator<=(const Duration& other) const {
    return _duration <= other._duration;
}

bool Duration::operator>=(const Duration& other) const {
    return _duration >= other._duration;
}

bool Duration::operator!=(const Duration& other) const {
    return _duration != other._duration;
}

bool Duration::operator==(const Duration& other) const {
    return _duration == other._duration;
}

Duration operator+(const Duration& left, const Duration& right) {
    return Duration(left.seconds() + right.seconds());
}

Duration operator-(const Duration& left, const Duration& right) {
    return Duration(left.seconds() - right.seconds());
}

Duration operator%(const Duration& left, const Duration& right) {
    return Duration(std::fmod(left.seconds(), right.seconds()));
}

std::ostream& operator<<(std::ostream& os, const Duration& value) {
    return os << value.seconds() << "s";
}

} // namespace Valdi
