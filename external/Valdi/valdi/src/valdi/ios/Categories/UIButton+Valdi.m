//
//  UIButton+Valdi.m
//  Valdi
//
//  Created by Simon Corsin on 12/12/17.
//  Copyright © 2017 Snap Inc. All rights reserved.
//

#import "valdi/ios/Categories/UIButton+Valdi.h"

#import "valdi/ios/Text/NSAttributedString+Valdi.h"
#import "valdi/ios/Text/SCValdiFontAttributes.h"
#import "valdi/ios/Text/SCValdiFont.h"

#import "valdi_core/UIColor+Valdi.h"
#import "valdi_core/UIImage+Valdi.h"

@implementation UIButton (Valdi)

- (BOOL)valdi_setTitleColor:(UIColor *)attributeValue forState:(UIControlState)controlState
{
    [self setTitleColor:attributeValue forState:controlState];
    return YES;
}

- (BOOL)valdi_setTitleShadowColor:(UIColor *)attributeValue forState:(UIControlState)controlState
{
    [self setTitleShadowColor:attributeValue forState:controlState];
    return YES;
}

- (BOOL)valdi_setImage:(id)attributeValue forState:(UIControlState)controlState
{
    UIImage *image = [UIImage imageFromValdiAttributeValue:attributeValue context:self.valdiContext];

    [self setImage:image forState:controlState];
    return YES;
}

- (BOOL)valdi_setBackgroundImage:(id)attributeValue forState:(UIControlState)controlState
{
    UIImage *image = [UIImage imageFromValdiAttributeValue:attributeValue context:self.valdiContext];

    [self setBackgroundImage:image forState:controlState];
    return YES;
}

- (void)valdi_setTitle:(NSString *)title forState:(UIControlState)controlState
{
    [self setTitle:title forState:controlState];
}

