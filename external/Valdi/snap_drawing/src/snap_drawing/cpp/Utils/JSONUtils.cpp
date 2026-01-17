//
//  JSONUtils.cpp
//  snap_drawing
//
//  Created by Simon Corsin on 3/10/22.
//

#include "snap_drawing/cpp/Utils/JSONUtils.hpp"

namespace snap::drawing {

constexpr double kJSONPrecision = 1000;

static Valdi::Value toJSONValue(Scalar value) {
    return Valdi::Value(round(static_cast<double>(value) * kJSONPrecision) / kJSONPrecision);
}

Valdi::Value toJSONValue(const Rect& rect) {
    return Valdi::Value()
        .setMapValue("x", toJSONValue(rect.x()))
        .setMapValue("y", toJSONValue(rect.y()))
        .setMapValue("width", toJSONValue(rect.width()))
        .setMapValue("height", toJSONValue(rect.height()));
}

Valdi::Value toJSONValue(const Size& size) {
    return Valdi::Value().setMapValue("width", toJSONValue(size.width)).setMapValue("height", toJSONValue(size.height));
}

} // namespace snap::drawing
