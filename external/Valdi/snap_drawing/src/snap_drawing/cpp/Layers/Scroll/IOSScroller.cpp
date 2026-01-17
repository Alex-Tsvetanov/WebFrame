//
//  IOSScroller.cpp
//  snap_drawing-ios
//
//  Created by Simon Corsin on 11/18/22.
//

#include "snap_drawing/cpp/Layers/Scroll/IOSScroller.hpp"
#include "snap_drawing/cpp/Layers/Scroll/SpringFlingScrollLayerAnimation.hpp"
#include "snap_drawing/cpp/Layers/Scroll/SpringScrollPhysics.hpp"
#include "snap_drawing/cpp/Layers/ScrollLayer.hpp"

#include <cmath>

namespace snap::drawing {

constexpr Scalar kVelocityThreshold = 0.2;
constexpr float kTimeThresholdInMilliseconds = 250;

constexpr Scalar kThreshold = 0.5;

constexpr Scalar kDecelerationNormalRate = 0.998;
static Scalar const kDecelerationNormalCoef = 1000 * log(kDecelerationNormalRate);

constexpr Scalar kDecelerationFastRate = 0.99;
static Scalar const kDecelerationFastCoef = 1000 * log(kDecelerationFastRate);

constexpr Scalar kSpringMass = 0.5;
constexpr Scalar kSpringStiffness = 95;
constexpr Scalar kSpringDampingRatio = 0.95;

static const SpringScrollPhysicsConfiguration* getSpringScrollPhysicsConfiguration() {
    static SpringScrollPhysicsConfiguration kInstance =
        SpringScrollPhysicsConfiguration::make(kSpringMass, kSpringStiffness, kSpringDampingRatio);
    return &kInstance;
}

static Scalar carriedVelocityComputation(Scalar velocity) {
    static constexpr Scalar kThreshold = 80000.0f;
    Scalar sign = std::signbit(velocity) ? -1 : 1;
    Scalar scaledMomentum = 0.000816f * std::pow(std::abs(velocity), 1.967f);

    return sign * std::min(scaledMomentum, kThreshold);
}

class IOSContentOffsetAnimation : public BaseScrollLayerAnimation {
public:
    IOSContentOffsetAnimation(const Point& sourceContentOffset, const Point& targetContentOffset, bool fast)
        : _contentOffset(sourceContentOffset), _targetContentOffset(targetContentOffset), _fast(fast) {
        _velocity = IOSScroller::computeDecelerationInitialVelocity(sourceContentOffset, targetContentOffset, fast);
        _duration = IOSScroller::computeDecelerationDuration(_velocity, false);
    }

    ~IOSContentOffsetAnimation() override = default;

    bool update(ScrollLayer& scrollLayer, Duration delta) override {
        _elapsed += delta;

        if (_elapsed > _duration) {
            applyContentOffset(scrollLayer, _targetContentOffset);
            return true;
        }

        auto contentOffset = IOSScroller::computeDecelerationOffset(_contentOffset, _velocity, _elapsed, _fast);

        auto adjustment = applyContentOffset(scrollLayer, contentOffset);

        _contentOffset.x += adjustment.dx;
        _contentOffset.y += adjustment.dy;

        return false;
    }

private:
    Duration _elapsed;
    Duration _duration;
    Point _contentOffset;
    Point _targetContentOffset;
    Vector _velocity;
    bool _fast;
};

class IOSFlingAnimation : public SpringFlingScrollLayerAnimation {
public:
    IOSFlingAnimation(IOSScroller* scroller, const Point& sourceContentOffset, const Vector& velocity, bool fast)
        : _scroller(Valdi::strongSmallRef(scroller)), _offset(sourceContentOffset), _velocity(velocity), _fast(fast) {}

    ~IOSFlingAnimation() override = default;

    void updateCarriedVelocity(const Vector& velocity) final {
        _scroller->updateCarriedVelocity(velocity);
    }

    bool onOverscroll(ScrollLayer& scrollLayer,
                      Duration elapsed,
                      const Vector& deceleratedVelocity,
                      const Point& contentOffset,
                      const Point& clampedContentOffset) {
        Duration startTime;
        Vector resolvedVelocity = deceleratedVelocity;
        Point resolvedSourceContentOffset = contentOffset;

        auto duration =
            IOSScroller::computeDecelerationTimeAtTargetOffset(_offset, _velocity, clampedContentOffset, _fast);

        if (duration) {
            startTime = elapsed - duration.value();
            resolvedVelocity = computeDecelerationVelocity(duration.value());
            resolvedSourceContentOffset = clampedContentOffset;
        }

        return startBouncing(scrollLayer,
                             resolvedVelocity,
                             resolvedSourceContentOffset,
                             clampedContentOffset,
                             startTime,
                             getSpringScrollPhysicsConfiguration());
    }

