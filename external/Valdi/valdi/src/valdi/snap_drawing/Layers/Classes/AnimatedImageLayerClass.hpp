//
//  AnimatedImageLayerClass.hpp
//  valdi-desktop-apple
//
//  Created by Simon Corsin on 7/15/22.
//

#pragma once

#include "valdi/snap_drawing/Layers/Classes/LayerClass.hpp"
#include "valdi/snap_drawing/Layers/Interfaces/ILayerClass.hpp"

#include "snap_drawing/cpp/Layers/AnimatedImageLayer.hpp"

namespace snap::drawing {

class AnimatedImageLayerListenerImpl : public AnimatedImageLayerListener {
public:
    AnimatedImageLayerListenerImpl();
    ~AnimatedImageLayerListenerImpl() override;

    void setCallback(const Valdi::Ref<Valdi::ValueFunction>& callback);

    void onProgress(AnimatedImageLayer& animatedImageLayer, const Duration& time, const Duration& duration) override;

    static void setCallbackOnLayer(AnimatedImageLayer& animatedImageLayer,
                                   const Valdi::Ref<Valdi::ValueFunction>& callback);

private:
    Valdi::Ref<Valdi::ValueFunction> _callback;
};

class AnimatedImageLayerClass : public ILayerClass {
public:
    explicit AnimatedImageLayerClass(const Ref<Resources>& resources, const Ref<LayerClass>& parentClass);
    ~AnimatedImageLayerClass() override;

    Valdi::Ref<Layer> instantiate() override;
    void bindAttributes(Valdi::AttributesBindingContext& binder) override;

    DECLARE_BOOLEAN_ATTRIBUTE(AnimatedImageLayer, loop)
    DECLARE_DOUBLE_ATTRIBUTE(AnimatedImageLayer, advanceRate)
    DECLARE_DOUBLE_ATTRIBUTE(AnimatedImageLayer, currentTime)
    DECLARE_UNTYPED_ATTRIBUTE(AnimatedImageLayer, onProgress)
    DECLARE_DOUBLE_ATTRIBUTE(AnimatedImageLayer, animationStartTime)
    DECLARE_DOUBLE_ATTRIBUTE(AnimatedImageLayer, animationEndTime)

    DECLARE_STRING_ATTRIBUTE(AnimatedImageLayer, objectFit)
};

} // namespace snap::drawing
