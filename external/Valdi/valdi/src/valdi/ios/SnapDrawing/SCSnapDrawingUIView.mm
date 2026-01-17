//
//  SCSnapDrawingUIView.m
//  valdi-ios
//
//  Created by Simon Corsin on 8/30/22.
//

#import "SCSnapDrawingUIView.h"
#import "SCSnapDrawingUIView+CPP.h"
#import "valdi_core/SCSnapDrawingRuntime.h"

#include "snap_drawing/cpp/Drawing/DrawLooper.hpp"

#include "snap_drawing/cpp/Drawing/Surface/SurfacePresenterManager.hpp"

#import "valdi/ios/SnapDrawing/SCValdiSnapDrawingSurfacePresenterManager.h"
#import "valdi/ios/SnapDrawing/SCValdiSurfacePresenterView.h"

static CGFloat resolveContentsScale() {
    // For some reasons, we don't see the contentScale being propagated properly
    // through the UIKit level.
    return [[UIScreen mainScreen] nativeScale];
}

@interface SCSnapDrawingUIView () <SCSnapDrawingMetalSurfacePresenterManagerDelegate, SCValdiSurfacePresenterViewDelegate>

@end

@implementation SCSnapDrawingUIView {
    id<SCSnapDrawingRuntime> _runtime;
}

- (instancetype)initWithRuntime:(id<SCSnapDrawingRuntime>)runtime
{
    self = [super init];

    if (self) {
        _runtime = runtime;

        auto cppRuntime = [self cppRuntime];

        _layerRoot = snap::drawing::makeLayer<snap::drawing::LayerRoot>(cppRuntime->getResources());

        auto graphicsContext = Valdi::castOrNull<snap::drawing::MetalGraphicsContext>(cppRuntime->getGraphicsContext());
        auto presenterManager = Valdi::makeShared<snap::drawing::IOSSurfacePresenterManager>(self, graphicsContext);

        cppRuntime->getDrawLooper()->addLayerRoot(_layerRoot, presenterManager, false);
    }

    return self;
}

- (void)dealloc
{
    auto *cppRuntime = [self cppRuntime];
    if (cppRuntime == nullptr) {
        return;
    }

    cppRuntime->getDrawLooper()->removeLayerRoot(_layerRoot);
}

- (void)layoutSubviews
{
    [super layoutSubviews];

    _layerRoot->setSize(snap::drawing::Size::make(self.bounds.size.width, self.bounds.size.height), resolveContentsScale());
    CGRect bounds = self.bounds;
    for (UIView *subview in self.subviews) {
        subview.frame = bounds;
        [subview layoutIfNeeded];
    }
}

- (snap::drawing::Runtime *)cppRuntime
{
    return Valdi::unsafeBridgeUnretained<snap::drawing::Runtime>([_runtime handle]);
}

- (id)createEmbeddedPresenterForView:(id)view
{
    return [SCValdiSurfacePresenterView presenterViewWithEmbeddedView:view];
}

- (id)createMetalPresenter:(inout CAMetalLayer *__autoreleasing *)metalLayer API_AVAILABLE(ios(13.0)) {
    SCValdiSurfacePresenterView *presenterView = [SCValdiSurfacePresenterView presenterViewWithMetalLayer:metalLayer contentsScale:resolveContentsScale()];
    presenterView.delegate = self;

    return presenterView;
}

- (void)removePresenter:(id)presenter
{
    [(UIView *)presenter removeFromSuperview];
}

- (void)setFrame:(CGRect)frame transform:(CATransform3D)transform opacity:(CGFloat)opacity clipPath:(CGPathRef)clipPath clipHasChanged:(BOOL)clipHasChanged forEmbeddedPresenter:(id)presenter
{
    [(SCValdiSurfacePresenterView *)presenter
     setEmbeddedViewFrame:frame
     transform:transform
     opacity:opacity
     clipPath:clipPath
     clipHasChanged:clipHasChanged];
}

- (void)setZIndex:(NSUInteger)zIndex forPresenter:(id)presenter
{
    SCValdiSurfacePresenterView *presenterView = presenter;

    [presenterView removeFromSuperview];

    [self insertSubview:presenterView atIndex:zIndex];

    presenterView.frame = self.bounds;
    [presenterView layoutIfNeeded];
}

- (void)surfacePresenterView:(SCValdiSurfacePresenterView *)surfaceView willResizeDrawableWithBlock:(SCValdiSurfacePresenterViewResizeBlock)block
{
    auto *cppRuntime = [self cppRuntime];
    if (cppRuntime == nullptr) {
        return;
    }
    auto drawLock = cppRuntime->getDrawLooper()->getDrawLock();
    block();
}

@end
