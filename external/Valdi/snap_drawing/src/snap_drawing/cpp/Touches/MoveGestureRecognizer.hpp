//
//  MoveGestureRecognizer.hpp
//  valdi-skia
//
//  Created by Vincent Brunet on 01/17/21.
//

#pragma once

#include "snap_drawing/cpp/Touches/GestureRecognizer.hpp"
#include "snap_drawing/cpp/Touches/TapGestureRecognizer.hpp"

namespace snap::drawing {

struct MoveGestureState {
    TouchEvent startEvent;
    TouchEvent lastEvent;
    TouchEvent currentEvent;

    MoveGestureState(const TouchEvent& startEvent, const TouchEvent& lastEvent, const TouchEvent& currentEvent);
};

template<class MoveEvent>
class MoveGestureRecognizer : public GestureRecognizer {
public:
    using ShouldBeginListener = GestureRecognizerShouldBeginListener<MoveGestureRecognizer, MoveEvent>;

    using Listener = GestureRecognizerListener<MoveGestureRecognizer, MoveEvent>;

    explicit MoveGestureRecognizer() = default;
    ~MoveGestureRecognizer() override = default;

    void setShouldBeginListener(ShouldBeginListener&& shouldBeginListener) {
        _shouldBeginListener = std::move(shouldBeginListener);
    }

    void setListener(Listener&& listener) {
        _listener = std::move(listener);
    }

    bool isEmpty() const {
        return (!_shouldBeginListener && !_listener);
    }

protected:
    bool shouldBegin() override {
        if (_shouldBeginListener) {
            return _shouldBeginListener(*this, makeMoveEvent());
        }
        return true;
    }

    virtual void onEnd(const TouchEvent& event) {
        transitionToState(GestureRecognizerStateEnded);
    }

    virtual void onPointerChange(const TouchEvent& event) {}

    void onProcess() override {
        if (_listener) {
            if (_moveState && _moveState->currentEvent.getType() == TouchEventTypeIdle) {
                return;
            }
            _listener(*this, getState(), makeMoveEvent());
        }
    }

    void onUpdate(const TouchEvent& event) override {
        if (!_moveState.has_value()) {
            _moveState = {MoveGestureState(event, event, event)};
        }

        if (event.getType() == TouchEventTypeNone || event.getType() == TouchEventTypeWheel) {
            transitionToState(GestureRecognizerStateFailed);
        } else {
            if (isActive()) {
                if (event.getType() == TouchEventTypePointerUp || event.getType() == TouchEventTypePointerDown) {
                    onPointerChange(event);
                } else if (event.getType() == TouchEventTypeUp || !shouldContinueMove(event)) {
                    onEnd(event);
                } else {
                    didContinueMove(event);
                }
            } else {
                if (shouldStartMove(event)) {
                    transitionToState(GestureRecognizerStateBegan);
                    _moveState = {MoveGestureState(event, event, event)};
                    didStartMove(event);
                } else if (event.getType() == TouchEventTypeUp) {
                    transitionToState(GestureRecognizerStateFailed);
                }
            }
        }

        _moveState.value().lastEvent = _moveState.value().currentEvent;
        _moveState.value().currentEvent = event;
    }

    void onReset() override {
        _moveState = std::nullopt;
    }

    virtual MoveEvent makeMoveEvent() const {
        SC_ASSERT(_moveState.has_value());

        auto startEvent = _moveState.value().startEvent;
        auto lastEvent = _moveState.value().lastEvent;
        auto currentEvent = _moveState.value().currentEvent;

        auto startLocationInWindow = startEvent.getLocationInWindow();
        auto currentLocationInWindow = currentEvent.getLocationInWindow();

        MoveEvent moveEvent;
        moveEvent.location = getLocation();
        moveEvent.offset.width = currentLocationInWindow.x - startLocationInWindow.x;
        moveEvent.offset.height = currentLocationInWindow.y - startLocationInWindow.y;
        moveEvent.velocity = computeVelocity(lastEvent, currentEvent);
        moveEvent.time = currentEvent.getTime();
        moveEvent.pointerCount = currentEvent.getPointerCount();

        return moveEvent;
    }

    virtual bool shouldStartMove(const TouchEvent& event) const = 0;
    virtual bool shouldContinueMove(const TouchEvent& event) const = 0;

    virtual void didStartMove(const TouchEvent& event) {}
    virtual void didContinueMove(const TouchEvent& event) {}

    std::optional<MoveGestureState> _moveState;

private:
    ShouldBeginListener _shouldBeginListener;

    Listener _listener;

    static Vector computeVelocity(const TouchEvent& previousEvent, const TouchEvent& newEvent) {
        auto delta = newEvent.getTime() - previousEvent.getTime();

        auto fSeconds = static_cast<Scalar>(delta.seconds());
        if (fSeconds == 0) {
            return Vector::makeEmpty();
        }

        auto previousLocationInWindow = previousEvent.getLocationInWindow();
        auto newLocationInWindow = newEvent.getLocationInWindow();

        auto velocityX = (newLocationInWindow.x - previousLocationInWindow.x) / fSeconds;
        auto velocityY = (newLocationInWindow.y - previousLocationInWindow.y) / fSeconds;

        return Vector::make(velocityX, velocityY);
    }
};

} // namespace snap::drawing
