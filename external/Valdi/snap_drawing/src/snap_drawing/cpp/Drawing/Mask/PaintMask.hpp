//
//  PaintMask.hpp
//  snap_drawing-pc
//
//  Created by Simon Corsin on 10/20/22.
//

#pragma once

#include "snap_drawing/cpp/Drawing/Mask/IMask.hpp"
#include "snap_drawing/cpp/Drawing/Paint.hpp"

#include "snap_drawing/cpp/Utils/BorderRadius.hpp"
#include "snap_drawing/cpp/Utils/Path.hpp"

namespace snap::drawing {

/**
 A PaintMask is an implementation of IMask that prepares the canvas
 using SkCanvas::saveBehind and draws a region from a given rect or path
 with a given paint.
 */
class PaintMask : public IMask {
public:
    PaintMask(const Paint& paint, const Path& path, const Rect& rect);
    ~PaintMask() override;

    Rect getBounds() const override;

    void prepare(SkCanvas* canvas) override;

    void apply(SkCanvas* canvas) override;

    String getDescription() override;

private:
    Paint _paint;
    Path _path;
    Rect _rect;
};

} // namespace snap::drawing
