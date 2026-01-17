//
//  SpringScrollPhysics.cpp
//  snap_drawing
//
//  Created by Simon Corsin on 11/23/22.
//

#include "snap_drawing/cpp/Layers/Scroll/SpringScrollPhysics.hpp"
#include <cmath>

namespace snap::drawing {

constexpr Scalar kThreshold = 0.5;
constexpr Duration kVelocityComputationDuration(0.01);
constexpr Scalar kMinOverScrollVelocity = kMinScrollVelocity;
constexpr Scalar kMaxOverscrollVelocity = 3500;

static Scalar computeClampedVelocity(Scalar velocity, Scalar displacement) {
    if (velocity == 0.0f && displacement == 0.0f) {
        return 0.0f;
    }

    auto sign = std::signbit(velocity) ? -1 : 1;
    return sign * std::clamp(std::abs(velocity), kMinOverScrollVelocity, kMaxOverscrollVelocity);
}

SpringScrollPhysicsConfiguration SpringScrollPhysicsConfiguration::make(Scalar springMass,
                                                                        Scalar springStiffness,
                                                                        Scalar springDampingRatio) {
    auto naturalFrequency = std::sqrt(springStiffness / springMass) * std::sqrt(1.0f - std::pow(springDampingRatio, 2));
    auto damping = 2.0f * springDampingRatio * std::sqrt(springMass * springStiffness);
    auto beta = damping / (2.0f * springMass);

    return SpringScrollPhysicsConfiguration(naturalFrequency, damping, beta);
}

SpringScrollPhysics::SpringScrollPhysics(const SpringScrollPhysicsConfiguration* configuration,
                                         const Vector& velocity,
                                         const Vector& displacement)
    : _configuration(configuration),
      _velocity(computeClampedVelocity(velocity.dx, displacement.dx),
                computeClampedVelocity(velocity.dy, displacement.dy)),
      _displacement(displacement) {
    auto springReferenceX =
        (_velocity.dx + configuration->getBeta() * displacement.dx) / configuration->getNaturalFrequency();
    auto springReferenceY =
        (_velocity.dy + configuration->getBeta() * displacement.dy) / configuration->getNaturalFrequency();
    _springReference = Vector::make(springReferenceX, springReferenceY);

    auto displacementLength = Point::length(displacement.dx, displacement.dy);
    auto velocityLength = Point::length(_velocity.dx, _velocity.dy);

    if (displacementLength != 0 || velocityLength != 0) {
        auto referenceLength = Point::length(_springReference.dx, _springReference.dy);

        _duration = Duration::fromSeconds(std::log((displacementLength + referenceLength) / kThreshold) /
                                          configuration->getBeta());
    }
}

SpringScrollPhysicsResult SpringScrollPhysics::compute(Duration elapsed) const {
    SpringScrollPhysicsResult result;

    if (elapsed < _duration) {
        result.finished = false;
        result.distance = computeDistance(elapsed);

        if (elapsed > kVelocityComputationDuration) {
            auto lastDistance = computeDistance(elapsed - kVelocityComputationDuration);
            result.velocity.dx = (result.distance.dx - lastDistance.dx) / kVelocityComputationDuration.seconds();
            result.velocity.dy = (result.distance.dy - lastDistance.dy) / kVelocityComputationDuration.seconds();
        } else {
            result.velocity = _velocity;
        }
    } else {
        result.finished = true;
    }

    return result;
}

Vector SpringScrollPhysics::computeDistance(Duration elapsed) const {
    auto time = elapsed.seconds();
    auto wd = _configuration->getNaturalFrequency() * time;
    auto factor1 = std::exp(-_configuration->getBeta() * time);
    auto factor2 = std::cos(wd);
    auto factor3 = std::sin(wd);
    auto bounceOffsetX = factor1 * (_displacement.dx * factor2 + _springReference.dx * factor3);
    auto bounceOffsetY = factor1 * (_displacement.dy * factor2 + _springReference.dy * factor3);
    return Vector::make(bounceOffsetX, bounceOffsetY);
}

} // namespace snap::drawing
