//
//  TouchEvents.cpp
//  ValdiRuntime
//
//  Created by Simon Corsin on 7/12/18.
//  Copyright © 2018 Snap Inc. All rights reserved.
//

#include "valdi_core/cpp/Events/TouchEvents.hpp"
#include "valdi_core/cpp/Utils/ConsoleLogger.hpp"
#include "valdi_core/cpp/Utils/StringCache.hpp"
#include "valdi_core/cpp/Utils/TimePoint.hpp"
#include "valdi_core/cpp/Utils/ValueArrayBuilder.hpp"
#include "valdi_core/cpp/Utils/ValueMap.hpp"

#include <chrono>

namespace Valdi {

void fillEventTime(ValueMap& map) {
    static auto kEventTimeKey = STRING_LITERAL("eventTime");
    map[kEventTimeKey] = Value(TimePoint::now().getTime());
}

void fillPosition(ValueMap& map, double x, double y) {
    static auto xKey = STRING_LITERAL("x");
    static auto yKey = STRING_LITERAL("y");
    map[xKey] = Value(x);
    map[yKey] = Value(y);
}

void fillOverscrollTension(ValueMap& map, double x, double y, double unclampedX, double unclampedY) {
    static auto overscrollTensionXKey = STRING_LITERAL("overscrollTensionX");
    static auto overscrollTensionYKey = STRING_LITERAL("overscrollTensionY");
    map[overscrollTensionXKey] = Value(unclampedX - x);
    map[overscrollTensionYKey] = Value(unclampedY - y);
}

void fillVelocity(ValueMap& map, double velocityX, double velocityY) {
    static auto velocityXKey = STRING_LITERAL("velocityX");
    static auto velocityYKey = STRING_LITERAL("velocityY");
    map[velocityXKey] = Value(velocityX);
    map[velocityYKey] = Value(velocityY);
}

void fillEvent(ValueMap& map, Valdi::TouchEventState state, double x, double y, double absoluteX, double absoluteY) {
    static auto absoluteXKey = STRING_LITERAL("absoluteX");
    static auto absoluteYKey = STRING_LITERAL("absoluteY");
    static auto stateKey = STRING_LITERAL("state");

    map.reserve(6);
    fillEventTime(map);
    fillPosition(map, x, y);
    map[absoluteXKey] = Value(absoluteX);
    map[absoluteYKey] = Value(absoluteY);
    map[stateKey] = Value(static_cast<int32_t>(state));
}

void fillPointerData(ValueMap& map, int pointerCount, Valdi::TouchEvents::PointerLocations& pointerLocations) {
    static auto pointerCountKey = STRING_LITERAL("pointerCount");
    static auto pointerLocationsKey = STRING_LITERAL("pointerLocations");
    static auto xKey = STRING_LITERAL("x");
    static auto yKey = STRING_LITERAL("y");
    static auto pointerIdKey = STRING_LITERAL("pointerId");

    auto array = ValueArray::make(pointerLocations.size());
    for (size_t i = 0; i < pointerLocations.size(); i++) {
        const Valdi::TouchEvents::PointerData& pointer = pointerLocations[i];
        auto pointerMap = makeShared<ValueMap>();
        pointerMap->reserve(3);
        (*pointerMap)[xKey] = Value(pointer.x);
        (*pointerMap)[yKey] = Value(pointer.y);
        (*pointerMap)[pointerIdKey] = Value(pointer.pointerId);
        array->emplace(i, Value(pointerMap));
    }

    map[pointerCountKey] = Value(pointerCount);
    map[pointerLocationsKey] = Value(array);
}

Value TouchEvents::makeDragEvent(Valdi::TouchEventState state,
                                 double x,
                                 double y,
                                 double absoluteX,
                                 double absoluteY,
                                 double deltaX,
                                 double deltaY,
                                 double velocityX,
                                 double velocityY,
                                 int pointerCount,
                                 TouchEvents::PointerLocations& pointerLocations) {
    static auto deltaXKey = STRING_LITERAL("deltaX");
    static auto deltaYKey = STRING_LITERAL("deltaY");

    auto map = makeShared<ValueMap>();
    fillEvent(*map, state, x, y, absoluteX, absoluteY);
    fillVelocity(*map, velocityX, velocityY);
    fillPointerData(*map, pointerCount, pointerLocations);
    (*map)[deltaXKey] = Value(deltaX);
    (*map)[deltaYKey] = Value(deltaY);

    return Value(map);
}

Value TouchEvents::makePinchEvent(Valdi::TouchEventState state,
                                  double x,
                                  double y,
                                  double absoluteX,
                                  double absoluteY,
                                  double scale,
                                  int pointerCount,
                                  Valdi::TouchEvents::PointerLocations& pointerLocations) {
    static auto scaleKey = STRING_LITERAL("scale");

    auto map = makeShared<ValueMap>();
    fillEvent(*map, state, x, y, absoluteX, absoluteY);
    fillPointerData(*map, pointerCount, pointerLocations);
    (*map)[scaleKey] = Value(scale);

    return Value(map);
}

Value TouchEvents::makeRotationEvent(Valdi::TouchEventState state,
                                     double x,
                                     double y,
                                     double absoluteX,
                                     double absoluteY,
                                     double rotation,
                                     int pointerCount,
                                     Valdi::TouchEvents::PointerLocations& pointerLocations) {
    static auto rotationKey = STRING_LITERAL("rotation");

    auto map = makeShared<ValueMap>();
    fillEvent(*map, state, x, y, absoluteX, absoluteY);
    fillPointerData(*map, pointerCount, pointerLocations);
    (*map)[rotationKey] = Value(rotation);

    return Value(map);
}

Value TouchEvents::makeTapEvent(Valdi::TouchEventState state,
                                double x,
                                double y,
                                double absoluteX,
                                double absoluteY,
                                int pointerCount,
                                Valdi::TouchEvents::PointerLocations& pointerLocations) {
    auto map = makeShared<ValueMap>();
    fillEvent(*map, state, x, y, absoluteX, absoluteY);
    fillPointerData(*map, pointerCount, pointerLocations);

    return Value(map);
}

Value TouchEvents::makeScrollEvent(
    double x, double y, double unclampedX, double unclampedY, double velocityX, double velocityY) {
    auto map = makeShared<ValueMap>();
    map->reserve(7);
    fillEventTime(*map);
    fillPosition(*map, x, y);
    fillOverscrollTension(*map, x, y, unclampedX, unclampedY);
    fillVelocity(*map, velocityX, velocityY);
    return Value(map);
}

} // namespace Valdi
