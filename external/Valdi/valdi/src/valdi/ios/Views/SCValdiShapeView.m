//
//  SCValdiShapeView.m
//  SCValdiUI
//
//  Created by Nathaniel Parrott on 6/6/19.
//

#import "valdi/ios/Views/SCValdiShapeView.h"

#import "valdi_core/SCValdiLogger.h"
#import "valdi/ios/SCValdiAttributesBinder.h"
#import "valdi/ios/Categories/UIView+Valdi.h"
#import "valdi_core/SCValdiGeometricPath.h"

@implementation SCValdiShapeView

- (instancetype)initWithFrame:(CGRect)frame
{
    self = [super initWithFrame:frame];

    if (self) {
        [self shapeLayer].fillColor = nil;
        [self shapeLayer].strokeColor = nil;
        [self valdi_setStrokeCap:nil];
        [self valdi_setStrokeJoin:nil];
    }

    return self;
}

#pragma mark - Valdi

- (void)valdi_applyPath:(id)pathData animator:(id<SCValdiAnimatorProtocol>)animator
{
    CGPathRef pathRef = CGPathFromGeometricPathData(pathData, self.bounds.size);

    if (animator) {
        [animator addAnimationOnLayer:[self shapeLayer]
                           forKeyPath:@"path"
                                value:(__bridge id)pathRef];
    } else {
        [self shapeLayer].path = pathRef;
    }
    
    if (pathRef) {
        CFRelease(pathRef);
    }
}

- (void)valdi_setPath:(id)pathData animator:(id<SCValdiAnimatorProtocol>)animator
{
    [self valdi_applyPath:pathData animator:animator];

    [self.valdiViewNode setDidFinishLayoutBlock:^(SCValdiShapeView *view, id<SCValdiAnimatorProtocol> animator) {
        [view valdi_applyPath:pathData animator:animator];
    } forKey:@"shape"];
}

- (void)valdi_setStrokeStart:(CGFloat)strokeStart animator:(id<SCValdiAnimatorProtocol>)animator
{
    if (animator) {
        [animator addAnimationOnLayer:[self shapeLayer]
                           forKeyPath:@"strokeStart"
                                value:@(strokeStart)];
    } else {
        [self shapeLayer].strokeStart = strokeStart;
    }
}

- (void)valdi_setStrokeEnd:(CGFloat)strokeEnd animator:(id<SCValdiAnimatorProtocol>)animator
{
    if (animator) {
        [animator addAnimationOnLayer:[self shapeLayer]
                           forKeyPath:@"strokeEnd"
                                value:@(strokeEnd)];
    } else {
        [self shapeLayer].strokeEnd = strokeEnd;
    }
}

+ (void)bindAttributes:(id<SCValdiAttributesBinderProtocol>)attributesBinder
{
    [attributesBinder bindAttribute:@"path"
        invalidateLayoutOnChange:NO
                   withUntypedBlock:^BOOL(SCValdiShapeView *view, id attributeValue, id<SCValdiAnimatorProtocol> animator) {
        [view valdi_setPath:attributeValue animator:animator];
        return YES;
    }
        resetBlock:^(SCValdiShapeView *view, id<SCValdiAnimatorProtocol> animator) {
        [view valdi_setPath:nil animator:animator];
        }];
    [attributesBinder bindAttribute:@"strokeCap"
        invalidateLayoutOnChange:NO
        withStringBlock:^BOOL(SCValdiShapeView *view, NSString *attributeValue, id<SCValdiAnimatorProtocol> animator) {
            return [view valdi_setStrokeCap:attributeValue];
        }
        resetBlock:^(SCValdiShapeView *view, id<SCValdiAnimatorProtocol> animator) {
            [view valdi_setStrokeCap:nil];
        }];
    [attributesBinder bindAttribute:@"strokeJoin"
        invalidateLayoutOnChange:NO
        withStringBlock:^BOOL(SCValdiShapeView *view, NSString *attributeValue, id<SCValdiAnimatorProtocol> animator) {
            return [view valdi_setStrokeJoin:attributeValue];
        }
        resetBlock:^(SCValdiShapeView *view, id<SCValdiAnimatorProtocol> animator) {
            [view valdi_setStrokeJoin:nil];
        }];
    [attributesBinder bindAttribute:@"fillColor"
        invalidateLayoutOnChange:NO
        withColorBlock:^BOOL(SCValdiShapeView *view, UIColor *attributeValue, id<SCValdiAnimatorProtocol> animator) {
            [view _setFillColor:attributeValue animator:animator];
            return YES;
        }
        resetBlock:^(SCValdiShapeView *view, id<SCValdiAnimatorProtocol> animator) {
            [view _setFillColor:nil animator:animator];
        }];
    [attributesBinder bindAttribute:@"strokeColor"
        invalidateLayoutOnChange:NO
        withColorBlock:^BOOL(SCValdiShapeView *view, UIColor *attributeValue, id<SCValdiAnimatorProtocol> animator) {
            [view _setStrokeColor:attributeValue animator:animator];
            return YES;
        }
        resetBlock:^(SCValdiShapeView *view, id<SCValdiAnimatorProtocol> animator) {
            [view _setStrokeColor:nil animator:animator];
        }];
    [attributesBinder bindAttribute:@"strokeWidth"
        invalidateLayoutOnChange:NO
        withDoubleBlock:^BOOL(SCValdiShapeView *view, CGFloat attributeValue, id<SCValdiAnimatorProtocol> animator) {
            [view _setLineWidth:attributeValue animator:animator];
            return YES;
        }
        resetBlock:^(SCValdiShapeView *view, id<SCValdiAnimatorProtocol> animator) {
            [view _setLineWidth:0 animator:animator];
        }];
    [attributesBinder bindAttribute:@"strokeStart"
        invalidateLayoutOnChange:NO
        withDoubleBlock:^BOOL(SCValdiShapeView *view, CGFloat attributeValue, id<SCValdiAnimatorProtocol> animator) {
            [view valdi_setStrokeStart:attributeValue animator:animator];
            return YES;
        }
        resetBlock:^(SCValdiShapeView *view, id<SCValdiAnimatorProtocol> animator) {
            [view valdi_setStrokeStart:0 animator:animator];
        }];
    [attributesBinder bindAttribute:@"strokeEnd"
        invalidateLayoutOnChange:NO
        withDoubleBlock:^BOOL(SCValdiShapeView *view, CGFloat attributeValue, id<SCValdiAnimatorProtocol> animator) {
            [view valdi_setStrokeEnd:attributeValue animator:animator];
            return YES;
        }
        resetBlock:^(SCValdiShapeView *view, id<SCValdiAnimatorProtocol> animator) {
            [view valdi_setStrokeEnd:1 animator:animator];
        }];
}

