//
//  SCValdiSurfacePresenterView.h
//  valdi-desktop-apple
//
//  Created by Simon Corsin on 2/14/22.
//

#import <UIKit/UIKit.h>

#import <QuartzCore/CAMetalLayer.h>

@class SCValdiSurfacePresenterView;

typedef void (^SCValdiSurfacePresenterViewResizeBlock)();

@protocol SCValdiSurfacePresenterViewDelegate <NSObject>

- (void)surfacePresenterView:(SCValdiSurfacePresenterView*)surfaceView
    willResizeDrawableWithBlock:(SCValdiSurfacePresenterViewResizeBlock)block;

@end

@interface SCValdiSurfacePresenterView : UIView <CALayerDelegate>

@property (weak, nonatomic) id<SCValdiSurfacePresenterViewDelegate> delegate;

@property (readonly, nonatomic) CGPathRef clipPath;
@property (readonly, nonatomic) CGRect viewFrame;
@property (readonly, nonatomic) CATransform3D viewTransform;
@property (strong, nonatomic) UIView* embeddedView;

+ (instancetype)presenterViewWithMetalLayer:(inout CAMetalLayer**)outMetalLayer
                              contentsScale:(CGFloat)contentsScale API_AVAILABLE(ios(13.0));
+ (instancetype)presenterViewWithEmbeddedView:(UIView*)embeddedView;

- (void)setEmbeddedViewFrame:(CGRect)frame
                   transform:(CATransform3D)transform
                     opacity:(CGFloat)opacity
                    clipPath:(CGPathRef)clipPath
              clipHasChanged:(BOOL)clipHasChanged;

@end
