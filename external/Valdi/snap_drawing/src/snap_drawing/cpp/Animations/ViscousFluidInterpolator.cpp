//
//  ViscousFluidInterpolator.cpp
//  snap_drawing-macos
//
//  Created by Simon Corsin on 11/21/22.
//

#include "snap_drawing/cpp/Animations/ViscousFluidInterpolator.hpp"

namespace snap::drawing {

ViscousFluidInterpolator::ViscousFluidInterpolator() {
    // must be set to 1.0 (used in viscousFluid())
    _normalize = 1.0f / viscousFluid(1.0f);
    // account for very small floating-point error
    _offset = 1.0f - _normalize * viscousFluid(1.0f);
}

double ViscousFluidInterpolator::viscousFluid(double x) const {
    x *= _scale;
    if (x < 1.0) {
        x -= (1.0 - std::exp(-x));
    } else {
        double start = 0.36787944117; // 1/e == exp(-1)
        x = 1.0 - std::exp(1.0 - x);
        x = start + x * (1.0 - start);
    }
    return x;
}

double ViscousFluidInterpolator::interpolate(double i) const {
    auto interpolated = _normalize * viscousFluid(i);
    if (interpolated > 0) {
        return interpolated + _offset;
    }
    return interpolated;
}

} // namespace snap::drawing
