//
//  SnapDrawingViewManager.hpp
//  valdi-skia-app
//
//  Created by Simon Corsin on 6/28/20.

#pragma once

#include "valdi/runtime/Interfaces/IViewManager.hpp"

#include "valdi/snap_drawing/Layers/Interfaces/ILayerClass.hpp"

#include "valdi_core/cpp/Interfaces/ILogger.hpp"
#include "valdi_core/cpp/Utils/Bytes.hpp"
#include "valdi_core/cpp/Utils/FlatMap.hpp"
#include "valdi_core/cpp/Utils/Shared.hpp"

namespace snap::drawing {

class ImageLoader;
class FontManager;
class DrawLooper;

using ActionCaller =
    Valdi::Function<void(Valdi::ViewNodeTree*, const std::string&, const Valdi::Ref<Valdi::ValueArray>&)>;

class SnapDrawingViewManager : public Valdi::SimpleRefCountable, public Valdi::IViewManager {
public:
    SnapDrawingViewManager(const Ref<Resources>& resources, Valdi::PlatformType platformType);
    ~SnapDrawingViewManager() override;

    Valdi::PlatformType getPlatformType() const override;

    Valdi::RenderingBackendType getRenderingBackendType() const override;

    Valdi::StringBox getDefaultViewClass() override;

    Valdi::Ref<Valdi::ViewFactory> createViewFactory(
        const Valdi::StringBox& className, const Valdi::Ref<Valdi::BoundAttributes>& boundAttributes) override;

    Valdi::Ref<Valdi::ViewFactory> bridgeViewFactory(
        const Valdi::Ref<Valdi::ViewFactory>& viewFactory,
        const Valdi::Ref<Valdi::BoundAttributes>& startingAttributes) override;

    void callAction(Valdi::ViewNodeTree* viewNodeTree,
                    const Valdi::StringBox& actionName,
                    const Valdi::Ref<Valdi::ValueArray>& parameters) override;

    Valdi::NativeAnimator createAnimator(snap::valdi_core::AnimationType type,
                                         const std::vector<double>& controlPoints,
                                         double duration,
                                         bool beginFromCurrentState,
                                         bool crossfade,
                                         double stiffness,
                                         double damping) override;

    std::vector<Valdi::StringBox> getClassHierarchy(const Valdi::StringBox& className) override;

    void bindAttributes(const Valdi::StringBox& className, Valdi::AttributesBindingContext& binder) override;

    float getPointScale() const override;

    void registerLayerClass(const Valdi::Ref<ILayerClass>& layerClass);
    Valdi::Ref<ILayerClass> getLayerClass(const Valdi::StringBox& className) const;
    Valdi::Ref<ILayerClass> getBaseLayerClass() const;

    void registerTypeface(const Valdi::StringBox& fontFamily,
                          FontStyle fontStyle,
                          bool canUseAsFallback,
                          const Valdi::BytesView& data);

    void setHostViewManager(Valdi::IViewManager* hostViewManager);

    const Ref<Resources>& getResources() const;

    Valdi::Value createViewNodeWrapper(const Valdi::Ref<Valdi::ViewNode>& viewNode,
                                       bool wrapInPlatformReference) override;

    Valdi::Ref<Valdi::IBitmapFactory> getViewRasterBitmapFactory() const override;

    Valdi::Ref<Valdi::IViewTransaction> createViewTransaction(
        const Valdi::Ref<Valdi::MainThreadManager>& mainThreadManager, bool shouldDefer) override;

private:
    [[maybe_unused]] Valdi::ILogger& _logger;
    Valdi::PlatformType _platformType;
    Valdi::FlatMap<Valdi::StringBox, Valdi::Ref<ILayerClass>> _layerClasses;
    Valdi::IViewManager* _hostViewManager = nullptr;
    Valdi::StringBox _baseLayerClassName;
    Ref<Resources> _resources;

    Valdi::StringBox getClassName(const ILayerClass& layerClass) const;
    void generateClassHierarchy(const ILayerClass& layerClass, std::vector<Valdi::StringBox>& out) const;

    bool shouldBridgeLayerClass(const Valdi::StringBox& layerClassName);
};

} // namespace snap::drawing
