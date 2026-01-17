//
//  ScrollGestureRecognizer.hpp
//  valdi-skia
//
//  Created by Simon Corsin on 6/29/20.
//

#pragma once

#include "snap_drawing/cpp/Touches/DragGestureRecognizer.hpp"
#include "snap_drawing/cpp/Touches/GesturesConfiguration.hpp"
#include "snap_drawing/cpp/Touches/MoveGestureRecognizer.hpp"
#include "snap_drawing/cpp/Utils/VelocityTracker.hpp"

namespace snap::drawing {

class ScrollGestureRecognizer : public MoveGestureRecognizer<DragEvent> {
public:
    explicit ScrollGestureRecognizer(const GesturesConfiguration& gesturesConfiguration);
    ~ScrollGestureRecognizer() override;

    bool requiresFailureOf(const GestureRecognizer& other) const override;

    void setHorizontal(bool horizontal);
    void setAnimatingScroll(bool animatingScroll);

protected:
    bool shouldStartMove(const TouchEvent& event) const override;
    bool shouldContinueMove(const TouchEvent& event) const override;

    void didStartMove(const TouchEvent& event) override;
    void didContinueMove(const TouchEvent& event) override;

    std::string_view getTypeName() const final;

private:
    DragEvent makeMoveEvent() const override;

private:
    Scalar _dragThreshold;
    bool _isHorizontal = false;
    bool _animatingScroll = false;

    VelocityTracker _horizontalVelocityTracker;
    VelocityTracker _verticalVelocityTracker;
};

} // namespace snap::drawing
