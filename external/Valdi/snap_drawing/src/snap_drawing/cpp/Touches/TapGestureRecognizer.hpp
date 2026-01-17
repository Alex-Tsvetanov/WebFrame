//
//  TapGestureRecognizer.hpp
//  valdi-skia
//
//  Created by Simon Corsin on 6/29/20.
//

#pragma once

#include "snap_drawing/cpp/Touches/GestureRecognizer.hpp"
#include "snap_drawing/cpp/Touches/GesturesConfiguration.hpp"

#include <optional>

namespace snap::drawing {

class TapGestureRecognizer : public GestureRecognizer {
public:
    using ShouldBeginListener = GestureRecognizerShouldBeginListener<TapGestureRecognizer, Point>;
    using Listener = GestureRecognizerListener<TapGestureRecognizer, Point>;

    TapGestureRecognizer(size_t numberOfTapsRequired, const GesturesConfiguration& gesturesConfiguration);
    ~TapGestureRecognizer() override;

    void setShouldBeginListener(ShouldBeginListener&& shouldBeginListener) {
        _shouldBeginListener = std::move(shouldBeginListener);
    }

    void setListener(Listener&& listener) {
        _listener = std::move(listener);
    }

    bool isEmpty() const {
        return (!_shouldBeginListener && !_listener);
    }

    bool requiresFailureOf(const GestureRecognizer& other) const override;

protected:
    bool shouldBegin() override {
        if (_shouldBeginListener) {
            return _shouldBeginListener(*this, getLocation());
        }
        return true;
    }

    void onUpdate(const TouchEvent& event) override;
    void onReset() override;

    void onProcess() override;

    void onStarted() override;

private:
    ShouldBeginListener _shouldBeginListener;

    size_t _numberOfTapsRequired;
    Duration _pressTimeout;
    Scalar _tapShiftTolerance;

    Listener _listener;

    std::vector<TouchEvent> _events;
};

} // namespace snap::drawing