    bool onDecelerate(ScrollLayer& scrollLayer, Duration elapsed) final {
        auto deceleratedOffset = computeDecelerationOffset(elapsed);
        auto deceleratedVelocity = computeDecelerationVelocity(elapsed);

        auto clampedOffset = clampContentOffset(scrollLayer, deceleratedOffset);

        const bool overScrolled = clampedOffset.x != deceleratedOffset.x || clampedOffset.y != deceleratedOffset.y;

        if (overScrolled) {
            return onOverscroll(scrollLayer, elapsed, deceleratedVelocity, deceleratedOffset, clampedOffset);
        }

        auto lowVelocity = Point::length(deceleratedVelocity.dx, deceleratedVelocity.dy) < kMinScrollVelocity;
        if (lowVelocity) {
            _scroller->reset();
            return true;
        }

        updateCarriedVelocity(deceleratedVelocity);

        auto adjustment = applyContentOffset(scrollLayer, deceleratedOffset);
        _offset.x += adjustment.dx;
        _offset.y += adjustment.dy;

        return false;
    }

private:
    Ref<IOSScroller> _scroller;
    Point _offset;
    Vector _velocity;
    bool _fast;

    Point computeDecelerationOffset(Duration elapsedTime) const {
        return IOSScroller::computeDecelerationOffset(_offset, _velocity, elapsedTime, _fast);
    }

