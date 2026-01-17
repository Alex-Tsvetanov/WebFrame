//
//  Animation.cpp
//  valdi-skia
//
//  Created by Simon Corsin on 7/3/20.
//

#include "snap_drawing/cpp/Animations/Animation.hpp"

namespace snap::drawing {

Animation::Animation(Duration duration, InterpolationFunction&& interpolationFunction, AnimationApplier&& applier)
    : _duration(duration),
      _remainingDuration(duration),
      _interpolationFunction(std::move(interpolationFunction)),
      _applier(std::move(applier)) {}

Animation::~Animation() = default;

bool Animation::run(Layer& layer, Duration delta) {
    if (!_started) {
        _applier(layer, 0.0);
        _started = true;
        return false;
    }

    _remainingDuration -= delta;

    if (_remainingDuration.seconds() <= 0) {
        _remainingDuration = Duration();
        _applier(layer, 1.0);

        return true;
    } else {
        auto ratio = _interpolationFunction(1.0 - (_remainingDuration.seconds() / _duration.seconds()));
        _applier(layer, ratio);

        return false;
    }
}

void Animation::cancel(Layer& layer) {
    finish(layer, false);
}

void Animation::complete(Layer& layer) {
    finish(layer, true);
}

void Animation::finish(Layer& layer, bool didComplete) {
    auto applier = std::move(_applier);
    _applier = nullptr;
    if (applier) {
        applier(layer, 1.0);
    }

    auto completions = std::move(_completions);
    for (const auto& completion : completions) {
        completion(didComplete);
    }
}

void Animation::addCompletion(AnimationCompletion&& completion) {
    _completions.emplace_back(std::move(completion));
}

} // namespace snap::drawing
