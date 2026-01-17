//
//  ExternalLayer.hpp
//  snap_drawing-macos
//
//  Created by Simon Corsin on 2/1/22.
//

#pragma once

#include "snap_drawing/cpp/Drawing/Surface/ExternalSurface.hpp"
#include "snap_drawing/cpp/Layers/Layer.hpp"
#include "valdi_core/cpp/Utils/Result.hpp"

namespace snap::drawing {

/**
 An ExternalLayer is a special Layer that draws an ExternalSurface instance.
 The ExternalSurface represents an external rendering source that SnapDrawing
 does not directly manage.
 */
class ExternalLayer : public Layer {
public:
    explicit ExternalLayer(const Ref<Resources>& resources);
    ~ExternalLayer() override;

    void setExternalSurface(const Ref<ExternalSurface>& externalSurface);
    const Ref<ExternalSurface>& getExternalSurface() const;

    bool shouldRasterizeExternalSurface() const;

protected:
    void onDraw(DrawingContext& drawingContext) override;

private:
    Ref<ExternalSurface> _externalSurface;

    Valdi::Result<Ref<Image>> rasterExternalSurface(int width, int height) const;
};

} // namespace snap::drawing
