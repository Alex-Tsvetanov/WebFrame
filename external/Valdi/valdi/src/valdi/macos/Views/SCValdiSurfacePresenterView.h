//
//  SCValdiSurfacePresenterView.h
//  valdi-desktop-apple
//
//  Created by Simon Corsin on 2/14/22.
//

#import <Cocoa/Cocoa.h>

#import <QuartzCore/CAMetalLayer.h>

@class SCValdiSurfacePresenterView;

typedef void (^SCValdiSurfacePresenterViewResizeBlock)();

@protocol SCValdiSurfacePresenterViewDelegate <NSObject>

- (void)surfacePresenterView:(SCValdiSurfacePresenterView*)surfaceView
    willResizeDrawableWithBlock:(SCValdiSurfacePresenterViewResizeBlock)block;

@end

@interface SCValdiSurfacePresenterView : NSView <CALayerDelegate>

@property (weak, nonatomic) id<SCValdiSurfacePresenterViewDelegate> delegate;

@property (readonly, nonatomic) CGPathRef clipPath;
@property (readonly, nonatomic) CGRect viewFrame;
@property (readonly, nonatomic) CATransform3D viewTransform;
@property (readonly, nonatomic) NSView* embeddedView;

+ (instancetype)presenterViewWithMetalLayer:(CAMetalLayer*)metalLayer;
+ (instancetype)presenterViewWithEmbeddedView:(NSView*)embeddedView;

- (void)setEmbeddedViewFrame:(CGRect)frame
                   transform:(CATransform3D)transform
                     opacity:(CGFloat)opacity
                    clipPath:(CGPathRef)clipPath
              clipHasChanged:(BOOL)clipHasChanged;

@end
