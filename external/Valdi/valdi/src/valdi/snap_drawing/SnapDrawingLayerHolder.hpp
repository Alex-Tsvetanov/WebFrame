//
//  SnapDrawingLayerHolder.hpp
//  valdi-skia-app
//
//  Created by Simon Corsin on 4/28/2021.

#pragma once

#include "valdi/runtime/Views/View.hpp"

namespace snap::drawing {

class Layer;

class DrawableSurfaceCanvas;

class SnapDrawingLayerHolder : public Valdi::View {
public:
    SnapDrawingLayerHolder();
    ~SnapDrawingLayerHolder() override;

    VALDI_CLASS_HEADER(SnapDrawingLayerHolder)

    void setWeak(const Valdi::Ref<Layer>& view);

    void setStrong(const Valdi::Ref<Layer>& view);

    Valdi::Ref<Layer> get() const;

    snap::valdi_core::Platform getPlatform() const override;

private:
    Valdi::Ref<Layer> _strong;
    Valdi::Weak<Layer> _weak;
};

} // namespace snap::drawing
