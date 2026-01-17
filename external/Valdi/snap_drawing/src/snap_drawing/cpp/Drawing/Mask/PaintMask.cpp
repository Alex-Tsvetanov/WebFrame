//
//  PaintMask.cpp
//  snap_drawing-pc
//
//  Created by Simon Corsin on 10/20/22.
//

#include "snap_drawing/cpp/Drawing/Mask/PaintMask.hpp"

#include "valdi_core/cpp/Utils/Format.hpp"
#include "valdi_core/cpp/Utils/StringCache.hpp"

#include "include/core/SkCanvas.h"

// This is a private header in Skia.
// This allows us to leverage the saveBehind API

class SkCanvasPriv {
public:
    // NOLINTNEXTLINE
    static int SaveBehind(SkCanvas* canvas, const SkRect* subset) {
        return canvas->only_axis_aligned_saveBehind(subset);
    }
};

namespace snap::drawing {

PaintMask::PaintMask(const Paint& paint, const Path& path, const Rect& rect)
    : _paint(paint), _path(path), _rect(rect) {}

PaintMask::~PaintMask() = default;

Rect PaintMask::getBounds() const {
    auto bounds = _path.getBounds();
    return bounds ? bounds.value() : _rect;
}

void PaintMask::prepare(SkCanvas* canvas) {
    auto bounds = getBounds();
    SkCanvasPriv::SaveBehind(canvas, &bounds.getSkValue());
}

void PaintMask::apply(SkCanvas* canvas) {
    if (_path.isEmpty()) {
        canvas->drawRect(_rect.getSkValue(), _paint.getSkValue());
    } else {
        canvas->drawPath(_path.getSkValue(), _paint.getSkValue());
    }

    canvas->restore();
}

String PaintMask::getDescription() {
    if (_path.isEmpty()) {
        return STRING_FORMAT("PaintMask with rect {}", _rect);
    } else {
        return STRING_FORMAT("PaintMask with path {}", _path);
    }
}

} // namespace snap::drawing
