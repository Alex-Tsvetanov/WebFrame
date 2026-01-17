//
//  Duration.hpp
//  valdi-pc
//
//  Created by Simon Corsin on 6/30/20.
//

#pragma once

#include <chrono>
#include <cstdlib>
#include <ostream>

namespace Valdi {

using TimeInterval = double;

class Duration {
public:
    constexpr explicit Duration(TimeInterval duration) : _duration(duration) {}
    constexpr Duration() = default;

    double milliseconds() const;
    double seconds() const;

    static Duration fromMilliseconds(double milliseconds);
    static Duration fromSeconds(double seconds);

    Duration& operator-=(const Duration& other);
    Duration& operator+=(const Duration& other);

    bool operator<(const Duration& other) const;
    bool operator>(const Duration& other) const;
    bool operator<=(const Duration& other) const;
    bool operator>=(const Duration& other) const;
    bool operator!=(const Duration& other) const;
    bool operator==(const Duration& other) const;

private:
    TimeInterval _duration = 0;
};

Duration operator+(const Duration& left, const Duration& right);
Duration operator-(const Duration& left, const Duration& right);
Duration operator%(const Duration& left, const Duration& right);

std::ostream& operator<<(std::ostream& os, const Duration& value);

} // namespace Valdi
