//
//  AndroidScroller.cpp
//  snap_drawing-macos
//
//  Created by Simon Corsin on 11/21/22.
//

#include "snap_drawing/cpp/Layers/Scroll/AndroidScroller.hpp"
#include "snap_drawing/cpp/Animations/InterpolationFunction.hpp"
#include "snap_drawing/cpp/Animations/ValueInterpolators.hpp"
#include "snap_drawing/cpp/Layers/Scroll/SplineScrollPhysics.hpp"
#include "snap_drawing/cpp/Layers/Scroll/SpringFlingScrollLayerAnimation.hpp"
#include "snap_drawing/cpp/Layers/ScrollLayer.hpp"
#include "snap_drawing/cpp/Resources.hpp"
#include <cmath>

namespace snap::drawing {

static constexpr Duration kSlowDuration = Duration(0.4);
static constexpr Duration kFastDuration = Duration(0.25);
static constexpr Scalar kFlingDecelerationRate = 0.998f;
static Scalar kFlingDecelerationCoefficient = 1000 * std::log(kFlingDecelerationRate);
static Scalar kFlingDecelerationCorrection = 1.0f / -kFlingDecelerationCoefficient;

constexpr Scalar kSpringMass = 0.35;
constexpr Scalar kSpringStiffness = 120;
constexpr Scalar kSpringDampingRatio = 0.95;

static const SpringScrollPhysicsConfiguration* getSpringScrollPhysicsConfiguration() {
    static SpringScrollPhysicsConfiguration kInstance =
        SpringScrollPhysicsConfiguration::make(kSpringMass, kSpringStiffness, kSpringDampingRatio);
    return &kInstance;
}

class AndroidContentOffsetAnimation : public BaseScrollLayerAnimation {
public:
    AndroidContentOffsetAnimation(const Point& sourceContentOffset, const Point& targetContentOffset, bool fast)
        : _sourceContentOffset(sourceContentOffset),
          _targetContentOffset(targetContentOffset),
          _duration(fast ? kFastDuration : kSlowDuration) {}

    ~AndroidContentOffsetAnimation() override = default;

    bool update(ScrollLayer& scrollLayer, Duration delta) override {
        _elapsed += delta;
        if (_elapsed >= _duration) {
            applyContentOffset(scrollLayer, _targetContentOffset);
            return true;
        }

        auto ratio = InterpolationFunctions::viscousFluid()(_elapsed.seconds() / _duration.seconds());
        auto currentContentOffset = interpolateValue(_sourceContentOffset, _targetContentOffset, ratio);

        auto adjustment = applyContentOffset(scrollLayer, currentContentOffset);
        _sourceContentOffset.x += adjustment.dx;
        _sourceContentOffset.y += adjustment.dy;

        return false;
    }

private:
    Point _sourceContentOffset;
    Point _targetContentOffset;
    Duration _duration;
    Duration _elapsed;
    Vector _contentOffsetAdjustment;
};

class AndroidFlingAnimation : public SpringFlingScrollLayerAnimation {
public:
    AndroidFlingAnimation(AndroidScroller* scroller,
                          Scalar scrollFriction,
                          const Point& sourceContentOffset,
                          const Vector& velocity,
                          bool /*fast*/)
        : _scroller(scroller), _sourceContentOffset(sourceContentOffset), _physics(scrollFriction, velocity) {}

    ~AndroidFlingAnimation() override = default;

protected:
    void updateCarriedVelocity(const Vector& velocity) final {
        _scroller->updateCarriedVelocity(velocity);
    }

    bool onDecelerate(ScrollLayer& scrollLayer, Duration elapsed) final {
        auto result = _physics.compute(elapsed);

        auto contentOffset = _sourceContentOffset.makeOffset(result.distance.dx, result.distance.dy);
        auto clampedContentOffset = clampContentOffset(scrollLayer, contentOffset);

        if (clampedContentOffset != contentOffset) {
            return startBouncing(scrollLayer,
                                 result.velocity,
                                 contentOffset,
                                 clampedContentOffset,
                                 Duration(),
                                 getSpringScrollPhysicsConfiguration());
        }

        auto adjustment = applyContentOffset(scrollLayer, contentOffset);
        _sourceContentOffset.x += adjustment.dx;
        _sourceContentOffset.y += adjustment.dy;

        updateCarriedVelocity(result.velocity);

        return result.finished;
    }

private:
    Ref<AndroidScroller> _scroller;
    Point _sourceContentOffset;
    SplineScrollPhysics _physics;
};

AndroidScroller::AndroidScroller(const Ref<Resources>& resources)
    : _scrollFriction(resources->getGesturesConfiguration().scrollFriction) {}

AndroidScroller::~AndroidScroller() = default;

void AndroidScroller::reset() {
    updateCarriedVelocity(Vector::makeEmpty());
}

void AndroidScroller::onDrag(GestureRecognizerState /*state*/, const DragEvent& event) {
    if (std::signbit(_carriedVelocity.dx) != std::signbit(event.velocity.dx)) {
        // Change direction, cancel all the carried velocity
        _carriedVelocity.dx = 0.0f;
    }

    if (std::signbit(_carriedVelocity.dy) != std::signbit(event.velocity.dy)) {
        // Change direction, cancel all the carried velocity
        _carriedVelocity.dy = 0.0f;
    }
}

Point AndroidScroller::computeDecelerationFinalOffset(const Point& contentOffset,
                                                      const Vector& velocity,
                                                      const Size& pageSize,
                                                      bool /*fast*/) {
    auto flingRangeX = pageSize.width / 2.0f;
    auto flingRangeY = pageSize.height / 2.0f;
    auto flingRawDistanceX = velocity.dx * kFlingDecelerationCorrection;
    auto flingRawDistanceY = velocity.dy * kFlingDecelerationCorrection;
    auto flingClampedDistanceX = std::min(std::max(flingRawDistanceX, -flingRangeX), +flingRangeX);
    auto flingClampedDistanceY = std::min(std::max(flingRawDistanceY, -flingRangeY), +flingRangeY);
    return Point(contentOffset.x + flingClampedDistanceX, contentOffset.y + flingClampedDistanceY);
}

void AndroidScroller::updateCarriedVelocity(Vector currentVelocity) {
    _carriedVelocity = currentVelocity;
}

Ref<BaseScrollLayerAnimation> AndroidScroller::fling(const Point& sourceContentOffset,
                                                     const Vector& velocity,
                                                     bool fast) {
    auto resolvedVelocity = Vector::make(velocity.dx + _carriedVelocity.dx, velocity.dy + _carriedVelocity.dy);

    return Valdi::makeShared<AndroidFlingAnimation>(this, _scrollFriction, sourceContentOffset, resolvedVelocity, fast);
}

Ref<BaseScrollLayerAnimation> AndroidScroller::animate(const Point& sourceContentOffset,
                                                       const Point& targetContentOffset,
                                                       bool fast) {
    return Valdi::makeShared<AndroidContentOffsetAnimation>(sourceContentOffset, targetContentOffset, fast);
}

} // namespace snap::drawing
