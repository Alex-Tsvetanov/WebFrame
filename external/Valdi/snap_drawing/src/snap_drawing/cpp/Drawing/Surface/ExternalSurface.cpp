//
//  ExternalSurface.cpp
//  snap_drawing-macos
//
//  Created by Simon Corsin on 2/1/22.
//

#include "snap_drawing/cpp/Drawing/Surface/ExternalSurface.hpp"
#include "snap_drawing/cpp/Utils/Image.hpp"
#include "valdi_core/cpp/Interfaces/IBitmapFactory.hpp"

namespace snap::drawing {

ExternalSurface::ExternalSurface() = default;
ExternalSurface::~ExternalSurface() = default;

Ref<Valdi::IBitmapFactory> ExternalSurface::getRasterBitmapFactory() const {
    return nullptr;
}

Valdi::Result<Valdi::Void> ExternalSurface::rasterInto(const Ref<Valdi::IBitmap>& bitmap,
                                                       const Rect& frame,
                                                       const Matrix& transform,
                                                       float rasterScaleX,
                                                       float rasterScaleY) {
    return Valdi::Error("rasterInto() not implemented");
}

const Size& ExternalSurface::getRelativeSize() const {
    return _relativeSize;
}

void ExternalSurface::setRelativeSize(const Size& relativeSize) {
    _relativeSize = relativeSize;
}

ExternalSurfaceSnapshot::ExternalSurfaceSnapshot(Ref<ExternalSurface> externalSurface)
    : _externalSurface(std::move(externalSurface)) {}

ExternalSurfaceSnapshot::~ExternalSurfaceSnapshot() = default;

const Ref<ExternalSurface>& ExternalSurfaceSnapshot::getExternalSurface() const {
    return _externalSurface;
}

} // namespace snap::drawing
