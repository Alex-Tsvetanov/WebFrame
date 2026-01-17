//
//  BoxShadow.cpp
//  valdi-desktop-apple
//
//  Created by Simon Corsin on 7/8/20.
//

#include "snap_drawing/cpp/Drawing/BoxShadow.hpp"

#include "include/core/SkBlurTypes.h"
#include "include/core/SkMaskFilter.h"
#include "snap_drawing/cpp/Utils/BorderRadius.hpp"

namespace snap::drawing {

BoxShadow::BoxShadow() = default;

BoxShadow::~BoxShadow() = default;

void BoxShadow::setOffset(Size offset) {
    _offset = offset;
}

void BoxShadow::setBlurAmount(Scalar blurAmount) {
    if (_blurAmount != blurAmount) {
        _blurAmount = blurAmount;
        if (blurAmount == 0) {
            _paint.getSkValue().setMaskFilter(nullptr);
        } else {
            _paint.getSkValue().setMaskFilter(SkMaskFilter::MakeBlur(SkBlurStyle::kNormal_SkBlurStyle, blurAmount * 2));
        }
    }
}

void BoxShadow::setColor(Color color) {
    _color = color;

    _paint.setColor(color);
}

void BoxShadow::draw(DrawingContext& drawingContext, const BorderRadius& borderRadius) {
    auto drawBounds = drawingContext.drawBounds().makeOffset(_offset.width, _offset.height);

    drawingContext.drawPaint(_paint, borderRadius, drawBounds, _lazyPath);
}

} // namespace snap::drawing
