//
// Created by Carson Holgate on 8/4/22.
//

#include "SpringAnimation.hpp"

#include "snap_drawing/cpp/Animations/Animation.hpp"
#include "snap_drawing/cpp/Layers/Layer.hpp"
#include "valdi_core/cpp/Utils/ConsoleLogger.hpp"

#include <float.h>

static constexpr double kMax = std::numeric_limits<double>::max();
static constexpr double kMin = std::numeric_limits<double>::min();

namespace snap::drawing {

SpringAnimation::SpringAnimation(double stiffness, double damping, double minVisibleChange, AnimationApplier&& applier)
    : _applier(applier), _spring(damping, stiffness, minVisibleChange) {}

SpringAnimation::~SpringAnimation() = default;

bool SpringAnimation::run(Layer& layer, Duration delta) {
    if (!_started) {
        _applier(layer, 0.0);
        _started = true;
        return false;
    }

    if (_endRequested) {
        if (_pendingPosition != kMax) {
            _spring.setFinalPosition(_pendingPosition);
            _pendingPosition = kMax;
        }
        _value = _spring.getFinalPosition();
        _velocity = 0;
        _endRequested = false;
        return true;
    }
    if (_pendingPosition != kMax) {
        // Approximate by considering half of the time spring position stayed at the old
        // position, half of the time it's at the new position.
        MassState massState = _spring.updateValues(_value, _velocity, delta.milliseconds() / 2);
        _spring.setFinalPosition(_pendingPosition);
        _pendingPosition = kMax;
        massState = _spring.updateValues(massState.value, massState.velocity, delta.milliseconds() / 2);
        _value = massState.value;
        _velocity = massState.velocity;
    } else {
        MassState massState = _spring.updateValues(_value, _velocity, delta.milliseconds());
        _value = massState.value;
        _velocity = massState.velocity;
    }
    _value = fmax(_value, kMin);
    _value = fmin(_value, kMax);
    if (_spring.isAtEquilibrium(_value, _velocity)) {
        _value = _spring.getFinalPosition();
        _velocity = 0;
        _applier(layer, _value);
        return true;
    }

    _applier(layer, _value);
    return false;
}

void SpringAnimation::cancel(Layer& layer) {
    finish(layer, false);
}

void SpringAnimation::complete(Layer& layer) {
    finish(layer, true);
}

void SpringAnimation::finish(Layer& layer, bool didComplete) {
    auto applier = std::move(_applier);
    _applier = nullptr;
    if (applier) {
        applier(layer, 1.0);
    }

    for (const auto& completion : _completions) {
        completion(didComplete);
    }
    _completions.clear();
}

void SpringAnimation::addCompletion(AnimationCompletion&& completion) {
    _completions.emplace_back(std::move(completion));
}

} // namespace snap::drawing