- (BOOL)willEnqueueIntoValdiPool
{
    return YES;
}

- (BOOL)requiresLayoutWhenAnimatingBounds
{
    return NO;
}

#pragma mark - Shape layer

- (CAShapeLayer *)shapeLayer
{
    return (CAShapeLayer *)self.layer;
}

+ (Class)layerClass
{
    return [CAShapeLayer class];
}

- (void)_setFillColor:(UIColor *)color animator:(id<SCValdiAnimatorProtocol> )animator
{
    if (animator) {
        [animator addAnimationOnLayer:self.shapeLayer forKeyPath:NSStringFromSelector(@selector(fillColor)) value:(__bridge id)color.CGColor];
    } else {
        self.shapeLayer.fillColor = color.CGColor;
    }
}

- (void)_setStrokeColor:(UIColor *)color animator:(id<SCValdiAnimatorProtocol> )animator
{
    if (animator) {
        [animator addAnimationOnLayer:self.shapeLayer forKeyPath:NSStringFromSelector(@selector(strokeColor)) value:(__bridge id)color.CGColor];
    } else {
        self.shapeLayer.strokeColor = color.CGColor;
    }
}

- (void)_setLineWidth:(CGFloat)lineWidth animator:(id<SCValdiAnimatorProtocol> )animator
{
    if (animator) {
        [animator addAnimationOnLayer:self.shapeLayer forKeyPath:NSStringFromSelector(@selector(lineWidth)) value:@(lineWidth)];
    } else {
        self.shapeLayer.lineWidth = lineWidth;
    }
}

- (BOOL)valdi_setStrokeCap:(NSString *)strokeCap
{
    CAShapeLayerLineCap lineCap = kCALineCapButt;
    if (strokeCap) {
        if ([strokeCap isEqualToString:@"butt"]) {
            lineCap = kCALineCapButt;
        } else if ([strokeCap isEqualToString:@"round"]) {
            lineCap = kCALineCapRound;
        } else if ([strokeCap isEqualToString:@"square"]) {
            lineCap = kCALineCapSquare;
        } else {
            return NO;
        }
    }
    [self shapeLayer].lineCap = lineCap;
    return YES;
}

- (BOOL)valdi_setStrokeJoin:(NSString *)strokeJoin
{
    CAShapeLayerLineJoin lineJoin = kCALineJoinMiter;
    if (strokeJoin) {
        if ([strokeJoin isEqualToString:@"bevel"]) {
            lineJoin = kCALineJoinBevel;
        } else if ([strokeJoin isEqualToString:@"miter"]) {
            lineJoin = kCALineJoinMiter;
        } else if ([strokeJoin isEqualToString:@"round"]) {
            lineJoin = kCALineJoinRound;
        } else {
            return NO;
        }
    }
    [self shapeLayer].lineJoin = lineJoin;
    return YES;
}

@end
