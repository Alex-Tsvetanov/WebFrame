//
// Created by Carson Holgate on 8/4/22.
//

#pragma once

#include "snap_drawing/cpp/Animations/Animation.hpp"
#include "snap_drawing/cpp/Animations/SpringForce.hpp"

namespace snap::drawing {

class SpringAnimation : public IAnimation {
public:
    SpringAnimation(double stiffness, double damping, double minVisibleChange, AnimationApplier&& applier);
    ~SpringAnimation() override;

    bool run(Layer& layer, Duration delta) override;

    void cancel(Layer& layer) override;

    void complete(Layer& layer) override;

    void addCompletion(AnimationCompletion&& completion) override;

private:
    AnimationApplier _applier;
    std::vector<snap::drawing::AnimationCompletion> _completions;
    bool _started = false;
    SpringForce _spring;
    double _value = 0;
    double _velocity = 0;
    double _pendingPosition = 1;
    bool _endRequested = false;

    void finish(Layer& layer, bool didComplete);
};

} // namespace snap::drawing
