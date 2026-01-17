#pragma once

#include <array>
#include <chrono>
#include <fmt/format.h>
#include <string>

namespace snap::utils::time {

/**
 * Helper wrapper class around std::chrono::duration which exposes
 * easy conversion methods as well a human readable fmt/format support.
 *
 * Duration can be divided or multiplied by an amount in integer or double.
 * It can be used for instance to calculate, given a total duration for N
 * iterations, an average duration time by dividing the duration by N.
 *
 */
template<class Clock>
class Duration {
public:
    constexpr explicit Duration(typename Clock::duration duration) : _duration(duration) {}
    constexpr Duration() : _duration(Clock::duration::zero()) {}

    Duration operator+(const Duration& operand) const {
        return Duration(_duration + operand._duration);
    }

    Duration& operator+=(const Duration& operand) {
        _duration += operand._duration;
        return *this;
    }

    Duration operator-(const Duration& operand) const {
        return Duration(_duration - operand._duration);
    }

    Duration& operator-=(const Duration& operand) {
        _duration -= operand._duration;
        return *this;
    }

    template<class T>
    Duration operator/(const T& amount) const {
        return Duration(_duration / amount);
    }

    template<class T>
    Duration operator*(const T& amount) const {
        return Duration(_duration * amount);
    }

    template<class T>
    Duration& operator/=(const T& amount) {
        _duration /= amount;
        return *this;
    }

    template<class T>
    Duration& operator*=(const T& amount) {
        _duration *= amount;
        return *this;
    }

    constexpr bool operator==(const Duration& rhs) const {
        return _duration == rhs._duration;
    }
    constexpr bool operator!=(const Duration& rhs) const {
        return _duration != rhs._duration;
    }
    constexpr bool operator<(const Duration& rhs) const {
        return _duration < rhs._duration;
    }
    constexpr bool operator<=(const Duration& rhs) const {
        return _duration <= rhs._duration;
    }
    constexpr bool operator>(const Duration& rhs) const {
        return _duration > rhs._duration;
    }
    constexpr bool operator>=(const Duration& rhs) const {
        return _duration >= rhs._duration;
    }

    double seconds() const {
        return convert<double, std::chrono::seconds>();
    }

    double milliseconds() const {
        return convert<double, std::chrono::milliseconds>();
    }

    double microseconds() const {
        return convert<double, std::chrono::microseconds>();
    }

    double nanoseconds() const {
        return convert<double, std::chrono::nanoseconds>();
    }

    typename Clock::duration chrono() const {
        return _duration;
    }

    // automatic cast to any time unit
    template<class T>
    constexpr T as() const {
        return std::chrono::duration_cast<T>(_duration);
    }

    std::string toString() const {
        static std::array<std::pair<std::string_view, double>, 4> units = {
            {{"ns", 1.0}, {"us", pow(1000, 1)}, {"ms", pow(1000, 2)}, {"s", pow(1000, 3)}}};
        static auto log1000 = log(1000);

        auto ns = nanoseconds();

        auto unitIndex = ns != 0.0 ? std::min(static_cast<size_t>(log(ns) / log1000), units.size() - 1) : 0;

        const auto& unit = units.at(unitIndex);
        auto remain = ns / unit.second;
        return fmt::format("{:.1f} {}", remain, unit.first);
    }

private:
    typename Clock::duration _duration;

    template<typename Rep, typename Ratio>
    Rep convert() const {
        using TargetTimeUnit = std::chrono::duration<Rep, typename Ratio::period>;
        return std::chrono::duration_cast<TargetTimeUnit>(_duration).count();
    }
};

} // namespace snap::utils::time

// Formatter for snap::utils::time::Duration
template<typename T>
struct fmt::formatter<snap::utils::time::Duration<T>> : fmt::formatter<std::string> {
    template<typename FormatContext>
    auto format(const snap::utils::time::Duration<T>& duration, FormatContext& ctx) {
        return fmt::formatter<std::string>::format(duration.toString(), ctx);
    }
};
