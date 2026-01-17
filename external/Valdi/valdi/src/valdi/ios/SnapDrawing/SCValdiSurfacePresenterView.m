//
//  SCValdiSurfacePresenterView.m
//  valdi-desktop-apple
//
//  Created by Simon Corsin on 2/14/22.
//

#import "SCValdiSurfacePresenterView.h"
#import <QuartzCore/CAShapeLayer.h>

API_AVAILABLE(ios(13.0))
@interface SCValdiMetalSurfaceView: SCValdiSurfacePresenterView
@end

@implementation SCValdiMetalSurfaceView

+ (Class)layerClass
{
    return [CAMetalLayer class];
}

- (CAMetalLayer *)metalLayer
{
    return (CAMetalLayer *)self.layer;
}

- (void)layoutSubviews
{
    CGRect bounds = self.bounds;
    CAMetalLayer *metalLayer = [self metalLayer];
    CGFloat contentsScale = metalLayer.contentsScale;
    CGSize drawableSize = CGSizeMake(bounds.size.width * contentsScale, bounds.size.height * contentsScale);
    if (!CGSizeEqualToSize(metalLayer.drawableSize, drawableSize)) {
        [self.delegate surfacePresenterView:self willResizeDrawableWithBlock:^{
            metalLayer.drawableSize = drawableSize;
        }];
    }
}

@end

@interface SCValdiEmbeddedSurfaceView: SCValdiSurfacePresenterView
@end

@implementation SCValdiEmbeddedSurfaceView

- (void)layoutSubviews
{
    UIView *embeddedView = self.embeddedView;
    embeddedView.layer.transform = self.viewTransform;
    embeddedView.frame = self.viewFrame;
}

- (CGSize)sizeThatFits:(CGSize)size
{
    return [self.embeddedView sizeThatFits:size];
}

@end

@implementation SCValdiSurfacePresenterView {
    CAShapeLayer *_clipLayer;
}

- (instancetype)init
{
    self = [super init];

    if (self) {
        self.translatesAutoresizingMaskIntoConstraints = NO;
    }

    return self;
}

- (BOOL)isFlipped
{
    return YES;
}

- (UIView *)hitTest:(CGPoint)point withEvent:(UIEvent *)event
{
    UIView *view = [super hitTest:point withEvent:event];
    if (view == self) {
        return nil;
    }

    return view;
}

- (void)setEmbeddedViewFrame:(CGRect)frame transform:(CATransform3D)transform opacity:(CGFloat)opacity clipPath:(CGPathRef)clipPath clipHasChanged:(BOOL)clipHasChanged
{
    _viewFrame = frame;
    _viewTransform = transform;

    _embeddedView.layer.transform = transform;
    _embeddedView.frame = frame;
    _embeddedView.alpha = opacity;

    if (clipHasChanged) {
        if (clipPath) {
            if (!_clipLayer) {
                _clipLayer = [CAShapeLayer layer];
            }

            _clipLayer.path = clipPath;

            if (!self.layer.mask) {
                self.layer.mask = _clipLayer;
            }
        } else {
            _clipLayer.path = nil;
            self.layer.mask = nil;
        }
    }
}

+ (instancetype)presenterViewWithMetalLayer:(inout CAMetalLayer **)outMetalLayer contentsScale:(CGFloat)contentsScale
{
    SCValdiMetalSurfaceView *surfaceView = [SCValdiMetalSurfaceView new];
    CAMetalLayer *metalLayer = [surfaceView metalLayer];
    metalLayer.contentsScale = contentsScale;
    *outMetalLayer = metalLayer;
    metalLayer.delegate = surfaceView;

    return surfaceView;
}

+ (instancetype)presenterViewWithEmbeddedView:(UIView *)embeddedView
{
    SCValdiEmbeddedSurfaceView *surfaceView = [SCValdiEmbeddedSurfaceView new];
    surfaceView.embeddedView = embeddedView;

    [surfaceView addSubview:embeddedView];

    return surfaceView;
}

@end
