//
//  SCValdiView.m
//  Valdi
//
//  Created by Simon Corsin on 12/11/17.
//  Copyright © 2017 Snap Inc. All rights reserved.
//

#import "valdi_core/SCValdiView.h"
#import "valdi_core/SCValdiAttributesBinderBase.h"
#import "valdi_core/SCValdiFunction.h"

@implementation SCValdiView

- (BOOL)willEnqueueIntoValdiPool
{
    return self.class == [SCValdiView class];
}

- (CGSize)sizeThatFits:(CGSize)size
{
    return CGSizeMake(0, 0);
}

- (CGPoint)convertPoint:(CGPoint)point fromView:(UIView *)view
{
    return [self valdi_convertPoint:point fromView:view];
}

- (CGPoint)convertPoint:(CGPoint)point toView:(UIView *)view
{
    return [self valdi_convertPoint:point toView:view];
}

- (BOOL)requiresLayoutWhenAnimatingBounds
{
    return NO;
}

- (UIView *)hitTest:(CGPoint)point withEvent:(UIEvent *)event
{
    if (self.valdiHitTest != nil) {
        return [self valdi_hitTest:point withEvent:event withCustomHitTest:self.valdiHitTest];
    }
    return [super hitTest:point withEvent:event];
}

@end
