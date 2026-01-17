//
//  TouchGestureRecognizer.cpp
//  valdi-skia
//
//  Created by Simon Corsin on 6/29/20.
//

#include "snap_drawing/cpp/Touches/TouchGestureRecognizer.hpp"

namespace snap::drawing {

TouchGestureRecognizer::TouchGestureRecognizer(const GesturesConfiguration& /*gesturesConfiguration*/) {}

TouchGestureRecognizer::~TouchGestureRecognizer() = default;

bool TouchGestureRecognizer::canRecognizeSimultaneously(const GestureRecognizer& /*other*/) const {
    return true;
}

void TouchGestureRecognizer::onUpdate(const TouchEvent& event) {
    switch (event.getType()) {
        case TouchEventTypeDown: {
            if (_onTouchDelayDuration <= Duration::fromSeconds(0)) {
                transitionToState(GestureRecognizerStateBegan);
            } else {
                _touchGestureState = {TouchGestureRecognizerState(event)};
            }
        } break;
        case TouchEventTypeWheel: {
        } break;
        case TouchEventTypeIdle: {
            if (!isActive() && _touchGestureState.has_value() &&
                event.getTime().durationSince(_touchGestureState->startEvent.getTime()) >= _onTouchDelayDuration) {
                transitionToState(GestureRecognizerStateBegan);
            }
        } break;
        // adding/removing pointers treated as moving - the touch event is still active
        case TouchEventTypePointerUp:
            [[fallthrough]];
        case TouchEventTypePointerDown:
            [[fallthrough]];
        case TouchEventTypeMoved: {
            if (isActive()) {
                transitionToState(GestureRecognizerStateChanged);
            } else if (_touchGestureState.has_value() &&
                       event.getTime().durationSince(_touchGestureState->startEvent.getTime()) >=
                           _onTouchDelayDuration) {
                transitionToState(GestureRecognizerStateBegan);
            }
        } break;
        case TouchEventTypeUp: {
            transitionToState(GestureRecognizerStateEnded);
        } break;
        case TouchEventTypeNone: {
            transitionToState(GestureRecognizerStateFailed);
        } break;
    }
}

void TouchGestureRecognizer::onProcess() {
    if (getState() == GestureRecognizerStateBegan) {
        if (_onStartListener) {
            _onStartListener(*this, getState(), getLocation());
        }
    } else if (getState() == GestureRecognizerStateEnded) {
        if (_onEndListener) {
            _onEndListener(*this, getState(), getLocation());
        }
    }

    if (getLastEvent()->getType() != TouchEventTypeIdle && _listener) {
        _listener(*this, getState(), getLocation());
    }
}

void TouchGestureRecognizer::setListener(TouchGestureRecognizerListener&& listener) {
    _listener = std::move(listener);
}

void TouchGestureRecognizer::setOnStartListener(TouchGestureRecognizerListener&& onStartListener) {
    _onStartListener = std::move(onStartListener);
}

void TouchGestureRecognizer::setOnEndListener(TouchGestureRecognizerListener&& onEndListener) {
    _onEndListener = std::move(onEndListener);
}

void TouchGestureRecognizer::setOnTouchDelayDuration(Duration onTouchDelayDuration) {
    _onTouchDelayDuration = onTouchDelayDuration;
}

bool TouchGestureRecognizer::isEmpty() const {
    return (!_listener && !_onStartListener && !_onEndListener && _onTouchDelayDuration == Duration::fromSeconds(0));
}

void TouchGestureRecognizer::onReset() {
    _touchGestureState = std::nullopt;
}

std::string_view TouchGestureRecognizer::getTypeName() const {
    return "touch";
}

} // namespace snap::drawing
