//
//  TapGestureRecognizer.cpp
//  valdi-skia
//
//  Created by Simon Corsin on 6/29/20.
//

#include "snap_drawing/cpp/Touches/TapGestureRecognizer.hpp"

namespace snap::drawing {

TapGestureRecognizer::TapGestureRecognizer(size_t numberOfTapsRequired,
                                           const GesturesConfiguration& gesturesConfiguration)
    : _numberOfTapsRequired(numberOfTapsRequired),
      _pressTimeout(gesturesConfiguration.doubleTapTimeout),
      _tapShiftTolerance(gesturesConfiguration.dragTouchSlop) {}

TapGestureRecognizer::~TapGestureRecognizer() = default;

bool TapGestureRecognizer::requiresFailureOf(const GestureRecognizer& other) const {
    return dynamic_cast<const TapGestureRecognizer*>(&other) != nullptr;
}

void TapGestureRecognizer::onProcess() {
    if (_listener) {
        _listener(*this, getState(), getLocation());
    }
}

void TapGestureRecognizer::onUpdate(const TouchEvent& event) {
    if (!_events.empty()) {
        auto& firstEvent = _events.front();
        auto distance = Point::distance(firstEvent.getLocationInWindow(), event.getLocationInWindow());
        if (distance >= _tapShiftTolerance) {
            transitionToState(GestureRecognizerStateFailed);
            return;
        }
        auto duration = event.getTime().durationSince(firstEvent.getTime());
        if (duration >= _pressTimeout) {
            transitionToState(GestureRecognizerStateFailed);
            return;
        }
    }

    switch (event.getType()) {
        case TouchEventTypeDown: {
            _events.emplace_back(event);
        } break;
        // adding/removing pointers treated as moving - the touch event is still active
        case TouchEventTypePointerUp:
            [[fallthrough]];
        case TouchEventTypePointerDown:
            [[fallthrough]];
        case TouchEventTypeWheel:
            [[fallthrough]];
        case TouchEventTypeIdle:
            [[fallthrough]];
        case TouchEventTypeMoved: {
        } break;
        case TouchEventTypeUp: {
            if (_events.size() == _numberOfTapsRequired) {
                transitionToState(GestureRecognizerStateBegan);
            }
        } break;
        case TouchEventTypeNone: {
            auto counter = _events.size();
            if (counter == 0 || counter >= _numberOfTapsRequired) {
                transitionToState(GestureRecognizerStateFailed);
                return;
            }
        } break;
    }

    if (_events.size() > _numberOfTapsRequired) {
        transitionToState(GestureRecognizerStateFailed);
        return;
    }
}

void TapGestureRecognizer::onStarted() {
    transitionToState(GestureRecognizerStateEnded);
}

void TapGestureRecognizer::onReset() {
    _events.clear();
}

} // namespace snap::drawing
