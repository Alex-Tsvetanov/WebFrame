//
//  SCValdiMacOSViewManager.h
//  valdi-macos
//
//  Created by Simon Corsin on 10/13/20.
//

#import "valdi/runtime/Interfaces/IViewManager.hpp"
#import <Cocoa/Cocoa.h>
#import <Foundation/Foundation.h>

namespace ValdiMacOS {

NSView* fromValdiView(const Valdi::Ref<Valdi::View>& view);

class ViewManager : public Valdi::IViewManager {
public:
    ViewManager();
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

    bool supportsClassNameNatively(const Valdi::StringBox& className) override;

    Valdi::Value createViewNodeWrapper(const Valdi::Ref<Valdi::ViewNode>& viewNode,
                                       bool wrapInPlatformReference) override;

    Valdi::Ref<Valdi::IViewTransaction> createViewTransaction(
        const Valdi::Ref<Valdi::MainThreadManager>& mainThreadManager, bool shouldDefer) override;

    Valdi::Ref<Valdi::IBitmapFactory> getViewRasterBitmapFactory() const override;
};
} // namespace ValdiMacOS
