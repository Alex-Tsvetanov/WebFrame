//
//  SplineScrollPhysics.cpp
//  snap_drawing-macos
//
//  Created by Simon Corsin on 11/22/22.
//

#include "snap_drawing/cpp/Layers/Scroll/SplineScrollPhysics.hpp"
#include <array>
#include <cmath>

namespace snap::drawing {

/**
  These constants were extracted from Android's OverScroller.java
*/
static constexpr Scalar kDefaultGravity = 2000.0f;
static constexpr Scalar kDefaultInflexion = 0.35f;
static constexpr Scalar kDefaultStartTension = 0.5f;
static constexpr Scalar kDefaultEndTension = 1.0f;
static constexpr Scalar kDefaultPhysicalCoeff = 9.80665f // g (m/s^2)
                                                * 39.37f // inch/meter
                                                * 160.0f // pixels per inch
                                                * 0.84f; // look and feel tuning
static constexpr Scalar kDefaultDecelerationRate = 2.3582017f;

static Scalar getOrDefault(Scalar value, Scalar defaultValue) {
    return value != 0.0f ? value : defaultValue;
}

/**
 A large part of this file was ported from Android's OverScroller.java
 file.
 */
struct SplineOverScrollerConstants {
    // Constant gravity value, used in the deceleration phase.
    Scalar gravity;
    Scalar inflexion; // Tension lines cross at (INFLEXION, 1)
    Scalar startTension;
    Scalar endTension;
    Scalar physicalCoeff;
    Scalar decelerationRate;

    static constexpr size_t kNBSamples = 100;

    std::array<Scalar, SplineOverScrollerConstants::kNBSamples + 1> splinePosition;
    std::array<Scalar, SplineOverScrollerConstants::kNBSamples + 1> splineTime;

    static void initialize(Scalar gravity,
                           Scalar inflexion,
                           Scalar startTension,
                           Scalar endTension,
                           Scalar physicalCoeff,
                           Scalar decelerationRate) {
        kInstance.doInitialize(getOrDefault(gravity, kDefaultGravity),
                               getOrDefault(inflexion, kDefaultInflexion),
                               getOrDefault(startTension, kDefaultStartTension),
                               getOrDefault(endTension, kDefaultEndTension),
                               getOrDefault(physicalCoeff, kDefaultPhysicalCoeff),
                               getOrDefault(decelerationRate, kDefaultDecelerationRate));
    }

    static const SplineOverScrollerConstants& get() {
        if (!kInstance._initialized) {
            initialize(0, 0, 0, 0, 0, 0);
        }

        return kInstance;
    }

private:
    bool _initialized = false;
    static SplineOverScrollerConstants kInstance;

