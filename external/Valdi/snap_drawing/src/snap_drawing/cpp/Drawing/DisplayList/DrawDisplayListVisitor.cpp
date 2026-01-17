//
//  DrawDisplayListVisitor.cpp
//  snap_drawing-macos
//
//  Created by Simon Corsin on 2/7/22.
//

#include "snap_drawing/cpp/Drawing/DisplayList/DrawDisplayListVisitor.hpp"

#include "include/core/SkCanvas.h"

#include "snap_drawing/cpp/Drawing/Mask/IMask.hpp"
#include "snap_drawing/cpp/Drawing/Paint.hpp"

namespace snap::drawing {

DrawDisplayListVisitor::DrawDisplayListVisitor(SkCanvas* canvas, Scalar scaleX, Scalar scaleY)
    : _canvas(canvas), _scaleX(scaleX), _scaleY(scaleY), _tempRect(Rect::makeEmpty()) {}

void DrawDisplayListVisitor::visit(const Operations::PushContext& pushContext) {
    if (pushContext.opacity == 1.0f) {
        _canvas->save();
    } else {
        Paint paint;
        paint.setAlpha(pushContext.opacity);
        _canvas->saveLayer(nullptr, &paint.getSkValue());
    }

    auto matrix = pushContext.matrix;

    matrix.setTranslateX(sanitizeScalarFromScale(matrix.getTranslateX(), _scaleX));
    matrix.setTranslateY(sanitizeScalarFromScale(matrix.getTranslateY(), _scaleY));

    _canvas->concat(matrix.getSkValue());
}

void DrawDisplayListVisitor::visit(const Operations::PopContext& /*popContext*/) {
    _canvas->restore();
}

void DrawDisplayListVisitor::visit(const Operations::DrawPicture& drawPicture) {
    if (drawPicture.opacity == 1.0f) {
        _canvas->drawPicture(drawPicture.picture);
    } else {
        Paint paint;
        paint.setAlpha(drawPicture.opacity);
        _canvas->drawPicture(drawPicture.picture, nullptr, &paint.getSkValue());
    }
}

void DrawDisplayListVisitor::visit(const Operations::ClipRect& clipRect) {
    _tempRect.right = clipRect.width;
    _tempRect.bottom = clipRect.height;

    _canvas->clipRect(_tempRect.getSkValue());
}

void DrawDisplayListVisitor::visit(const Operations::ClipRound& clipRound) {
    _tempRect.right = clipRound.width;
    _tempRect.bottom = clipRound.height;

    auto path = clipRound.borderRadius.getPath(_tempRect);

    if (!path.isEmpty()) {
        _canvas->clipPath(path.getSkValue());
    }
}

void DrawDisplayListVisitor::visit(const Operations::DrawExternalSurface& /*drawExternalSurface*/) {}

void DrawDisplayListVisitor::visit(const Operations::PrepareMask& prepareMask) {
    prepareMask.mask->prepare(_canvas);
}

void DrawDisplayListVisitor::visit(const Operations::ApplyMask& applyMask) {
    applyMask.mask->apply(_canvas);
}

} // namespace snap::drawing
