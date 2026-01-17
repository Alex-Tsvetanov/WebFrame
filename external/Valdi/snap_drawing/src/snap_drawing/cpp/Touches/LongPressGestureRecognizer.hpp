//
//  LongPressGestureRecognizer.hpp
//  valdi-skia
//
//  Created by Simon Corsin on 6/29/20.
//

#pragma once

#include "snap_drawing/cpp/Touches/GestureRecognizer.hpp"
#include "snap_drawing/cpp/Touches/GesturesConfiguration.hpp"

#include <optional>

namespace snap::drawing {

struct LongPressGestureRecognizerState {
    TouchEvent startEvent;

    explicit inline LongPressGestureRecognizerState(const TouchEvent& startEvent) : startEvent(startEvent) {}
};

class LongPressGestureRecognizer : public GestureRecognizer {
public:
    using ShouldBeginListener = GestureRecognizerShouldBeginListener<LongPressGestureRecognizer, Point>;
    using Listener = GestureRecognizerListener<LongPressGestureRecognizer, Point>;

    LongPressGestureRecognizer(const GesturesConfiguration& gesturesConfiguration);
    ~LongPressGestureRecognizer() override;

    void setShouldBeginListener(ShouldBeginListener&& shouldBeginListener) {
        _shouldBeginListener = std::move(shouldBeginListener);
    }

    void setListener(Listener&& listener) {
        _listener = std::move(listener);
    }

    bool isEmpty() const {
        return (!_shouldBeginListener && !_listener);
    }

    void setLongPressTimeout(Duration longPressTimeout);

protected:
    bool shouldBegin() override {
        if (_shouldBeginListener) {
            return _shouldBeginListener(*this, getLocation());
        }
        return true;
    }

    void onProcess() override;

    void onUpdate(const TouchEvent& event) override;
    void onReset() override;

    std::string_view getTypeName() const final;

private:
    ShouldBeginListener _shouldBeginListener;

    Duration _longPressTimeout;
    Scalar _longPressShiftTolerance;
    Listener _listener;

    std::optional<LongPressGestureRecognizerState> _longPressState;
};

} // namespace snap::drawing
