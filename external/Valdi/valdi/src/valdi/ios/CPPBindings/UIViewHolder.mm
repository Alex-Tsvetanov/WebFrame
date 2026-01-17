//
//  UIViewHolder.m
//  valdi-ios
//
//  Created by Simon Corsin on 4/26/21.
//

#import "valdi/ios/CPPBindings/UIViewHolder.h"
#import "valdi_core/SCValdiBitmap.h"

#import "valdi/ios/Animations/SCValdiAnimator.h"
#import "valdi/ios/SCValdiContext.h"
#import "valdi/ios/Utils/ContextUtils.h"
#import "valdi/ios/SCValdiViewNode+CPP.h"
#import "valdi/ios/SCValdiViewNode.h"

#import "valdi_core/SCValdiValueUtils.h"
#import "valdi_core/SCValdiViewAssetHandlingProtocol.h"
#import "valdi_core/SCValdiViewOwner.h"

#import "valdi/runtime/Attributes/Animator.hpp"
#import "valdi_core/cpp/Resources/LoadedAsset.hpp"

#import "valdi_core/SCValdiContentViewProviding.h"
#import "valdi_core/SCValdiRectUtils.h"
#import "valdi_core/SCValdiView.h"
#import "valdi_core/SCValdiViewComponent.h"
#import "valdi_core/UIView+ValdiObjects.h"

#import "valdi_core/SCNValdiCoreAnimator+Private.h"
#import "valdi_core/cpp/Utils/BitmapWithBuffer.hpp"
#import "valdi_core/cpp/Utils/ByteBuffer.hpp"
#import "valdi_core/cpp/Utils/TrackedLock.hpp"

namespace ValdiIOS {

UIViewHolder::UIViewHolder(SCValdiRef *viewRef): ObjCObject(viewRef) {}

UIViewHolder::~UIViewHolder() = default;

VALDI_CLASS_IMPL(UIViewHolder)

snap::valdi_core::Platform UIViewHolder::getPlatform() const {
    return snap::valdi_core::Platform::Ios;
}

SCValdiRef *UIViewHolder::getRef() const {
    return getValue();
}

UIView *UIViewHolder::getView() const {
    return [getRef() instance];
}

static void applyLayoutToView(UIView *view, const Valdi::Frame& frame) {
    SCValdiViewLayout calculatedLayout = SCValdiMakeViewLayout(frame.x, frame.y, frame.width, frame.height);

    CGRect currentBounds = view.bounds;
    BOOL sizeChanged = !CGSizeEqualToSize(currentBounds.size, calculatedLayout.size);

    currentBounds.size = calculatedLayout.size;

    view.center = calculatedLayout.center;
    view.bounds = currentBounds;

    if (sizeChanged) {
        [view.valdiViewNode didApplyLayoutWithAnimator:nil];
    }
}

Valdi::Result<Valdi::Void> UIViewHolder::rasterInto(const Valdi::Ref<Valdi::IBitmap>& bitmap,
                              const Valdi::Frame& frame,
                              const Valdi::Matrix& transform,
                              float rasterScaleX,
                              float rasterScaleY) {
    Valdi::Result<Valdi::Void> result = Valdi::Void();

    UIViewHolder::executeSyncInMainThread([&]() {
        UIView *view = getView();
        if (!view) {
            result = Valdi::Error("Invalid view");
            return;
        }

        if (view.superview == nil) {
            applyLayoutToView(view, frame);
        }

        [view layoutIfNeeded];

        auto bitmapInfo = bitmap->getInfo();
        auto *bytes = bitmap->lockBytes();

        if (bytes == nullptr) {
            result = Valdi::Error("Failed to load bitmap bytes");
            return;
        }

        CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
        CGContextRef bitmapContext = CGBitmapContextCreate(bytes, bitmapInfo.width, bitmapInfo.height, 8, bitmapInfo.rowBytes, colorSpace, CGBitmapInfoFromValdiBitmapInfoCpp(bitmapInfo));

        if (bitmapContext) {
            UIGraphicsPushContext(bitmapContext);

            CGContextClearRect(bitmapContext, CGRectMake(0, 0, bitmapInfo.width, bitmapInfo.height));

            CGAffineTransform affineTransform = CGAffineTransformIdentity;
            affineTransform = CGAffineTransformTranslate(affineTransform, 0, bitmapInfo.height);
            affineTransform = CGAffineTransformScale(affineTransform, 1.0, -1.0);
            CGContextConcatCTM(bitmapContext, affineTransform);

            affineTransform = CGAffineTransformMakeScale(rasterScaleX, rasterScaleY);

            if (!transform.isIdentity()) {
                affineTransform = CGAffineTransformConcat(CGAffineTransformMake(transform.values[0], transform.values[1], transform.values[2], transform.values[3], transform.values[4], transform.values[5]), affineTransform);
            }
            affineTransform = CGAffineTransformTranslate(affineTransform, frame.x, frame.y);
            CGContextConcatCTM(bitmapContext, affineTransform);

            [view.layer renderInContext:bitmapContext];
            UIGraphicsPopContext();
            CGContextRelease(bitmapContext);
        } else {
            result = Valdi::Error("Failed to create CGBitmapContext");
        }

        CGColorSpaceRelease(colorSpace);

        bitmap->unlockBytes();
    });

    return result;
}

Valdi::Ref<UIViewHolder> UIViewHolder::makeWithUIView(UIView *view) {
    return Valdi::makeShared<UIViewHolder>([[SCValdiRef alloc] initWithInstance:view strong:YES]);
}

UIView* UIViewHolder::uiViewFromRef(const Valdi::Ref<Valdi::View>& viewRef) {
    auto typedRef = Valdi::castOrNull<UIViewHolder>(viewRef);
    if (typedRef == nullptr) {
        return nil;
    }
    return typedRef->getView();
}

void UIViewHolder::executeSyncInMainThread(const Valdi::Function<void()> &cb) {
    if (NSThread.isMainThread) {
        cb();
    } else {
        Valdi::DropAllTrackedLocks dropAllTrackedLocks;
        dispatch_sync(dispatch_get_main_queue(), ^{
            cb();
        });
    }
}

}
