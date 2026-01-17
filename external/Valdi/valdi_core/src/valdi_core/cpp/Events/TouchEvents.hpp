//
//  TouchEvents.hpp
//  ValdiRuntime
//
//  Created by Simon Corsin on 7/12/18.
//  Copyright © 2018 Snap Inc. All rights reserved.
//

#pragma once

#include "valdi_core/cpp/Utils/SmallVector.hpp"
#include "valdi_core/cpp/Utils/Value.hpp"

namespace Valdi {

enum TouchEventState {
    TouchEventStateStarted = 0,
    TouchEventStateChanged,
    TouchEventStateEnded,
    TouchEventStatePossible,
};

namespace TouchEvents {

struct PointerData {
    double x;
    double y;
    int pointerId;

    PointerData(double x, double y, int pointerId) : x(x), y(y), pointerId(pointerId) {}
};

using PointerLocations = Valdi::SmallVector<PointerData, 2>;

Value makeTapEvent(TouchEventState state,
                   double x,
                   double y,
                   double absoluteX,
                   double absoluteY,
                   int pointerCount,
                   PointerLocations& pointerLocations);
Value makeDragEvent(TouchEventState state,
                    double x,
                    double y,
                    double absoluteX,
                    double absoluteY,
                    double deltaX,
                    double deltaY,
                    double velocityX,
                    double velocityY,
                    int pointerCount,
                    PointerLocations& pointerLocations);
Value makePinchEvent(TouchEventState state,
                     double x,
                     double y,
                     double absoluteX,
                     double absoluteY,
                     double scale,
                     int pointerCount,
                     PointerLocations& pointerLocations);
Value makeRotationEvent(TouchEventState state,
                        double x,
                        double y,
                        double absoluteX,
                        double absoluteY,
                        double rotation,
                        int pointerCount,
                        PointerLocations& pointerLocations);

Value makeScrollEvent(double x, double y, double unclampedX, double unclampedY, double velocityX, double velocityY);

}; // namespace TouchEvents

} // namespace Valdi
