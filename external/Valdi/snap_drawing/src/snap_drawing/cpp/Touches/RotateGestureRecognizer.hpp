//
//  RotateGestureRecognizer.hpp
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

struct RotateEvent : DragEvent {
    Scalar rotation;
};

class RotateGestureRecognizer : public MoveGestureRecognizer<RotateEvent> {
public:
    RotateGestureRecognizer(const GesturesConfiguration& gesturesConfiguration);
    ~RotateGestureRecognizer() override;

    bool requiresFailureOf(const GestureRecognizer& other) const override;
    bool canRecognizeSimultaneously(const GestureRecognizer& other) const override;

protected:
    std::string_view getTypeName() const final;

private:
    // track previous rotations - such that multiple single rotations can be all added together
    Scalar _netRotation = 0;

    Scalar getCurrentRotation() const;
    bool shouldStartMove(const TouchEvent& event) const override;
    bool shouldContinueMove(const TouchEvent& event) const override;
    void onReset() override;
    void onPointerChange(const TouchEvent& event) override;

    RotateEvent makeMoveEvent() const override;
};

} // namespace snap::drawing
