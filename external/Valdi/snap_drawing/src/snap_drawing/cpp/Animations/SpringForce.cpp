//
// Created by Carson Holgate on 8/4/22.
//

#include "SpringForce.hpp"
#include "valdi_core/cpp/Utils/ConsoleLogger.hpp"
#include <cstdlib>
#include <float.h>
#include <math.h>

// This was borrowed from Android's spring force implementation
// https://android.googlesource.com/platform/frameworks/support/+/300475fc42431cefb0d8f1bfc70de1a7a052a501/dynamic-animation/src/main/java/android/support/animation/SpringForce.java#77
static constexpr double kVelocityThresholdMultiplier = 1000.0 / 16.0;

namespace snap::drawing {
SpringForce::SpringForce(double damping, double stiffness, double valueThreshold)
    : _valueThreshold(abs(valueThreshold)) {
    // This is borrowed from the Android implementation
    // //valdi/src/java/com/snap/valdi/attributes/impl/animations/ValdiSpringAnimator.kt#L31
    _dampingRatio = damping / (2 * sqrt(stiffness));
    if (_dampingRatio < 0) {
        SC_ASSERT_FAIL("SpringForce damping ratio must be greater than 0");
    }
    _frequency = sqrt(stiffness);

    if (_dampingRatio > 1) {
        // Over damping
        _gammaPlus = -_dampingRatio * _frequency + _frequency * sqrt(_dampingRatio * _dampingRatio - 1);
        _gammaMinus = -_dampingRatio * _frequency - _frequency * sqrt(_dampingRatio * _dampingRatio - 1);
    } else if (_dampingRatio >= 0 && _dampingRatio < 1) {
        // Under damping
        _dampedFreq = _frequency * sqrt(1 - _dampingRatio * _dampingRatio);
    }

    _velocityThreshold = _valueThreshold * kVelocityThresholdMultiplier;
}

void SpringForce::setFinalPosition(double finalPosition) {
    _finalPosition = finalPosition;
}

double SpringForce::getFinalPosition() const {
    return _finalPosition;
}

MassState SpringForce::updateValues(double lastDisplacement, double lastVelocity, long timeElapsed) {
    double deltaT = timeElapsed / 1000.0;
    lastDisplacement -= _finalPosition;
    double displacement;
    double currentVelocity;
    double e = M_E;

    if (_dampingRatio > 1) {
        // Overdamped
        double coeffA = lastDisplacement - (_gammaMinus * lastDisplacement - lastVelocity) / (_gammaMinus - _gammaPlus);
        double coeffB = (_gammaMinus * lastDisplacement - lastVelocity) / (_gammaMinus - _gammaPlus);
        displacement = coeffA * pow(e, _gammaMinus * deltaT) + coeffB * pow(e, _gammaPlus * deltaT);
        currentVelocity =
            coeffA * _gammaMinus * pow(e, _gammaMinus * deltaT) + coeffB * _gammaPlus * pow(e, _gammaPlus * deltaT);
    } else if (_dampingRatio == 1) {
        // Critically damped
        double coeffA = lastDisplacement;
        double coeffB = lastVelocity + _frequency * lastDisplacement;
        displacement = (coeffA + coeffB * deltaT) * pow(e, -_frequency * deltaT);
        currentVelocity = (coeffA + coeffB * deltaT) * pow(e, -_frequency * deltaT) * (-_frequency) +
                          coeffB * pow(e, -_frequency * deltaT);
    } else {
        // Underdamped
        double cosCoeff = lastDisplacement;
        double sinCoeff = (1 / _dampedFreq) * (_dampingRatio * _frequency * lastDisplacement + lastVelocity);
        displacement = pow(e, -_dampingRatio * _frequency * deltaT) *
                       (cosCoeff * cos(_dampedFreq * deltaT) + sinCoeff * sin(_dampedFreq * deltaT));
        currentVelocity =
            displacement * (-_frequency) * _dampingRatio +
            pow(e, -_dampingRatio * _frequency * deltaT) * (-_dampedFreq * cosCoeff * sin(_dampedFreq * deltaT) +
                                                            _dampedFreq * sinCoeff * cos(_dampedFreq * deltaT));
    }
    _massState.value = displacement + _finalPosition;
    _massState.velocity = currentVelocity;
    return _massState;
}

double SpringForce::getAcceleration(double lastDisplacement, double lastVelocity) const {
    lastDisplacement -= getFinalPosition();
    double k = _frequency * _frequency;
    double c = 2 * _frequency * _dampingRatio;
    return -k * lastDisplacement - c * lastVelocity;
}

bool SpringForce::isAtEquilibrium(double value, double velocity) const {
    return abs(velocity) < _velocityThreshold && abs(value - _finalPosition) < _valueThreshold;
}

} // namespace snap::drawing