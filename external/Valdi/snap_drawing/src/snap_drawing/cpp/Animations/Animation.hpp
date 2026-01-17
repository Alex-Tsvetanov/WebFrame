//
//  Animation.hpp
//  valdi-skia
//
//  Created by Simon Corsin on 7/3/20.
//

#pragma once

#include "valdi_core/cpp/Utils/Function.hpp"
#include "valdi_core/cpp/Utils/Shared.hpp"

#include "snap_drawing/cpp/Animations/InterpolationFunction.hpp"
#include "snap_drawing/cpp/Utils/TimePoint.hpp"

#include <vector>

namespace snap::drawing {

class Layer;

using AnimationCompletion = Valdi::Function<void(bool)>;
using AnimationApplier = Valdi::Function<void(Layer&, double)>;

class IAnimation : public Valdi::SimpleRefCountable {
public:
    /**
     Run the animation on the given layer with the delta duration from the last run call.
     Returns whether the animation has completed.
     */
    virtual bool run(Layer& layer, Duration delta) = 0;

    /**
     Cancel the animation, which will clear the animation object and call the completion handler
     */
    virtual void cancel(Layer& layer) = 0;

    /**
     Complete the animation, which will clear the animation object and call the completion handler.
     This should be called after the run() function returns true.
     */
    virtual void complete(Layer& layer) = 0;

    /**
     A set of completion handlers to call when the animation completes or is cancelled
     */
    virtual void addCompletion(AnimationCompletion&& completion) = 0;
};

class Animation : public IAnimation {
public:
    Animation(Duration duration, InterpolationFunction&& interpolationFunction, AnimationApplier&& applier);
    ~Animation() override;

    bool run(Layer& layer, Duration delta) override;

    void cancel(Layer& layer) override;

    void complete(Layer& layer) override;

    void addCompletion(AnimationCompletion&& completion) override;

private:
    Duration _duration;
    Duration _remainingDuration;
    InterpolationFunction _interpolationFunction;
    AnimationApplier _applier;
    std::vector<snap::drawing::AnimationCompletion> _completions;
    bool _started = false;

    void finish(Layer& layer, bool didComplete);
};

} // namespace snap::drawing
