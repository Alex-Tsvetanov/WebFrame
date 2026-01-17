//
//  IViewManager.hpp
//  ValdiRuntime
//
//  Created by Simon Corsin on 5/24/18.
//  Copyright © 2018 Snap Inc. All rights reserved.
//

#pragma once

#include "valdi/runtime/Attributes/Animator.hpp"
#include "valdi/runtime/Context/RenderingBackendType.hpp"
#include "valdi/runtime/Context/ViewNode.hpp"
#include "valdi/runtime/Context/ViewNodeTree.hpp"
#include "valdi_core/AnimationType.hpp"
#include "valdi_core/cpp/Context/PlatformType.hpp"
#include "valdi_core/cpp/Utils/Bytes.hpp"
#include "valdi_core/cpp/Utils/Value.hpp"

#include <string>

namespace Valdi {

class BoundAttributes;
class AttributesBindingContext;
class IViewTransaction;
class IBitmapFactory;

/**
 * Abstraction over view creation, moving views to a different parent view and action calling.
 */
class IViewManager {
public:
    IViewManager() = default;
    virtual ~IViewManager() = default;

    virtual PlatformType getPlatformType() const = 0;
    virtual RenderingBackendType getRenderingBackendType() const = 0;

    virtual StringBox getDefaultViewClass() = 0;

    virtual Ref<ViewFactory> createViewFactory(const StringBox& className,
                                               const Ref<BoundAttributes>& boundAttributes) = 0;

    /**
      Create a ViewFactory that can create views compatible with this IViewManager instance, from a ViewFactory
      created from a different IViewManager instnace.
     */
    virtual Ref<ViewFactory> bridgeViewFactory(const Ref<ViewFactory>& /*viewFactory*/,
                                               const Ref<BoundAttributes>& /*startingAttributes*/) {
        return nullptr;
    };

    virtual void callAction(ViewNodeTree* viewNodeTree,
                            const StringBox& actionName,
                            const Ref<ValueArray>& parameters) = 0;

    virtual NativeAnimator createAnimator(snap::valdi_core::AnimationType type,
                                          const std::vector<double>& controlPoints,
                                          double duration,
                                          bool beginFromCurrentState,
                                          bool crossfade,
                                          double stiffness,
                                          double damping) = 0;
    /**
     Get the class hierarchy where the className is the first element and each subsequent element is the parent
     class of the previous element.
     */
    virtual std::vector<StringBox> getClassHierarchy(const StringBox& className) = 0;

    virtual void bindAttributes(const StringBox& className, AttributesBindingContext& binder) = 0;

    /**
     * Returns the ratio between a point and pixel.
     */
    virtual float getPointScale() const {
        return 1.0f;
    }

    /**
     * Return a wrapper object referencing the native ViewNode instance
     */
    virtual Value createViewNodeWrapper(const Ref<ViewNode>& viewNode, bool wrapInPlatformReference) = 0;

    /**
      Whether whether this IViewManager supports the given view class natively without
      having to use a bridge or a fallback.
     */
    virtual bool supportsClassNameNatively(const StringBox& /*className*/) {
        return true;
    }

    /**
      Start a new view transaction, which will enable the runtime to interact with a view hierarcy of the type
      managed by this IViewManager instance. If 'shouldDefer' is true, the view transaction will
      be expected to be deferred if the current thread is not the main thread.
     */
    virtual Ref<IViewTransaction> createViewTransaction(const Ref<MainThreadManager>& mainThreadManager,
                                                        bool shouldDefer) = 0;

    /**
    Returns a BitmapFactory that can provide bitmaps suitable for rastering
     views managed by this IViewManager instance.
     */
    virtual Ref<IBitmapFactory> getViewRasterBitmapFactory() const = 0;

    /**
      A/B TEST ONLY: This function is for an A/B test to disable the animation remove on complete for iOS.
      Should be removed once the A/B test is complete, by v13.21.
     */
    virtual void setDisableAnimationRemoveOnCompleteIos(bool disable) {};
};

} // namespace Valdi
