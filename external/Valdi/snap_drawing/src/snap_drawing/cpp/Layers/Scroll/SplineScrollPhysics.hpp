//
//  SplineScrollPhysics.hpp
//  snap_drawing-macos
//
//  Created by Simon Corsin on 11/22/22.
//

#pragma once

#include "snap_drawing/cpp/Utils/Duration.hpp"
#include "snap_drawing/cpp/Utils/Geometry.hpp"

namespace snap::drawing {

struct SplineScrollPhysicsResult {
    Vector distance;
    Vector velocity;
    bool finished = false;
};

class SplineScrollPhysics {
public:
    SplineScrollPhysics(Scalar scrollFriction, const Vector& velocity);

    SplineScrollPhysicsResult compute(Duration elapsed) const;

    static void initialize(Scalar gravity,
                           Scalar inflexion,
                           Scalar startTension,
                           Scalar endTension,
                           Scalar physicalCoeff,
                           Scalar decelerationRate);

private:
    Scalar _scrollFriction;
    Vector _distance;
    Duration _durationX;
    Duration _durationY;

    Scalar computeDeceleration(Scalar velocity) const;
    Scalar computeDistance(Scalar velocity, Scalar deceleration) const;
    static Duration computeDuration(Scalar deceleration);

    static bool doCompute(
        Duration totalDuration, Scalar totalDistance, Duration elapsed, Scalar* outDistance, Scalar* outVelocity);
};

} // namespace snap::drawing
