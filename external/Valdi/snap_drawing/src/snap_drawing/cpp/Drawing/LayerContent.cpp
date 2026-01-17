//
//  LayerContent.cpp
//  snap_drawing
//
//  Created by Simon Corsin on 2/1/22.
//

#include "snap_drawing/cpp/Drawing/LayerContent.hpp"

namespace snap::drawing {

LayerContent::LayerContent(const sk_sp<SkPicture>& picture, const Ref<ExternalSurfaceSnapshot>& externalSurface)
    : picture(picture), externalSurface(externalSurface) {}
LayerContent::LayerContent() = default;
LayerContent::~LayerContent() = default;

bool LayerContent::isEmpty() const {
    return picture == nullptr && externalSurface == nullptr;
}

void LayerContent::clear() {
    picture = nullptr;
    externalSurface = nullptr;
}

} // namespace snap::drawing
