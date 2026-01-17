//
//  SpringScrollPhysics.hpp
//  snap_drawing
//
//  Created by Simon Corsin on 11/23/22.
//

#pragma once

#include "snap_drawing/cpp/Utils/Duration.hpp"
#include "snap_drawing/cpp/Utils/Geometry.hpp"

namespace snap::drawing {

class SpringScrollPhysicsConfiguration {
public:
    constexpr SpringScrollPhysicsConfiguration(Scalar naturalFrequency, Scalar damping, Scalar beta)
        : _naturalFrequency(naturalFrequency), _damping(damping), _beta(beta) {}

    constexpr Scalar getNaturalFrequency() const {
        return _naturalFrequency;
    }

    constexpr Scalar getDamping() const {
        return _damping;
    }

    constexpr Scalar getBeta() const {
        return _beta;
    }

    static SpringScrollPhysicsConfiguration make(Scalar springMass, Scalar springStiffness, Scalar springDampingRatio);

private:
    Scalar _naturalFrequency = 0.0f;
    Scalar _damping = 0.0f;
    Scalar _beta = 0.0f;
};

struct SpringScrollPhysicsResult {
    Vector distance;
    Vector velocity;
    bool finished = false;
};

constexpr Scalar kMinScrollVelocity = 15;

class SpringScrollPhysics {
public:
    SpringScrollPhysics(const SpringScrollPhysicsConfiguration* configuration,
                        const Vector& velocity,
                        const Vector& displacement);

    SpringScrollPhysicsResult compute(Duration elapsed) const;

    const Duration& getDuration() const {
        return _duration;
    }

private:
    const SpringScrollPhysicsConfiguration* _configuration;
    Vector _velocity;
    Vector _displacement;
    Vector _springReference;
    Duration _duration;

    Vector computeDistance(Duration elapsed) const;
};

} // namespace snap::drawing
