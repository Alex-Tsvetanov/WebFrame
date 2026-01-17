#pragma once

#include "snap_drawing/cpp/Touches/GestureRecognizer.hpp"

namespace snap::drawing {

struct WheelScrollEvent {
    Point location;
    Vector direction;
};

class WheelGestureRecognizer;
using WheelGestureRecognizerListener = GestureRecognizerListener<WheelGestureRecognizer, WheelScrollEvent>;

class WheelGestureRecognizer : public GestureRecognizer {
public:
    explicit WheelGestureRecognizer(WheelGestureRecognizerListener&& listener);
    ~WheelGestureRecognizer() override;

    bool canRecognizeSimultaneously(const GestureRecognizer& other) const override;

protected:
    void onUpdate(const TouchEvent& event) override;
    void onProcess() override;

    std::string_view getTypeName() const final;

private:
    WheelGestureRecognizerListener _listener;
};

} // namespace snap::drawing
