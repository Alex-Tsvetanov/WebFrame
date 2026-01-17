//
//  SpinnerLayerClass.cpp
//  valdi-desktop-apple
//
//  Created by Simon Corsin on 1/12/22.
//

#include "valdi/snap_drawing/Layers/Classes/SpinnerLayerClass.hpp"
#include "valdi/snap_drawing/Utils/AttributesBinderUtils.hpp"

namespace snap::drawing {

SpinnerLayerClass::SpinnerLayerClass(const Ref<Resources>& resources, const Ref<LayerClass>& parentClass)
    : ILayerClass(resources, "SCValdiSpinnerView", "com.snap.valdi.views.ValdiSpinnerView", parentClass, false) {}

SpinnerLayerClass::~SpinnerLayerClass() = default;

Valdi::Ref<Layer> SpinnerLayerClass::instantiate() {
    return snap::drawing::makeLayer<SpinnerLayer>(getResources());
}

void SpinnerLayerClass::bindAttributes(Valdi::AttributesBindingContext& binder) {
    BIND_COLOR_ATTRIBUTE(SpinnerLayer, color, false);
}

IMPLEMENT_COLOR_ATTRIBUTE(
    SpinnerLayer,
    color,
    {
        view.setColor(snapDrawingColorFromValdiColor(value));
        return Valdi::Void();
    },
    { view.setColor(Color::white()); })

} // namespace snap::drawing
