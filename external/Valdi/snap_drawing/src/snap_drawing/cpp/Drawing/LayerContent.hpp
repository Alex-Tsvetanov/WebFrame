//
//  LayerContent.hpp
//  snap_drawing
//
//  Created by Simon Corsin on 2/1/22.
//

#pragma once

#include "include/core/SkPicture.h"
#include "snap_drawing/cpp/Drawing/Surface/ExternalSurface.hpp"
#include "snap_drawing/cpp/Utils/Aliases.hpp"

namespace snap::drawing {

/**
 LayerContent is a wrapper around a recorded list of draw commands through SkPicture,
 or a ExternalSurface reference. The LayerContent might hold the picture representing the
 background and result of the onDraw() call, or the foreground picture.
 It is created by the DrawingContext.
 */
struct LayerContent {
    sk_sp<SkPicture> picture;
    Ref<ExternalSurfaceSnapshot> externalSurface;

    LayerContent(const sk_sp<SkPicture>& picture, const Ref<ExternalSurfaceSnapshot>& externalSurface);
    LayerContent();
    ~LayerContent();

    bool isEmpty() const;
    void clear();
};

} // namespace snap::drawing
