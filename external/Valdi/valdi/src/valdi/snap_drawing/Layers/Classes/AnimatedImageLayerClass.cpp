//
//  AnimatedImageClass.cpp
//  valdi-desktop-apple
//
//  Created by Simon Corsin on 7/15/22.
//

#include "valdi/snap_drawing/Layers/Classes/AnimatedImageLayerClass.hpp"
#include "valdi/runtime/Context/ViewNode.hpp"
#include "valdi/snap_drawing/Utils/AttributesBinderUtils.hpp"
#include "valdi/snap_drawing/Utils/ValdiUtils.hpp"
#include "valdi_core/cpp/Utils/LoggerUtils.hpp"

namespace snap::drawing {

AnimatedImageLayerListenerImpl::AnimatedImageLayerListenerImpl() = default;
AnimatedImageLayerListenerImpl::~AnimatedImageLayerListenerImpl() = default;

void AnimatedImageLayerListenerImpl::setCallback(const Valdi::Ref<Valdi::ValueFunction>& callback) {
    _callback = callback;
}

void AnimatedImageLayerListenerImpl::onProgress(AnimatedImageLayer& animatedImageLayer,
                                                const Duration& time,
                                                const Duration& duration) {
    auto viewNode = valdiViewNodeFromLayer(animatedImageLayer);
    if (viewNode != nullptr) {
        viewNode->valueChanged(viewNode->getAttributeIds().getIdForName("currentTime"), Valdi::Value(time.seconds()));
    }

    if (_callback != nullptr) {
        auto param = Valdi::Value()
                         .setMapValue("time", Valdi::Value(time.seconds()))
                         .setMapValue("duration", Valdi::Value(duration.seconds()));

        (*_callback)(Valdi::ValueFunctionFlagsAllowThrottling, {std::move(param)});
    }
}

void AnimatedImageLayerListenerImpl::setCallbackOnLayer(AnimatedImageLayer& animatedImageLayer,
                                                        const Valdi::Ref<Valdi::ValueFunction>& callback) {
    auto listener = Valdi::castOrNull<AnimatedImageLayerListenerImpl>(animatedImageLayer.getListener());

    if (listener == nullptr) {
        listener = Valdi::makeShared<AnimatedImageLayerListenerImpl>();
        animatedImageLayer.setListener(listener);
    }

    listener->setCallback(callback);
}

AnimatedImageLayerClass::AnimatedImageLayerClass(const Ref<Resources>& resources, const Ref<LayerClass>& parentClass)
    : ILayerClass(
          resources, "SCValdiAnimatedContentView", "com.snap.valdi.views.AnimatedImageView", parentClass, false) {}
AnimatedImageLayerClass::~AnimatedImageLayerClass() = default;

Valdi::Ref<Layer> AnimatedImageLayerClass::instantiate() {
    return makeLayer<AnimatedImageLayer>(getResources());
}

void AnimatedImageLayerClass::bindAttributes(Valdi::AttributesBindingContext& binder) {
    binder.bindAssetAttributes(snap::valdi_core::AssetOutputType::Lottie);
    BIND_DOUBLE_ATTRIBUTE(AnimatedImageLayer, advanceRate, false);
    BIND_BOOLEAN_ATTRIBUTE(AnimatedImageLayer, loop, false);
    BIND_UNTYPED_ATTRIBUTE(AnimatedImageLayer, onProgress, false);
    BIND_DOUBLE_ATTRIBUTE(AnimatedImageLayer, currentTime, false);
    BIND_DOUBLE_ATTRIBUTE(AnimatedImageLayer, animationStartTime, false);
    BIND_DOUBLE_ATTRIBUTE(AnimatedImageLayer, animationEndTime, false);

    BIND_STRING_ATTRIBUTE(AnimatedImageLayer, objectFit, false);
}

IMPLEMENT_BOOLEAN_ATTRIBUTE(
    AnimatedImageLayer,
    loop,
    {
        view.setShouldLoop(value);
        return Valdi::Void();
    },
    { view.setShouldLoop(false); })

IMPLEMENT_DOUBLE_ATTRIBUTE(
    AnimatedImageLayer,
    advanceRate,
    {
        view.setAdvanceRate(value);
        return Valdi::Void();
    },
    { view.setAdvanceRate(0.0); })

IMPLEMENT_DOUBLE_ATTRIBUTE(
    AnimatedImageLayer,
    currentTime,
    {
        view.setCurrentTime(Duration::fromSeconds(value));
        return Valdi::Void();
    },
    { view.setCurrentTime(Duration()); })

IMPLEMENT_UNTYPED_ATTRIBUTE(
    AnimatedImageLayer,
    onProgress,
    {
        AnimatedImageLayerListenerImpl::setCallbackOnLayer(view, value.getFunctionRef());
        return Valdi::Void();
    },
    { AnimatedImageLayerListenerImpl::setCallbackOnLayer(view, nullptr); })

IMPLEMENT_DOUBLE_ATTRIBUTE(
    AnimatedImageLayer,
    animationStartTime,
    {
        view.setAnimationStartTime(Duration::fromSeconds(value));
        return Valdi::Void();
    },
    { view.setAnimationStartTime(Duration()); })

IMPLEMENT_DOUBLE_ATTRIBUTE(
    AnimatedImageLayer,
    animationEndTime,
    {
        view.setAnimationEndTime(Duration::fromSeconds(value));
        return Valdi::Void();
    },
    { view.setAnimationEndTime(Duration()); })

IMPLEMENT_STRING_ATTRIBUTE(
    AnimatedImageLayer,
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

} // namespace snap::drawing
