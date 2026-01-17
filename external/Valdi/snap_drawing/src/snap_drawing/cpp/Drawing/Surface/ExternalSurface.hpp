//
//  ExternalSurface.hpp
//  snap_drawing-macos
//
//  Created by Simon Corsin on 2/1/22.
//

#pragma once

#include "snap_drawing/cpp/Utils/Aliases.hpp"
#include "snap_drawing/cpp/Utils/Geometry.hpp"
#include "snap_drawing/cpp/Utils/Matrix.hpp"
#include "snap_drawing/cpp/Utils/Path.hpp"

#include "valdi_core/cpp/Utils/Result.hpp"

#include "snap_drawing/cpp/Drawing/Surface/Surface.hpp"

namespace Valdi {
class IBitmap;
class IBitmapFactory;
} // namespace Valdi

namespace snap::drawing {

struct ExternalSurfacePresenterState;

/**
An external surface is a surface that is not drawn directly by SnapDrawing.
It is typically used to represent embedded platform views like UIView on iOS
or View on Android.
 */
class ExternalSurface : public Surface {
public:
    ExternalSurface();
    ~ExternalSurface() override;

    virtual Ref<Valdi::IBitmapFactory> getRasterBitmapFactory() const;

    virtual Valdi::Result<Valdi::Void> rasterInto(const Ref<Valdi::IBitmap>& bitmap,
                                                  const Rect& frame,
                                                  const Matrix& transform,
                                                  float rasterScaleX,
                                                  float rasterScaleY);

    const Size& getRelativeSize() const;
    void setRelativeSize(const Size& relativeSize);

private:
    Size _relativeSize;
};

/**
An external surface snapshot holds an external surface, and is created every time
the ExternalLayer is re-drawn. The ExternalSurface is inherently mutated, whereas the
ExternalSurfaceSnapshot is not. A snapshot gets created whenever the ExternalSurface
has been mutated.
 */
class ExternalSurfaceSnapshot : public Valdi::SimpleRefCountable {
public:
    explicit ExternalSurfaceSnapshot(Ref<ExternalSurface> externalSurface);
    ~ExternalSurfaceSnapshot() override;

    const Ref<ExternalSurface>& getExternalSurface() const;

private:
    Ref<ExternalSurface> _externalSurface;
};

} // namespace snap::drawing
