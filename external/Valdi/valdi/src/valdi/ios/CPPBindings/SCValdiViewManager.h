//
//  SCValdiViewManager.h
//  ValdiIOS
//
//  Created by Simon Corsin on 5/24/18.
//  Copyright © 2018 Snap Inc. All rights reserved.
//

#ifdef __cplusplus

#import "valdi/RuntimeMessageHandler.hpp"
#import "valdi/runtime/Attributes/AttributesManager.hpp"
#import "valdi/runtime/Interfaces/IViewManager.hpp"
#import "valdi/runtime/Runtime.hpp"
#import "valdi_core/SCValdiDebugMessageDisplayer.h"
#import "valdi_core/SCValdiExceptionReporter.h"
#import "valdi_core/cpp/Utils/FlatMap.hpp"
#import "valdi_core/cpp/Utils/Shared.hpp"

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

@protocol SCValdiFontManagerProtocol;

namespace ValdiIOS {

class ViewManager : public snap::valdi::RuntimeMessageHandler, public Valdi::IViewManager {
public:
    ViewManager(id<SCValdiFontManagerProtocol> fontManager);
    ~ViewManager() override;

    Valdi::Ref<Valdi::ViewFactory> createViewFactory(
        const Valdi::StringBox& className, const Valdi::Ref<Valdi::BoundAttributes>& boundAttributes) override;

    void callAction(Valdi::ViewNodeTree* viewNodeTree,
                    const Valdi::StringBox& actionName,
                    const Valdi::Ref<Valdi::ValueArray>& parameters) override;

    Valdi::PlatformType getPlatformType() const override;
    Valdi::RenderingBackendType getRenderingBackendType() const override;
    Valdi::StringBox getDefaultViewClass() override;

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

    void onUncaughtJsError(const int32_t errorCode,
                           const Valdi::StringBox& moduleName,
                           const std::string& errorMessage,
                           const std::string& stackTrace) override;

    void onJsCrash(const Valdi::StringBox& moduleName,
                   const std::string& errorMessage,
                   const std::string& stackTrace,
                   bool isANR) override;

    void onDebugMessage(int32_t level, const std::string& message) override;

    void setDebugMessageDisplayer(id<SCValdiDebugMessageDisplayer> debugMessageDisplayer);

    void setExceptionReporter(id<SCValdiExceptionReporter> exceptionReporter);
    Valdi::Value createViewNodeWrapper(const Valdi::Ref<Valdi::ViewNode>& viewNode,
                                       bool wrapInPlatformReference) override;

    Valdi::Ref<Valdi::IViewTransaction> createViewTransaction(
        const Valdi::Ref<Valdi::MainThreadManager>& mainThreadManager, bool shouldDefer) override;

    void setDisableAnimationRemoveOnCompleteIos(bool disable) override;

    Valdi::Ref<Valdi::IBitmapFactory> getViewRasterBitmapFactory() const override;

private:
    Valdi::FlatMap<Valdi::StringBox, Class> _classByName;
    id<SCValdiDebugMessageDisplayer> _debugMessageDisplayer = nullptr;
    id<SCValdiExceptionReporter> _exceptionReporter = nullptr;
    id<SCValdiFontManagerProtocol> _fontManager;
    float _pointScale;
    bool _disableAnimationRemoveOnComplete;

    void insertSubviewAtIndex(UIView* parentView, UIView* view, int index);

    Class viewClassFromClassName(const Valdi::StringBox& className);
};
} // namespace ValdiIOS

#endif
