//
//  DrawingContext.hpp
//  valdi-skia-app
//
//  Created by Simon Corsin on 6/28/20.
//

#pragma once

#include "utils/base/NonCopyable.hpp"

#include "valdi_core/cpp/Interfaces/IBitmap.hpp"

#include "snap_drawing/cpp/Drawing/LayerContent.hpp"
#include "snap_drawing/cpp/Drawing/Paint.hpp"

#include "snap_drawing/cpp/Utils/Image.hpp"
#include "snap_drawing/cpp/Utils/Matrix.hpp"
#include "snap_drawing/cpp/Utils/Path.hpp"

#include "include/core/SkCanvas.h"
#include "include/core/SkPictureRecorder.h"

#include <optional>

namespace snap::drawing {

class BorderRadius;
class LazyPath;
class ExternalSurface;

class DrawingContext : public snap::NonCopyable {
public:
    DrawingContext(Scalar width, Scalar height);
    ~DrawingContext();

    /**
     Complete the rendering. This must be called after prepare() has been called.
     */
    LayerContent finish();

    /**
     Draw the paint to the context with the given border radius.
     */
    void drawPaint(const Paint& paint, const BorderRadius& borderRadius, LazyPath& lazyPath);

    /**
     Draw the paint to the context with the given border radius at the given targetRect.
     */
    void drawPaint(const Paint& paint, const BorderRadius& borderRadius, const Rect& targetRect, LazyPath& lazyPath);

    /**
     Draw the paint as a rect to the context at the given targetRect.
     */
    void drawPaint(const Paint& paint, const Rect& targetRect);

    /**
     Draw the paint with the given path
     */
    void drawPaint(const Paint& paint, const Path& path);

    /**
     Draw the bitmap into the current draw bounds, scaling the bitmap with the given FittingSizeMode
     */
    void drawBitmap(const Valdi::Ref<Valdi::IBitmap>& bitmap, FittingSizeMode fittingSizeMode);

    /**
     Draw the image into the given target rect
     */
    void drawImage(const Image& image, const Rect& imageRect, const Rect& targetRect, const Paint* paint);

    void drawExternalSurface(const Ref<ExternalSurface>& externalSurface);

    void clipPath(const Path& path);

    void clipRect(const Rect& rect);

    void concat(const Matrix& matrix);

    int save();

    void restore(int count);

    const Rect& drawBounds() const;

    SkCanvas* canvas();

private:
    Rect _drawBounds;
    SkPictureRecorder _recorder;
    Ref<ExternalSurface> _externalSurface;
    SkCanvas* _canvas = nullptr;
};

} // namespace snap::drawing
