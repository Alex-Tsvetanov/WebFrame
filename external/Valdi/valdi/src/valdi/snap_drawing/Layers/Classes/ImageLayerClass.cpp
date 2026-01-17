//
//  ImageLayerClass.cpp
//  valdi-desktop-apple
//
//  Created by Simon Corsin on 1/12/22.
//

#include "valdi/snap_drawing/Layers/Classes/ImageLayerClass.hpp"
#include "valdi/snap_drawing/Utils/AttributesBinderUtils.hpp"

namespace snap::drawing {

ImageLayerClass::ImageLayerClass(const Ref<Resources>& resources, const Ref<LayerClass>& parentClass)
    : ILayerClass(resources, "SCValdiImageView", "com.snap.valdi.views.ValdiImageView", parentClass, false) {}

ImageLayerClass::~ImageLayerClass() = default;

Valdi::Ref<Layer> ImageLayerClass::instantiate() {
    return snap::drawing::makeLayer<snap::drawing::ImageLayer>(getResources());
}

void ImageLayerClass::bindAttributes(Valdi::AttributesBindingContext& binder) {
    binder.bindAssetAttributes(snap::valdi_core::AssetOutputType::ImageSnapDrawing);

    BIND_COLOR_ATTRIBUTE(ImageLayer, tint, false);

    BIND_STRING_ATTRIBUTE(ImageLayer, objectFit, false);

    BIND_DOUBLE_ATTRIBUTE(ImageLayer, contentScaleX, false);
    BIND_DOUBLE_ATTRIBUTE(ImageLayer, contentScaleY, false);
    BIND_DOUBLE_ATTRIBUTE(ImageLayer, contentRotation, false);
}

IMPLEMENT_COLOR_ATTRIBUTE(
    ImageLayer,
    tint,
    {
        view.setTintColor(snapDrawingColorFromValdiColor(value));
        return Valdi::Void();
    },
    { view.setTintColor(Color::transparent()); })

IMPLEMENT_STRING_ATTRIBUTE(
    ImageLayer,
    objectFit,
    {
        if (value == "none") {
            view.setFittingSizeMode(FittingSizeModeCenter);
        } else if (value == "fill") {
            view.setFittingSizeMode(FittingSizeModeFill);
        } else if (value == "cover") {
            view.setFittingSizeMode(FittingSizeModeCenterScaleFill);
        } else if (value == "contain") {
            view.setFittingSizeMode(FittingSizeModeCenterScaleFit);
        }
        return Valdi::Void();
    },
    { view.setFittingSizeMode(FittingSizeModeFill); })

IMPLEMENT_DOUBLE_ATTRIBUTE(
    ImageLayer,
    contentScaleX,
    {
        view.setContentScaleX(static_cast<Scalar>(value));
        return Valdi::Void();
    },
    { view.setContentScaleX(1.0f); })

IMPLEMENT_DOUBLE_ATTRIBUTE(
    ImageLayer,
    contentScaleY,
    {
        view.setContentScaleY(static_cast<Scalar>(value));
        return Valdi::Void();
    },
    { view.setContentScaleY(1.0f); })

IMPLEMENT_DOUBLE_ATTRIBUTE(
    ImageLayer,
    contentRotation,
    {
        view.setContentRotation(static_cast<Scalar>(value));
        return Valdi::Void();
    },
    { view.setContentRotation(0.0f); })

} // namespace snap::drawing
