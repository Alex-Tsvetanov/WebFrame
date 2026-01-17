//
//  LongPressGestureRecognizer.cpp
//  valdi-skia
//
//  Created by Simon Corsin on 6/29/20.
//

#include "snap_drawing/cpp/Touches/LongPressGestureRecognizer.hpp"

namespace snap::drawing {

LongPressGestureRecognizer::LongPressGestureRecognizer(const GesturesConfiguration& gesturesConfiguration)
    : _longPressTimeout(gesturesConfiguration.longPressTimeout),
      _longPressShiftTolerance(gesturesConfiguration.dragTouchSlop) {}

LongPressGestureRecognizer::~LongPressGestureRecognizer() = default;

void LongPressGestureRecognizer::setLongPressTimeout(Duration longPressTimeout) {
    _longPressTimeout = longPressTimeout;
}

void LongPressGestureRecognizer::onProcess() {
    if (_listener) {
        _listener(*this, getState(), getLocation());
    }
}

void LongPressGestureRecognizer::onUpdate(const TouchEvent& event) {
    switch (event.getType()) {
        case TouchEventTypeDown: {
            // Cancel the long press if we have a second down event
            if (_longPressState.has_value()) {
                transitionToState(GestureRecognizerStateFailed);
            } else {
                _longPressState = {LongPressGestureRecognizerState(event)};
            }
        } break;
        case TouchEventTypePointerUp:
            [[fallthrough]];
        case TouchEventTypePointerDown:
            [[fallthrough]];
        case TouchEventTypeIdle:
            [[fallthrough]];
        case TouchEventTypeMoved: {
            if (isActive()) {
                transitionToState(GestureRecognizerStateChanged);
            } else if (_longPressState.has_value()) {
                auto distance =
                    Point::distance(_longPressState->startEvent.getLocationInWindow(), event.getLocationInWindow());
                if (distance > _longPressShiftTolerance) {
                    transitionToState(GestureRecognizerStateFailed);
                } else if (event.getTime().durationSince(_longPressState->startEvent.getTime()) >= _longPressTimeout) {
                    transitionToState(GestureRecognizerStateBegan);
                }
            }
        } break;
        case TouchEventTypeUp: {
            if (isActive()) {
                transitionToState(GestureRecognizerStateEnded);
            } else {
                transitionToState(GestureRecognizerStateFailed);
            }
        } break;
        case TouchEventTypeWheel:
            [[fallthrough]];
        case TouchEventTypeNone:
            transitionToState(GestureRecognizerStateFailed);
            break;
    }
}

void LongPressGestureRecognizer::onReset() {
    _longPressState = std::nullopt;
}

std::string_view LongPressGestureRecognizer::getTypeName() const {
    return "long press";
}

} // namespace snap::drawing
