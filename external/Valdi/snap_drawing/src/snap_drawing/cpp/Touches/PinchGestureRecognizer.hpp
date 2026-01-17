//
//  PinchGestureRecognizer.hpp
//  valdi-skia
//
//  Created by Simon Corsin on 6/29/20.
//

#pragma once

#include "snap_drawing/cpp/Touches/DragGestureRecognizer.hpp"
#include "snap_drawing/cpp/Touches/GestureRecognizer.hpp"
#include "snap_drawing/cpp/Touches/GesturesConfiguration.hpp"
#include "snap_drawing/cpp/Touches/MoveGestureRecognizer.hpp"

namespace snap::drawing {

struct PinchEvent : DragEvent {
    Scalar scale;
};

class PinchGestureRecognizer : public MoveGestureRecognizer<PinchEvent> {
public:
    explicit PinchGestureRecognizer(const GesturesConfiguration& gesturesConfiguration);
    ~PinchGestureRecognizer() override;

    bool requiresFailureOf(const GestureRecognizer& other) const override;
    bool canRecognizeSimultaneously(const GestureRecognizer& other) const override;

protected:
    std::string_view getTypeName() const final;

private:
    // track previous scales - such that multiple single scales can be all multiplied together
    Scalar _netScale = 1;

    Scalar getCurrentScale() const;
    bool shouldStartMove(const TouchEvent& event) const override;
    bool shouldContinueMove(const TouchEvent& event) const override;
    void onReset() override;
    void onPointerChange(const TouchEvent& event) override;

    PinchEvent makeMoveEvent() const override;
};

} // namespace snap::drawing
