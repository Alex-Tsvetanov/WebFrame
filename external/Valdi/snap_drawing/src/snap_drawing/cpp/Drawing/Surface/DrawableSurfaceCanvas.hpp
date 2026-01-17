//
//  DrawableSurfaceCanvas.hpp
//  snap_drawing
//
//  Created by Simon Corsin on 1/20/22.
//

#pragma once

#include "valdi_core/cpp/Utils/Bytes.hpp"
#include "valdi_core/cpp/Utils/Result.hpp"

#include "snap_drawing/cpp/Utils/Image.hpp"

class SkSurface;
class SkCanvas;

namespace snap::drawing {

class DrawableSurfaceCanvas {
public:
    DrawableSurfaceCanvas(SkSurface* surface, int width, int height);

    SkCanvas* getSkiaCanvas() const;
    SkSurface* getSkiaSurface() const;

    int getWidth() const;
    int getHeight() const;

    Valdi::Result<Ref<Image>> snapshot() const;

private:
    SkSurface* _surface;
    SkCanvas* _canvas;
    int _width;
    int _height;
};

} // namespace snap::drawing
