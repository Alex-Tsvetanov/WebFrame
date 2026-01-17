//
//  Color.hpp
//  snap_drawing
//
//  Created by Simon Corsin on 1/28/22.
//

#pragma once

#include "include/core/SkColor.h"
#include "snap_drawing/cpp/Utils/Scalar.hpp"
#include "snap_drawing/cpp/Utils/SkiaBridge.hpp"

#include <ostream>
#include <string>

namespace snap::drawing {

using ColorComponent = uint8_t;

struct Color : public BridgedSkValue<Color, SkColor> {
    uint32_t value = 0;

    constexpr Color() = default;
    constexpr explicit Color(uint32_t value) : value(value) {}

    std::string toString() const;

    constexpr ColorComponent getAlpha() const {
        return (value >> 24) & 0xFF;
    }

    constexpr ColorComponent getRed() const {
        return (value >> 16) & 0xFF;
    }

    constexpr ColorComponent getGreen() const {
        return (value >> 8) & 0xFF;
    }

    constexpr ColorComponent getBlue() const {
        return value & 0xFF;
    }

    constexpr Color withAlpha(ColorComponent a) const {
        return Color::makeARGB(a, getRed(), getGreen(), getBlue());
    }

    constexpr Color withAlphaRatio(Scalar alphaRatio) const {
        return withAlpha(static_cast<ColorComponent>(static_cast<Scalar>(0xFF) * alphaRatio));
    }

    static constexpr Color make(uint32_t value) {
        return Color(value);
    }

    static constexpr Color makeARGB(ColorComponent a, ColorComponent r, ColorComponent g, ColorComponent b) {
        return make((a << 24) | (r << 16) | (g << 8) | (b << 0));
    }

    static constexpr Color transparent() {
        return make(0);
    }

    static constexpr Color black() {
        return makeARGB(0xFF, 0, 0, 0);
    }

    static constexpr Color white() {
        return makeARGB(0xFF, 0xFF, 0xFF, 0xFF);
    }

    static constexpr Color red() {
        return makeARGB(0xFF, 0xFF, 0, 0);
    }

    static constexpr Color green() {
        return makeARGB(0xFF, 0, 0xFF, 0);
    }

    static constexpr Color blue() {
        return makeARGB(0xFF, 0, 0, 0xFF);
    }

    constexpr bool operator==(Color other) const {
        return value == other.value;
    }

    constexpr bool operator!=(Color other) const {
        return value != other.value;
    }
};

std::ostream& operator<<(std::ostream& os, const Color& color);

} // namespace snap::drawing
