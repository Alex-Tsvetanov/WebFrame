//
//  ValdiAnimator.hpp
//  valdi-desktop-apple
//
//  Created by Simon Corsin on 7/3/20.
//

#pragma once

#include "snap_drawing/cpp/Animations/Animation.hpp"
#include "snap_drawing/cpp/Utils/Aliases.hpp"
#include "snap_drawing/cpp/Utils/TimePoint.hpp"
#include "valdi_core/Animator.hpp"
#include "valdi_core/cpp/Utils/Shared.hpp"

namespace snap::drawing {

// These are pulled from the ValdiValueAnimatorConfig
// ../../../java/com/snap/valdi/attributes/impl/animations/ValdiValueAnimatorConfig.kt#L13
static const double MIN_VISIBLE_CHANGE_COLOR = 0.00390;

static const double MIN_VISIBLE_CHANGE_ALPHA = 0.00390;

static const double MIN_VISIBLE_CHANGE_PIXEL = 0.00016;

static const double MIN_VISIBLE_CHANGE_SCALE_RATIO = 0.0004;

static const double MIN_VISIBLE_CHANGE_ROTATION_DEGREES_ANGLE = 0.00066;

struct PendingAnimation {
    Ref<Layer> view;
    Ref<IAnimation> animation;
    String key;
};

class ValdiAnimator : public snap::valdi_core::Animator, public Valdi::SharedPtrRefCountable {
public:
    ValdiAnimator(Duration duration,
                  InterpolationFunction&& interpolationFunction,
                  bool beginFromCurrentState,
                  double stiffness,
                  double damping);
    ~ValdiAnimator() override;

    InterpolationFunction getInterpolationFunction() const;
    Duration getDuration() const;
    double getStiffness() const;
    double getDamping() const;
    bool beginFromCurrentState() const;

    void createAndAppendAnimation(Layer& view, const String& key, double minVisibleChange, AnimationApplier&& applier);

    void appendAnimation(const Ref<Layer>& view, const Ref<IAnimation>& animation, const String& key);

    void flushAnimations(const Valdi::Value& completion) override;

    bool isSpringAnimation() const;

    void cancel() override;

private:
    Duration _duration;
    InterpolationFunction _interpolationFunction;
    bool _beginFromCurrentState;
    double _stiffness;
    double _damping;
    bool _isSpringAnimation;
    bool _wasCancelled = false;
    std::vector<PendingAnimation> _pendingAnimations;
    std::vector<PendingAnimation> _runningAnimations;
};

} // namespace snap::drawing
