//
//  DrawingContext.cpp
//  valdi-skia-app
//
//  Created by Simon Corsin on 6/28/20.
//

#include "snap_drawing/cpp/Drawing/DrawingContext.hpp"
#include "snap_drawing/cpp/Drawing/Surface/ExternalSurface.hpp"
#include "snap_drawing/cpp/Utils/BitmapUtils.hpp"
#include "snap_drawing/cpp/Utils/BorderRadius.hpp"
#include "snap_drawing/cpp/Utils/LazyPath.hpp"

#include "valdi_core/cpp/Interfaces/IBitmap.hpp"

namespace snap::drawing {

DrawingContext::DrawingContext(Scalar width, Scalar height) : _drawBounds(Rect::makeXYWH(0, 0, width, height)) {}

DrawingContext::~DrawingContext() = default;

LayerContent DrawingContext::finish() {
    LayerContent content;
    if (_externalSurface != nullptr) {
        content.externalSurface = Valdi::makeShared<ExternalSurfaceSnapshot>(std::move(_externalSurface));
    }

    if (_canvas != nullptr) {
        content.picture = _recorder.finishRecordingAsPicture();
    }

    return content;
}

void DrawingContext::drawPaint(const Paint& paint, const BorderRadius& borderRadius, LazyPath& lazyPath) {
    drawPaint(paint, borderRadius, drawBounds(), lazyPath);
}

void DrawingContext::drawPaint(const Paint& paint,
                               const BorderRadius& borderRadius,
                               const Rect& targetRect,
                               LazyPath& lazyPath) {
    if (borderRadius.isEmpty()) {
        drawPaint(paint, targetRect);
    } else {
        if (lazyPath.update(targetRect.size())) {
            borderRadius.applyToPath(targetRect, lazyPath.path());
        }

        drawPaint(paint, lazyPath.path());
    }
}

void DrawingContext::drawPaint(const Paint& paint, const Path& path) {
    if (!path.isEmpty()) {
        canvas()->drawPath(path.getSkValue(), paint.getSkValue());
    }
}

void DrawingContext::drawPaint(const Paint& paint, const Rect& targetRect) {
    canvas()->drawRect(targetRect.getSkValue(), paint.getSkValue());
}

void DrawingContext::drawBitmap(const Valdi::Ref<Valdi::IBitmap>& bitmap, FittingSizeMode fittingSizeMode) {
    auto imageResult = Image::makeFromBitmap(bitmap, false);
    if (!imageResult) {
        return;
    }
    auto image = imageResult.moveValue();

    auto imageRect = Rect::makeLTRB(0, 0, static_cast<Scalar>(image->width()), static_cast<Scalar>(image->height()));
    auto targetRect = drawBounds().makeFittingSize(imageRect.size(), fittingSizeMode);

    drawImage(*image, imageRect, targetRect, nullptr);
}

void DrawingContext::drawImage(const Image& image, const Rect& imageRect, const Rect& targetRect, const Paint* paint) {
    canvas()->drawImageRect(image.getSkValue(),
                            imageRect.getSkValue(),
                            targetRect.getSkValue(),
                            SkSamplingOptions(),
                            paint != nullptr ? &paint->getSkValue() : nullptr,
                            SkCanvas::kStrict_SrcRectConstraint);
}

void DrawingContext::clipPath(const Path& path) {
    canvas()->clipPath(path.getSkValue(), true);
}

void DrawingContext::clipRect(const Rect& rect) {
    canvas()->clipRect(rect.getSkValue(), true);
}

void DrawingContext::concat(const Matrix& matrix) {
    canvas()->concat(matrix.getSkValue());
}

const Rect& DrawingContext::drawBounds() const {
    return _drawBounds;
}

void DrawingContext::drawExternalSurface(const Ref<ExternalSurface>& externalSurface) {
    SC_ASSERT(_externalSurface == nullptr);
    _externalSurface = externalSurface;
}

int DrawingContext::save() {
    return canvas()->save();
}

void DrawingContext::restore(int count) {
    canvas()->restoreToCount(count);
}

SkCanvas* DrawingContext::canvas() {
    if (_canvas == nullptr) {
        _canvas = _recorder.beginRecording(_drawBounds.getSkValue());
    }

    return _canvas;
}

} // namespace snap::drawing
