//
//  SpringFlingScrollLayerAnimation.cpp
//  snap_drawing
//
//  Created by Simon Corsin on 11/28/22.
//

#include "snap_drawing/cpp/Layers/Scroll/SpringFlingScrollLayerAnimation.hpp"

namespace snap::drawing {

SpringFlingScrollLayerAnimation::SpringFlingScrollLayerAnimation() = default;
SpringFlingScrollLayerAnimation::~SpringFlingScrollLayerAnimation() = default;

bool SpringFlingScrollLayerAnimation::update(ScrollLayer& scrollLayer, Duration delta) {
    _elapsed += delta;

    if (_springScrollPhysics) {
        return onBounce(scrollLayer);
    } else {
        return onDecelerate(scrollLayer, _elapsed);
    }
}

bool SpringFlingScrollLayerAnimation::startBouncing(
    ScrollLayer& scrollLayer,
    const Vector& velocity,
    const Point& sourceContentOffset,
    const Point& targetContentOffset,
    const Duration& startTime,
    const SpringScrollPhysicsConfiguration* scrollPhysicsConfiguration) {
    _targetContentOffset = targetContentOffset;

    auto displacement =
        Vector::make(sourceContentOffset.x - targetContentOffset.x, sourceContentOffset.y - targetContentOffset.y);
    _elapsed = startTime;
    _springScrollPhysics = {SpringScrollPhysics(scrollPhysicsConfiguration, velocity, displacement)};

    return onBounce(scrollLayer);
}

bool SpringFlingScrollLayerAnimation::onBounce(ScrollLayer& scrollLayer) {
    auto& springScrollPhysics = _springScrollPhysics.value();

    auto result = springScrollPhysics.compute(_elapsed);

    updateCarriedVelocity(result.velocity);

    auto adjustment = applyContentOffset(
        scrollLayer,
        Point::make(_targetContentOffset.x + result.distance.dx, _targetContentOffset.y + result.distance.dy));
    _targetContentOffset.x += adjustment.dx;
    _targetContentOffset.y += adjustment.dy;

    return result.finished;
}

} // namespace snap::drawing
