//
//  TouchGestureRecognizer.hpp
//  valdi-skia
//
//  Created by Simon Corsin on 6/29/20.
//

#pragma once

#include "snap_drawing/cpp/Touches/GestureRecognizer.hpp"
#include "snap_drawing/cpp/Touches/GesturesConfiguration.hpp"

namespace snap::drawing {

class TouchGestureRecognizer;
using TouchGestureRecognizerListener = GestureRecognizerListener<TouchGestureRecognizer, Point>;

struct TouchGestureRecognizerState {
    TouchEvent startEvent;

    explicit inline TouchGestureRecognizerState(const TouchEvent& startEvent) : startEvent(startEvent) {}
};

class TouchGestureRecognizer : public GestureRecognizer {
public:
    explicit TouchGestureRecognizer(const GesturesConfiguration& gesturesConfiguration);
    ~TouchGestureRecognizer() override;

    bool canRecognizeSimultaneously(const GestureRecognizer& other) const override;

    void setListener(TouchGestureRecognizerListener&& listener);

    void setOnStartListener(TouchGestureRecognizerListener&& onStartListener);
    void setOnEndListener(TouchGestureRecognizerListener&& onEndListener);

    void setOnTouchDelayDuration(Duration onTouchDelayDuration);

    bool isEmpty() const;

protected:
    void onUpdate(const TouchEvent& event) override;

    void onProcess() override;

    void onReset() override;

    std::string_view getTypeName() const final;

private:
    TouchGestureRecognizerListener _listener;
    TouchGestureRecognizerListener _onStartListener;
    TouchGestureRecognizerListener _onEndListener;
    Duration _onTouchDelayDuration;

    std::optional<TouchGestureRecognizerState> _touchGestureState;
};

} // namespace snap::drawing
