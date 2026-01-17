//
//  ImageLayerClass.hpp
//  valdi-desktop-apple
//
//  Created by Simon Corsin on 1/12/22.
//

#pragma once

#include "snap_drawing/cpp/Layers/ImageLayer.hpp"
#include "valdi/snap_drawing/Layers/Classes/LayerClass.hpp"
#include "valdi/snap_drawing/Layers/Interfaces/ILayerClass.hpp"

namespace snap::drawing {

class ImageLayerClass : public ILayerClass {
public:
    explicit ImageLayerClass(const Ref<Resources>& resources, const Ref<LayerClass>& parentClass);
    ~ImageLayerClass() override;

    Valdi::Ref<Layer> instantiate() override;
    void bindAttributes(Valdi::AttributesBindingContext& binder) override;

    DECLARE_COLOR_ATTRIBUTE(ImageLayer, tint)
    DECLARE_STRING_ATTRIBUTE(ImageLayer, objectFit)

    DECLARE_DOUBLE_ATTRIBUTE(ImageLayer, contentScaleX)
    DECLARE_DOUBLE_ATTRIBUTE(ImageLayer, contentScaleY)
    DECLARE_DOUBLE_ATTRIBUTE(ImageLayer, contentRotation)
};

} // namespace snap::drawing
