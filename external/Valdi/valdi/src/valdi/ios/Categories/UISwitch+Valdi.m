//
//  UISwitch+Valdi.m
//  Valdi
//
//  Created by Andrew Thieck on 9/20/18.
//

#import "valdi/ios/Categories/UISwitch+Valdi.h"

#import "valdi/ios/Categories/UIView+Valdi.h"

@implementation UISwitch (Valdi)

+ (void)bindAttributes:(id<SCValdiAttributesBinderProtocol>)attributesBinder
{
    [attributesBinder bindAttribute:@"onTintColor"
        invalidateLayoutOnChange:NO
        withColorBlock:^BOOL(UISwitch *switchView, UIColor *color, id<SCValdiAnimatorProtocol> animator) {
            SCValdiAnimatorTransitionWrap(animator, switchView, { switchView.onTintColor = color; });
            return YES;
        }
        resetBlock:^(UISwitch *switchView, id<SCValdiAnimatorProtocol> animator) {
            switchView.onTintColor = [UIColor grayColor];
        }];

    [attributesBinder bindAttribute:@"offTintColor"
        invalidateLayoutOnChange:NO
        withColorBlock:^BOOL(UISwitch *switchView, UIColor *color, id<SCValdiAnimatorProtocol> animator) {
            SCValdiAnimatorTransitionWrap(animator, switchView, {
                switchView.tintColor = color;
                switchView.layer.cornerRadius = switchView.frame.size.height / 2.0;
                switchView.backgroundColor = color;
            });
            return YES;
        }
        resetBlock:^(UISwitch *switchView, id<SCValdiAnimatorProtocol> animator) {
            switchView.onTintColor = [UIColor grayColor];
        }];

    [attributesBinder bindAttribute:@"on"
        invalidateLayoutOnChange:NO
        withBoolBlock:^BOOL(UISwitch *switchView, BOOL on, id<SCValdiAnimatorProtocol> animator) {
            SCValdiAnimatorTransitionWrap(animator, switchView, { switchView.on = on; });
            return YES;
        }
        resetBlock:^(UISwitch *switchView, id<SCValdiAnimatorProtocol> animator) {
            switchView.on = NO;
        }];

    [attributesBinder setPlaceholderViewMeasureDelegate:^UIView *{
        return [UISwitch new];
    }];
}

@end
