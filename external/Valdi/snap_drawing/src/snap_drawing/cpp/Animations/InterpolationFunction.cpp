//
//  InterpolationFunction.cpp
//  valdi-skia
//
//  Created by Simon Corsin on 7/3/20.
//

#include "snap_drawing/cpp/Animations/InterpolationFunction.hpp"
#include "snap_drawing/cpp/Animations/ViscousFluidInterpolator.hpp"
#include "snap_drawing/cpp/Animations/bezier.hpp"
#include <cmath>
#include <cstdio>

namespace snap::drawing {

static Bezier::Bezier<3> makeTimingCurve(float p1x, float p1y, float p2x, float p2y) {
    Bezier::Bezier<3> curve({{0.0, 0.0}, {p1x, p1y}, {p2x, p2y}, {1.0, 1.0}});
    return curve;
}

static InterpolationFunction makeInterpolationFunction(const Bezier::Bezier<3>& curve) {
    return [curve](double t) -> double {
        double mappedX = curve.valueAt(static_cast<float>(t), 0);
        double mappedY = curve.valueAt(static_cast<float>(mappedX), 1);
        return mappedY;
    };
}

// matches https://developer.apple.com/documentation/quartzcore/camediatimingfunctionname/1521854-default
Bezier::Bezier<3> defaultBezier = makeTimingCurve(0.25, 0.1, 0.25, 1.0);
// matches https://developer.apple.com/documentation/quartzcore/camediatimingfunctionname/1522173-easeineaseout
Bezier::Bezier<3> easeInOutBezier = makeTimingCurve(0.42, 0.0, 0.58, 1.0);
// matches https://developer.apple.com/documentation/quartzcore/camediatimingfunctionname/1521971-easein
Bezier::Bezier<3> easeInBezier = makeTimingCurve(0.42, 0.0, 1.0, 1.0);
// matches https://developer.apple.com/documentation/quartzcore/camediatimingfunctionname/1522178-easeout
Bezier::Bezier<3> easeOutBezier = makeTimingCurve(0.0, 0.0, 0.58, 1.0);

Bezier::Bezier<3> strongEaseOutBezier = makeTimingCurve(0.9, 0.90, 0.95, 1.0);

static const ViscousFluidInterpolator& getViscousFluidInterpolator() {
    static ViscousFluidInterpolator kInstance;
    return kInstance;
}

InterpolationFunction InterpolationFunctions::linear() {
    return [](double ratio) -> double { return ratio; };
}

InterpolationFunction InterpolationFunctions::systemDefault() {
    return makeInterpolationFunction(defaultBezier);
}

InterpolationFunction InterpolationFunctions::easeInOut() {
    return makeInterpolationFunction(easeInOutBezier);
}

InterpolationFunction InterpolationFunctions::easeIn() {
    return makeInterpolationFunction(easeInBezier);
}

InterpolationFunction InterpolationFunctions::easeOut() {
    return makeInterpolationFunction(easeOutBezier);
}

InterpolationFunction InterpolationFunctions::strongEaseOut() {
    return makeInterpolationFunction(strongEaseOutBezier);
}

InterpolationFunction InterpolationFunctions::viscousFluid() {
    return [](double ratio) -> double { return getViscousFluidInterpolator().interpolate(ratio); };
}

} // namespace snap::drawing