+ (void)bindAttributes:(id<SCValdiAttributesBinderProtocol>)attributesBinder
{
    [attributesBinder bindCompositeAttribute:@"fontSpecs" parts:[NSAttributedString valdiFontAttributes] withUntypedBlock:^BOOL(UIButton *view, id attributeValue, id<SCValdiAnimatorProtocol> animator) {
        SCValdiFontAttributes *attributes = ObjectAs(attributeValue, SCValdiFontAttributes);
        if (!attributes) {
            return NO;
        }
        view.titleLabel.font = [attributes.font resolveFontFromTraitCollection:view.valdiContext.traitCollection];
        view.titleLabel.textColor = attributes.color;
        view.titleLabel.textAlignment = [attributes resolveTextAlignmentWithIsRightToLeft:view.valdiViewNode.isRightToLeft];

        return YES;
    } resetBlock:^(UIButton *view, id<SCValdiAnimatorProtocol> animator) {
        SCValdiFontAttributes *attributes = [NSAttributedString defaultFontAttributes];

        view.titleLabel.font = [attributes.font resolveFontFromTraitCollection:view.valdiContext.traitCollection];
        view.titleLabel.textColor = attributes.color;
        view.titleLabel.textAlignment = [attributes resolveTextAlignmentWithIsRightToLeft:view.valdiViewNode.isRightToLeft];
    }];

    [attributesBinder registerPreprocessorForAttribute:@"fontSpecs" enableCache:YES withBlock:^id(id value) {
        return [NSAttributedString fontAttributesWithCompositeValue:value];
    }];

    NSDictionary<NSString *, NSNumber *> *controlStateByString = @{
        @"normal" : @(UIControlStateNormal),
        @"disabled" : @(UIControlStateDisabled),
        @"selected" : @(UIControlStateSelected),
        @"highlighted" : @(UIControlStateHighlighted),
        @"focused" : @(UIControlStateFocused),
    };

    for (NSString *controlStateString in controlStateByString) {
        UIControlState controlState = (UIControlState)[controlStateByString[controlStateString] unsignedIntegerValue];

        [attributesBinder bindAttribute:[NSString stringWithFormat:@"%@:%@", controlStateString, @"title"]
            invalidateLayoutOnChange:YES
            withStringBlock:^BOOL(UIButton *view, NSString *attributeValue, id<SCValdiAnimatorProtocol> animator) {
                SCValdiAnimatorTransitionWrap(animator, view,
                                                 { [view valdi_setTitle:attributeValue forState:controlState]; });
                return YES;
            }
            resetBlock:^(UIButton *view, id<SCValdiAnimatorProtocol> animator) {
                SCValdiAnimatorTransitionWrap(animator, view,
                                                 { [view valdi_setTitle:nil forState:controlState]; });
            }];
        [attributesBinder bindAttribute:[NSString stringWithFormat:@"%@:%@", controlStateString, @"titleColor"]
            invalidateLayoutOnChange:NO
            withColorBlock:^BOOL(UIButton *view, UIColor *attributeValue, id<SCValdiAnimatorProtocol> animator) {
                SCValdiAnimatorTransitionWrap(
                    animator, view, { return [view valdi_setTitleColor:attributeValue forState:controlState]; });
                return YES;
            }
            resetBlock:^(UIButton *view, id<SCValdiAnimatorProtocol> animator) {
                SCValdiAnimatorTransitionWrap(animator, view,
                                                 { [view valdi_setTitleColor:nil forState:controlState]; });
            }];
        [attributesBinder bindAttribute:[NSString stringWithFormat:@"%@:%@", controlStateString, @"titleShadowColor"]
            invalidateLayoutOnChange:NO
            withColorBlock:^BOOL(UIButton *view, UIColor *attributeValue, id<SCValdiAnimatorProtocol> animator) {
                SCValdiAnimatorTransitionWrap(animator, view, {
                    return [view valdi_setTitleShadowColor:attributeValue forState:controlState];
                });
                return YES;
            }
            resetBlock:^(UIButton *view, id<SCValdiAnimatorProtocol> animator) {
                SCValdiAnimatorTransitionWrap(animator, view,
                                                 { [view valdi_setTitleShadowColor:nil forState:controlState]; });
            }];
        [attributesBinder bindAttribute:[NSString stringWithFormat:@"%@:%@", controlStateString, @"image"]
            invalidateLayoutOnChange:YES
            withUntypedBlock:^BOOL(UIButton *view, id attributeValue, id<SCValdiAnimatorProtocol> animator) {
                SCValdiAnimatorTransitionWrap(
                    animator, view, { return [view valdi_setImage:attributeValue forState:controlState]; });
                return YES;
            }
            resetBlock:^(UIButton *view, id<SCValdiAnimatorProtocol> animator) {
                SCValdiAnimatorTransitionWrap(animator, view,
                                                 { [view valdi_setImage:nil forState:controlState]; });
            }];
        [attributesBinder bindAttribute:[NSString stringWithFormat:@"%@:%@", controlStateString, @"backgroundImage"]
            invalidateLayoutOnChange:NO
            withUntypedBlock:^BOOL(UIButton *view, id attributeValue, id<SCValdiAnimatorProtocol> animator) {
                SCValdiAnimatorTransitionWrap(animator, view, {
                    return [view valdi_setBackgroundImage:attributeValue forState:controlState];
                });
                return YES;
            }
            resetBlock:^(UIButton *view, id<SCValdiAnimatorProtocol> animator) {
                SCValdiAnimatorTransitionWrap(animator, view,
                                                 { [view valdi_setBackgroundImage:nil forState:controlState]; });
            }];
    }

    [attributesBinder bindAttribute:@"value"
        invalidateLayoutOnChange:YES
        withStringBlock:^BOOL(UIButton *view, NSString *attributeValue, id<SCValdiAnimatorProtocol> animator) {
            SCValdiAnimatorTransitionWrap(
                animator, view, { [view valdi_setTitle:attributeValue forState:UIControlStateNormal]; });
            return YES;
        }
        resetBlock:^(UIButton *view, id<SCValdiAnimatorProtocol> animator) {
            SCValdiAnimatorTransitionWrap(animator, view,
                                             { [view valdi_setTitle:nil forState:UIControlStateNormal]; });
        }];
    [attributesBinder bindAttribute:@"color"
        invalidateLayoutOnChange:NO
        withColorBlock:^BOOL(UIButton *view, UIColor *attributeValue, id<SCValdiAnimatorProtocol> animator) {
            SCValdiAnimatorTransitionWrap(
                animator, view, { return [view valdi_setTitleColor:attributeValue forState:UIControlStateNormal]; });
            return YES;
        }
        resetBlock:^(UIButton *view, id<SCValdiAnimatorProtocol> animator) {
            SCValdiAnimatorTransitionWrap(animator, view,
                                             { [view valdi_setTitleColor:nil forState:UIControlStateNormal]; });
        }];


    [attributesBinder setPlaceholderViewMeasureDelegate:^UIView *{
        return [UIButton new];
    }];
}

- (BOOL)willEnqueueIntoValdiPool
{
    return self.class == [UIButton class];
}

@end
