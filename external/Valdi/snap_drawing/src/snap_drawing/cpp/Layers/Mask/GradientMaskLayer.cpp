//
//  GradientMaskLayer.cpp
//  snap_drawing-pc
//
//  Created by Simon Corsin on 10/20/22.
//

#include "snap_drawing/cpp/Layers/Mask/GradientMaskLayer.hpp"

namespace snap::drawing {

GradientMaskLayer::GradientMaskLayer() = default;
GradientMaskLayer::~GradientMaskLayer() = default;

const LinearGradient& GradientMaskLayer::getGradient() const {
    return _gradient;
}

LinearGradient& GradientMaskLayer::getGradient() {
    return _gradient;
}

void GradientMaskLayer::onConfigurePaint(Paint& paint, const Rect& bounds) {
    PaintMaskLayer::onConfigurePaint(paint, bounds);

    _gradient.update(bounds);
    _gradient.applyToPaint(paint);
}

} // namespace snap::drawing
