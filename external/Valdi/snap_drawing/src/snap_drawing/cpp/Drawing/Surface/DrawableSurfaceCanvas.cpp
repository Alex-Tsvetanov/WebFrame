//
//  DrawableSurfaceCanvas.cpp
//  snap_drawing
//
//  Created by Simon Corsin on 1/20/22.
//

#include "snap_drawing/cpp/Drawing/Surface/DrawableSurfaceCanvas.hpp"
#include "include/core/SkSurface.h"
#include "snap_drawing/cpp/Utils/BytesUtils.hpp"

namespace snap::drawing {

DrawableSurfaceCanvas::DrawableSurfaceCanvas(SkSurface* surface, int width, int height)
    : _surface(surface), _canvas(surface->getCanvas()), _width(width), _height(height) {}

SkCanvas* DrawableSurfaceCanvas::getSkiaCanvas() const {
    return _canvas;
}

SkSurface* DrawableSurfaceCanvas::getSkiaSurface() const {
    return _surface;
}

int DrawableSurfaceCanvas::getWidth() const {
    return _width;
}

int DrawableSurfaceCanvas::getHeight() const {
    return _height;
}

Valdi::Result<Ref<Image>> DrawableSurfaceCanvas::snapshot() const {
    auto snapshot = getSkiaSurface()->makeImageSnapshot();
    if (snapshot == nullptr) {
        return Valdi::Error("Failed to create snapshot");
    }

    return Ref<Image>(Valdi::makeShared<Image>(snapshot));
}

} // namespace snap::drawing