    void doInitialize(Scalar gravity,
                      Scalar inflexion,
                      Scalar startTension,
                      Scalar endTension,
                      Scalar physicalCoeff,
                      Scalar decelerationRate) {
        this->gravity = gravity;
        this->inflexion = inflexion;
        this->startTension = startTension;
        this->endTension = endTension;
        this->physicalCoeff = physicalCoeff;
        this->decelerationRate = decelerationRate;
        _initialized = true;

        auto p1 = startTension * inflexion;
        auto p2 = 1.0f - endTension * (1.0f - inflexion);
        Scalar xMin = 0.0f;
        Scalar yMin = 0.0f;
        for (size_t i = 0; i < kNBSamples; i++) {
            Scalar alpha = static_cast<Scalar>(i) / static_cast<Scalar>(kNBSamples);

            Scalar xMax = 1.0f;
            Scalar x = 0.0f;
            Scalar tx = 0.0f;
            Scalar coef = 0.0f;
            for (;;) {
                x = xMin + (xMax - xMin) / 2.0f;
                coef = 3.0f * x * (1.0f - x);
                tx = coef * ((1.0f - x) * p1 + x * p2) + x * x * x;
                if (std::abs(tx - alpha) < 1E-5) {
                    break;
                }
                if (tx > alpha) {
                    xMax = x;
                } else {
                    xMin = x;
                }
            }
            splinePosition[i] = coef * ((1.0f - x) * startTension + x) + x * x * x;

            Scalar yMax = 1.0f;
            Scalar y = 0.0f;
            Scalar dy = 0.0f;
            for (;;) {
                y = yMin + (yMax - yMin) / 2.0f;
                coef = 3.0f * y * (1.0f - y);
                dy = coef * ((1.0f - y) * startTension + y) + y * y * y;
                if (std::abs(dy - alpha) < 1E-5) {
                    break;
                }
                if (dy > alpha) {
                    yMax = y;
                } else {
                    yMin = y;
                }
            }
            splineTime[i] = coef * ((1.0f - y) * p1 + y * p2) + y * y * y;
        }
        splinePosition[kNBSamples] = splineTime[kNBSamples] = 1.0f;
    }
};

// NOLINTNEXTLINE(fuchsia-statically-constructed-objects)
SplineOverScrollerConstants SplineOverScrollerConstants::kInstance;

SplineScrollPhysics::SplineScrollPhysics(Scalar scrollFriction, const Vector& velocity)
    : _scrollFriction(scrollFriction) {
    if (velocity.dx != 0.0f) {
        auto decelerationX = computeDeceleration(velocity.dx);
        _distance.dx = computeDistance(velocity.dx, decelerationX);
        _durationX = computeDuration(decelerationX);
    }

    if (velocity.dy != 0.0f) {
        auto decelerationY = computeDeceleration(velocity.dy);
        _distance.dy = computeDistance(velocity.dy, decelerationY);
        _durationY = computeDuration(decelerationY);
    }
}

SplineScrollPhysicsResult SplineScrollPhysics::compute(Duration elapsed) const {
    SplineScrollPhysicsResult result;

    auto xFinished = doCompute(_durationX, _distance.dx, elapsed, &result.distance.dx, &result.velocity.dx);
    auto yFinished = doCompute(_durationY, _distance.dy, elapsed, &result.distance.dy, &result.velocity.dy);

    result.finished = xFinished && yFinished;

    return result;
}

bool SplineScrollPhysics::doCompute(
    Duration totalDuration, Scalar totalDistance, Duration elapsed, Scalar* outDistance, Scalar* outVelocity) {
    if (elapsed >= totalDuration) {
        *outDistance = totalDistance;
        *outVelocity = 0.0f;
        return true;
    }

    const auto& constants = SplineOverScrollerConstants::get();

    auto ratio = static_cast<Scalar>(elapsed.seconds() / totalDuration.seconds());
    auto index = static_cast<size_t>(SplineOverScrollerConstants::kNBSamples * ratio);
    Scalar distanceCoef = 1.f;
    Scalar velocityCoef = 0.f;
    if (index < SplineOverScrollerConstants::kNBSamples) {
        auto tInf = static_cast<Scalar>(index) / static_cast<Scalar>(SplineOverScrollerConstants::kNBSamples);
        auto tSup = static_cast<Scalar>(index + 1) / static_cast<Scalar>(SplineOverScrollerConstants::kNBSamples);
        auto dInf = constants.splinePosition[index];
        auto dSup = constants.splinePosition[index + 1];
        velocityCoef = (dSup - dInf) / (tSup - tInf);
        distanceCoef = dInf + (ratio - tInf) * velocityCoef;
    }

    *outDistance = distanceCoef * totalDistance;
    *outVelocity = velocityCoef * totalDistance / static_cast<Scalar>(totalDuration.seconds());

    return false;
}

Scalar SplineScrollPhysics::computeDeceleration(Scalar velocity) const {
    const auto& constants = SplineOverScrollerConstants::get();
    return std::log(constants.inflexion * std::abs(velocity) / (_scrollFriction * constants.physicalCoeff));
}

Scalar SplineScrollPhysics::computeDistance(Scalar velocity, Scalar deceleration) const {
    const auto& constants = SplineOverScrollerConstants::get();
    auto decelMinusOne = constants.decelerationRate - 1.0;
    auto absDistance =
        _scrollFriction * constants.physicalCoeff * std::exp(constants.decelerationRate / decelMinusOne * deceleration);

    return std::signbit(velocity) ? -absDistance : absDistance;
}

Duration SplineScrollPhysics::computeDuration(Scalar deceleration) {
    auto decelMinusOne = SplineOverScrollerConstants::get().decelerationRate - 1.0;
    return Duration::fromSeconds(std::exp(deceleration / decelMinusOne));
}

void SplineScrollPhysics::initialize(Scalar gravity,
                                     Scalar inflexion,
                                     Scalar startTension,
                                     Scalar endTension,
                                     Scalar physicalCoeff,
                                     Scalar decelerationRate) {
    SplineOverScrollerConstants::initialize(
        gravity, inflexion, startTension, endTension, physicalCoeff, decelerationRate);
}

} // namespace snap::drawing
