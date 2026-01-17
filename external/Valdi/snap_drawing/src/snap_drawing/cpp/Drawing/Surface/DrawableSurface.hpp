//
//  DrawableSurface.hpp
//  snap_drawing-macos
//
//  Created by Simon Corsin on 2/7/22.
//

#pragma once

#include "snap_drawing/cpp/Drawing/GraphicsContext/GraphicsContext.hpp"
#include "snap_drawing/cpp/Drawing/Surface/DrawableSurfaceCanvas.hpp"
#include "snap_drawing/cpp/Drawing/Surface/Surface.hpp"
#include "snap_drawing/cpp/Utils/Aliases.hpp"

#include "valdi_core/cpp/Utils/Result.hpp"

namespace snap::drawing {

class DrawableSurface : public Surface {
public:
    virtual GraphicsContext* getGraphicsContext() = 0;

    virtual Valdi::Result<DrawableSurfaceCanvas> prepareCanvas() = 0;
    virtual void flush() = 0;
};

} // namespace snap::drawing