    Vector computeDecelerationVelocity(Duration elapsed) const {
        auto rate = IOSScroller::getDecelerationRate(_fast);
        auto ratio = std::pow(rate, elapsed.milliseconds());
        auto deceleratedVelocityX = _velocity.dx * ratio;
        auto deceleratedVelocityY = _velocity.dy * ratio;
        return Vector::make(deceleratedVelocityX, deceleratedVelocityY);
    }
};

IOSScroller::IOSScroller() : _dragStartTime(0) {}
IOSScroller::~IOSScroller() = default;

Point IOSScroller::computeDecelerationFinalOffset(const Point& contentOffset,
                                                  const Vector& velocity,
                                                  const Size& /*pageSize*/,
                                                  bool fast) {
    auto coef = IOSScroller::getDecelerationCoef(fast);
    auto finalOffsetX = contentOffset.x + velocity.dx / -coef;
    auto finalOffsetY = contentOffset.y + velocity.dy / -coef;
    return Point::make(finalOffsetX, finalOffsetY);
}

Ref<BaseScrollLayerAnimation> IOSScroller::animate(const Point& sourceContentOffset,
                                                   const Point& targetContentOffset,
                                                   bool fast) {
    reset();

    return Valdi::makeShared<IOSContentOffsetAnimation>(sourceContentOffset, targetContentOffset, fast);
}

Ref<BaseScrollLayerAnimation> IOSScroller::fling(const Point& sourceContentOffset, const Vector& velocity, bool fast) {
    auto resolvedVelocity = Vector::make(velocity.dx + _carriedVelocity.dx, velocity.dy + _carriedVelocity.dy);

    updateCarriedVelocity(resolvedVelocity);

    return Valdi::makeShared<IOSFlingAnimation>(this, sourceContentOffset, resolvedVelocity, fast);
}

void IOSScroller::onDrag(GestureRecognizerState state, const DragEvent& event) {
    if (std::signbit(_carriedVelocity.dx) != std::signbit(event.velocity.dx)) {
        // Change direction, cancel all the carried velocity
        _carriedVelocity.dx = 0.0f;
    }

    if (std::signbit(_carriedVelocity.dy) != std::signbit(event.velocity.dy)) {
        // Change direction, cancel all the carried velocity
        _carriedVelocity.dy = 0.0f;
    }

    auto currentTime = event.time;

    if (state == GestureRecognizerStateBegan) {
        _dragStartTime = currentTime;
    } else if (state == GestureRecognizerStateEnded) {
        // Cancel carried velocity if the fling velocity is under our threshold
        if (std::abs(event.velocity.dx) < (std::abs(_carriedVelocity.dx) * kVelocityThreshold)) {
            _carriedVelocity.dx = 0;
        }
        if (std::abs(event.velocity.dy) < (std::abs(_carriedVelocity.dy) * kVelocityThreshold)) {
            _carriedVelocity.dy = 0;
        }

        if ((currentTime - _dragStartTime) > Duration::fromMilliseconds(kTimeThresholdInMilliseconds)) {
            // Cancel carried velocity if the fling took more than 250ms
            _carriedVelocity.dx = 0.0f;
            _carriedVelocity.dy = 0.0f;
        }
    }
}

void IOSScroller::reset() {
    _carriedVelocity.dx = 0.0f;
    _carriedVelocity.dy = 0.0f;
}

void IOSScroller::updateCarriedVelocity(Vector currentVelocity) {
    _carriedVelocity.dx = carriedVelocityComputation(currentVelocity.dx);
    _carriedVelocity.dy = carriedVelocityComputation(currentVelocity.dy);
}

Vector IOSScroller::computeDecelerationInitialVelocity(const Point& initialOffset,
                                                       const Point& finalOffset,
                                                       bool fast) {
    auto coef = getDecelerationCoef(fast);
    auto initialVelocityX = (finalOffset.x - initialOffset.x) * -coef;
    auto initialVelocityY = (finalOffset.y - initialOffset.y) * -coef;
    return Vector::make(initialVelocityX, initialVelocityY);
}

Scalar IOSScroller::getDecelerationCoef(bool fast) {
    if (fast) {
        return kDecelerationFastCoef;
    } else {
        return kDecelerationNormalCoef;
    }
}

Scalar IOSScroller::getDecelerationRate(bool fast) {
    if (fast) {
        return kDecelerationFastRate;
    } else {
        return kDecelerationNormalRate;
    }
}

Duration IOSScroller::computeDecelerationDuration(const Vector& velocity, bool fast) {
    auto velocityLength = Point::length(velocity.dx, velocity.dy);
    if (velocityLength == 0) {
        return Duration::fromSeconds(0);
    }
    auto coef = IOSScroller::getDecelerationCoef(fast);
    auto durationSeconds = std::log(-coef * kThreshold / velocityLength) / coef;
    return Duration::fromSeconds(durationSeconds);
}

Point IOSScroller::computeDecelerationOffset(const Point& contentOffset,
                                             const Vector& velocity,
                                             Duration elapsedTime,
                                             bool fast) {
    auto coef = IOSScroller::getDecelerationCoef(fast);
    auto rate = IOSScroller::getDecelerationRate(fast);
    // dx = x + ((((rate ^ ms) - 1) / coef) * vx)

    auto ratio = (std::pow(rate, elapsedTime.milliseconds()) - 1) / coef;
    auto deceleratedOffsetX = contentOffset.x + ratio * velocity.dx;
    auto deceleratedOffsetY = contentOffset.y + ratio * velocity.dy;
    return Point::make(deceleratedOffsetX, deceleratedOffsetY);
}

std::optional<Duration> IOSScroller::computeDecelerationTimeAtTargetOffset(Scalar sourceOffset,
                                                                           Scalar velocity,
                                                                           Scalar targetOffset,
                                                                           bool fast) {
    // Base equation:
    // dx = x + ((((rate ^ ms) - 1) / coef) * vx)
    // ((dx - x) / vx * coeff) + 1 = rate ^ ms

    auto distance = targetOffset - sourceOffset;

    if (velocity == 0.0f || std::signbit(distance) != std::signbit(velocity)) {
        return std::nullopt;
    }

    auto coef = IOSScroller::getDecelerationCoef(fast);
    auto rate = IOSScroller::getDecelerationRate(fast);
    auto base = (distance / velocity * coef) + 1.0f;

    auto timeMs = std::log(base) / std::log(rate);

    return Duration::fromMilliseconds(timeMs);
}

std::optional<Duration> IOSScroller::computeDecelerationTimeAtTargetOffset(const Point& contentOffset,
                                                                           const Vector& velocity,
                                                                           const Point& targetContentOffset,
                                                                           bool fast) {
    auto xDuration = computeDecelerationTimeAtTargetOffset(contentOffset.x, velocity.dx, targetContentOffset.x, fast);
    auto yDuration = computeDecelerationTimeAtTargetOffset(contentOffset.y, velocity.dy, targetContentOffset.y, fast);

    if (xDuration && yDuration) {
        return {std::min(xDuration.value(), yDuration.value())};
    } else if (xDuration) {
        return xDuration;
    } else if (yDuration) {
        return yDuration;
    } else {
        return std::nullopt;
    }
}

} // namespace snap::drawing
