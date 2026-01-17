//
//  ValdiAnimator.cpp
//  valdi-desktop-apple
//
//  Created by Simon Corsin on 7/3/20.
//

#include "valdi/snap_drawing/Animations/ValdiAnimator.hpp"

#include "snap_drawing/cpp/Animations/Animation.hpp"
#include "snap_drawing/cpp/Animations/SpringAnimation.hpp"
#include "snap_drawing/cpp/Layers/Layer.hpp"

#include "valdi/runtime/Utils/AsyncGroup.hpp"
#include "valdi_core/cpp/Utils/ValueFunction.hpp"

namespace snap::drawing {

ValdiAnimator::ValdiAnimator(Duration duration,
                             InterpolationFunction&& interpolationFunction,
                             bool beginFromCurrentState,
                             double stiffness,
                             double damping)
    : _duration(duration),
      _interpolationFunction(std::move(interpolationFunction)),
      _beginFromCurrentState(beginFromCurrentState),
      _stiffness(stiffness),
      _damping(damping) {
    _isSpringAnimation = _stiffness != 0 && _damping != 0;
}

ValdiAnimator::~ValdiAnimator() = default;

void ValdiAnimator::appendAnimation(const Ref<Layer>& view, const Ref<IAnimation>& animation, const String& key) {
    PendingAnimation pendingAnimation;
    pendingAnimation.view = view;
    pendingAnimation.animation = animation;
    pendingAnimation.key = key;

    _pendingAnimations.emplace_back(std::move(pendingAnimation));
}

void ValdiAnimator::flushAnimations(const Valdi::Value& completion) {
    auto group = Valdi::makeShared<Valdi::AsyncGroup>();

    auto pendingAnimations = std::move(_pendingAnimations);
    for (const auto& pendingAnimation : pendingAnimations) {
        group->enter();
        pendingAnimation.animation->addCompletion([=](auto /*completed*/) { group->leave(); });

        pendingAnimation.view->addAnimation(pendingAnimation.key, pendingAnimation.animation);
        _runningAnimations.emplace_back(pendingAnimation);
    }

    group->notify([strongSelf = Valdi::strongSmallRef(this), completion]() {
        if (completion.isFunction()) {
            (*completion.getFunction())({Valdi::Value(strongSelf->_wasCancelled)});
        }
        strongSelf->_runningAnimations.clear();
    });
}

void ValdiAnimator::cancel() {
    if (_wasCancelled) {
        return;
    }

    _wasCancelled = true;

    auto runningAnimations = std::move(_runningAnimations);
    for (const auto& runningAnimation : runningAnimations) {
        if (runningAnimation.view.get() != nullptr && runningAnimation.animation != nullptr) {
            runningAnimation.animation->cancel(*runningAnimation.view.get());
            runningAnimation.view->removeAnimation(runningAnimation.key);
        }
    }
}

void ValdiAnimator::createAndAppendAnimation(Layer& view,
                                             const String& key,
                                             double minVisibleChange,
                                             AnimationApplier&& applier) {
    auto ref = Valdi::strongSmallRef(&view);
    Ref<IAnimation> animation;
    if (_isSpringAnimation) {
        animation = Valdi::makeShared<SpringAnimation>(_stiffness, _damping, minVisibleChange, std::move(applier));
    } else {
        animation = Valdi::makeShared<Animation>(_duration, getInterpolationFunction(), std::move(applier));
    }
    appendAnimation(ref, animation, key);
}

Duration ValdiAnimator::getDuration() const {
    return _duration;
}

double ValdiAnimator::getStiffness() const {
    return _stiffness;
}

double ValdiAnimator::getDamping() const {
    return _damping;
}

InterpolationFunction ValdiAnimator::getInterpolationFunction() const {
    return _interpolationFunction;
}

bool ValdiAnimator::beginFromCurrentState() const {
    return _beginFromCurrentState;
}

bool ValdiAnimator::isSpringAnimation() const {
    return _isSpringAnimation;
}

} // namespace snap::drawing
