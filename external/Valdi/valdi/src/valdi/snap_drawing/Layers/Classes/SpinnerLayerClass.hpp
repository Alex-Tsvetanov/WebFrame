//
//  SpinnerLayerClass.hpp
//  valdi-desktop-apple
//
//  Created by Simon Corsin on 1/12/22.
//

#pragma once

#include "snap_drawing/cpp/Layers/SpinnerLayer.hpp"
#include "valdi/snap_drawing/Layers/Classes/LayerClass.hpp"
#include "valdi/snap_drawing/Layers/Interfaces/ILayerClass.hpp"

namespace snap::drawing {

class SpinnerLayerClass : public ILayerClass {
public:
    SpinnerLayerClass(const Ref<Resources>& resources, const Ref<LayerClass>& parentClass);
    ~SpinnerLayerClass() override;

    Valdi::Ref<Layer> instantiate() override;
    void bindAttributes(Valdi::AttributesBindingContext& binder) override;

    DECLARE_COLOR_ATTRIBUTE(SpinnerLayer, color)
};

} // namespace snap::drawing
