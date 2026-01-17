//
//  ScrollGestureRecognizer.cpp
//  valdi-skia
//
//  Created by Simon Corsin on 6/29/20.
//

#include "snap_drawing/cpp/Touches/ScrollGestureRecognizer.hpp"
#include "snap_drawing/cpp/Touches/LongPressGestureRecognizer.hpp"
#include "snap_drawing/cpp/Touches/PinchGestureRecognizer.hpp"
#include "snap_drawing/cpp/Touches/RotateGestureRecognizer.hpp"

#include "utils/debugging/Assert.hpp"

#include <cmath>

namespace snap::drawing {

constexpr Scalar kScrollVelocityThreshold = 50;

ScrollGestureRecognizer::ScrollGestureRecognizer(const GesturesConfiguration& gesturesConfiguration)
    : _dragThreshold(gesturesConfiguration.dragTouchSlop) {}

ScrollGestureRecognizer::~ScrollGestureRecognizer() = default;

void ScrollGestureRecognizer::setHorizontal(bool horizontal) {
    _isHorizontal = horizontal;
}

void ScrollGestureRecognizer::setAnimatingScroll(bool animatingScroll) {
    _animatingScroll = animatingScroll;
}

bool ScrollGestureRecognizer::requiresFailureOf(const GestureRecognizer& /*other*/) const {
    return false;
}

DragEvent ScrollGestureRecognizer::makeMoveEvent() const {
    auto event = MoveGestureRecognizer<DragEvent>::makeMoveEvent();
    if (_isHorizontal) {
        event.velocity.dy = 0;
        event.velocity.dx = -_horizontalVelocityTracker.computeVelocity();
        if (fabs(event.velocity.dx) < kScrollVelocityThreshold) {
            event.velocity.dx = 0;
        }
    } else {
        event.velocity.dx = 0;
        event.velocity.dy = -_verticalVelocityTracker.computeVelocity();
        if (fabs(event.velocity.dy) < kScrollVelocityThreshold) {
            event.velocity.dy = 0;
        }
    }
    return event;
}

bool ScrollGestureRecognizer::shouldStartMove(const TouchEvent& event) const {
    // Immediately start the scroll on down if we are animating.
    if (_animatingScroll) {
        return true;
    }

    auto startEvent = _moveState.value().startEvent;
    const auto& testedEvent = event;

    auto startLocationInWindow = startEvent.getLocationInWindow();
    auto testedLocationInWindow = testedEvent.getLocationInWindow();

    auto distance = Point::distance(startLocationInWindow, testedLocationInWindow);
    if (distance < _dragThreshold) {
        return false;
    }

    auto diffX = testedLocationInWindow.x - startLocationInWindow.x;
    auto diffY = testedLocationInWindow.y - startLocationInWindow.y;

    if (_isHorizontal) {
        return std::fabs(diffX) > std::fabs(diffY);
    } else {
        return std::fabs(diffY) > std::fabs(diffX);
    }
}

bool ScrollGestureRecognizer::shouldContinueMove(const TouchEvent& event) const {
    return _moveState->startEvent.getPointerCount() == event.getPointerCount();
}

void ScrollGestureRecognizer::didStartMove(const TouchEvent& event) {
    _horizontalVelocityTracker.clear();
    _verticalVelocityTracker.clear();
    didContinueMove(event);
}

void ScrollGestureRecognizer::didContinueMove(const TouchEvent& event) {
    auto locationInWindow = event.getLocationInWindow();
    _horizontalVelocityTracker.addSample(event.getTime(), locationInWindow.x);
    _verticalVelocityTracker.addSample(event.getTime(), locationInWindow.y);
}

std::string_view ScrollGestureRecognizer::getTypeName() const {
    return "scroll";
}
} // namespace snap::drawing
