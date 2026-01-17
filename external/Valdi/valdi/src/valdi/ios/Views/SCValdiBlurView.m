//
//  SCValdiBlurView.m
//  Valdi
//
//  Created by Brandon Francis on 3/9/19.
//

#import "valdi/ios/Views/SCValdiBlurView.h"

#import "valdi/ios/Categories/UIView+Valdi.h"

@implementation SCValdiBlurView

- (instancetype)initWithFrame:(CGRect)frame
{
    self = [super initWithFrame:frame];
    if (self) {
        [self _setStyle:UIBlurEffectStyleLight animator:nil];
    }
    return self;
}

#pragma mark - SCValdiContentViewProviding

- (UIView *)contentViewForInsertingValdiChildren
{
    return self.contentView;
}

#pragma mark - UIView+Valdi

- (BOOL)requiresShapeLayerForBorderRadius
{
    return YES;
}

- (BOOL)willEnqueueIntoValdiPool
{
    return YES;
}

#pragma mark - Internal methods

- (BOOL)_setStyle:(UIBlurEffectStyle)style animator:(nullable id<SCValdiAnimatorProtocol> )animator
{
    self.effect = [UIBlurEffect effectWithStyle:style];
    if (animator) {
        [animator addTransitionOnLayer:self.layer];
    }
    return YES;
}

#pragma mark - Static methods

static UIBlurEffectStyle _SCUIBlurEffectStyleFromString(NSString *style)
{
    if ([style isEqualToString:@"light"]) {
        return UIBlurEffectStyleLight;
    } else if ([style isEqualToString:@"dark"]) {
        return UIBlurEffectStyleDark;
    } else if ([style isEqualToString:@"extraLight"]) {
        return UIBlurEffectStyleExtraLight;
    } else if ([style isEqualToString:@"regular"]) {
        return UIBlurEffectStyleRegular;
    } else if ([style isEqualToString:@"prominent"]) {
        return UIBlurEffectStyleProminent;
    }
    return UIBlurEffectStyleLight;
}

+ (void)bindAttributes:(id<SCValdiAttributesBinderProtocol>)attributesBinder
{
    [attributesBinder bindAttribute:@"style"
        invalidateLayoutOnChange:NO
        withStringBlock:^BOOL(SCValdiBlurView *view, NSString *attributeValue, id<SCValdiAnimatorProtocol> animator) {
            UIBlurEffectStyle style = _SCUIBlurEffectStyleFromString(attributeValue);
            return [view _setStyle:style animator:animator];
        }
        resetBlock:^(SCValdiBlurView *view, id<SCValdiAnimatorProtocol> animator) {
            [view _setStyle:UIBlurEffectStyleLight animator:animator];
        }];
}

@end
