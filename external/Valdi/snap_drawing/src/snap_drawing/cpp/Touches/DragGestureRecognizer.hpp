//
//  DragGestureRecognizer.hpp
//  valdi-skia
//
//  Created by Simon Corsin on 6/29/20.
//

#pragma once

#include "snap_drawing/cpp/Touches/GestureRecognizer.hpp"
#include "snap_drawing/cpp/Touches/GesturesConfiguration.hpp"
#include "snap_drawing/cpp/Touches/MoveGestureRecognizer.hpp"

namespace snap::drawing {

struct DragEvent {
    Point location;
    Size offset;
    Vector velocity;
    TimePoint time;
    size_t pointerCount;

    DragEvent() : time(0), pointerCount(0) {};
};

class DragGestureRecognizer : public MoveGestureRecognizer<DragEvent> {
public:
    explicit DragGestureRecognizer(const GesturesConfiguration& gesturesConfiguration);
    ~DragGestureRecognizer() override;

    bool requiresFailureOf(const GestureRecognizer& other) const override;
    bool canRecognizeSimultaneously(const GestureRecognizer& other) const override;

protected:
    bool shouldStartMove(const TouchEvent& event) const override;
    bool shouldContinueMove(const TouchEvent& event) const override;

    std::string_view getTypeName() const final;

private:
    DragEvent makeMoveEvent() const override;

    Scalar _dragThreshold;
};

} // namespace snap::drawing
