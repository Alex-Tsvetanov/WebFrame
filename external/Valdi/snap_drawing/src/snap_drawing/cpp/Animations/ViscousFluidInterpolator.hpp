//
//  ViscousFluidInterpolator.hpp
//  snap_drawing-macos
//
//  Created by Simon Corsin on 11/21/22.
//

#pragma once

#include "snap_drawing/cpp/Utils/Scalar.hpp"

namespace snap::drawing {

/**
 An interpolator implementing the "viscous fluid" effect
 from the Android's scroll animation implementation.
 */
class ViscousFluidInterpolator {
public:
    ViscousFluidInterpolator();

    double interpolate(double i) const;

private:
    double _scale = 8.0f;
    double _normalize;
    double _offset;

    double viscousFluid(double x) const;
};

} // namespace snap::drawing
