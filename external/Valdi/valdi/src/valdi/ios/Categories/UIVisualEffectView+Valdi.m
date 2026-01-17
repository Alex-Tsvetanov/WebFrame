//
//  UIVisualEffectView+Valdi.m
//  Valdi
//
//  Created by Nathaniel Parrott on 8/20/18.
//

#import "valdi/ios/Categories/UIVisualEffectView+Valdi.h"

#import "valdi/ios/Categories/UIView+Valdi.h"

static UIBlurEffectStyle _SCBlurEffectStyleFromString(NSString *style)
{
    static NSDictionary<NSString *, NSNumber *> *map;
    static dispatch_once_t onceToken;
    
    dispatch_once(&onceToken, ^{
        if (@available(iOS 13.0, *)) {
            map = @{
                @"extraLight" : @(UIBlurEffectStyleExtraLight),
                @"light" : @(UIBlurEffectStyleLight),
                @"dark" : @(UIBlurEffectStyleDark),
                @"regular" : @(UIBlurEffectStyleRegular),
                @"prominent" : @(UIBlurEffectStyleProminent),
                /* 
                 * Blur styles available in iOS 13.
                 * Styles which automatically adapt to the user interface style:
                 */
                @"systemUltraThinMaterial": @(UIBlurEffectStyleSystemUltraThinMaterial),
                @"systemThinMaterial": @(UIBlurEffectStyleSystemThinMaterial),
                @"systemMaterial": @(UIBlurEffectStyleSystemMaterial),
                @"systemThickMaterial": @(UIBlurEffectStyleSystemThickMaterial),
                @"systemChromeMaterial": @(UIBlurEffectStyleSystemChromeMaterial),
                /* And always-light and always-dark versions:
                 */
                @"systemUltraThinMaterialLight": @(UIBlurEffectStyleSystemUltraThinMaterialLight),
                @"systemThinMaterialLight": @(UIBlurEffectStyleSystemThinMaterialLight),
                @"systemMaterialLight": @(UIBlurEffectStyleSystemMaterialLight),
                @"systemThickMaterialLight": @(UIBlurEffectStyleSystemThickMaterialLight),
                @"systemChromeMaterialLight": @(UIBlurEffectStyleSystemChromeMaterialLight),
                @"systemUltraThinMaterialDark": @(UIBlurEffectStyleSystemUltraThinMaterialDark),
                @"systemThinMaterialDark": @(UIBlurEffectStyleSystemThinMaterialDark),
                @"systemMaterialDark": @(UIBlurEffectStyleSystemMaterialDark),
                @"systemThickMaterialDark": @(UIBlurEffectStyleSystemThickMaterialDark),
                @"systemChromeMaterialDark": @(UIBlurEffectStyleSystemChromeMaterialDark),
            };
        } else {
            map = @{
                @"extraLight" : @(UIBlurEffectStyleExtraLight),
                @"light" : @(UIBlurEffectStyleLight),
                @"dark" : @(UIBlurEffectStyleDark),
                @"regular" : @(UIBlurEffectStyleRegular),
                @"prominent" : @(UIBlurEffectStyleProminent),
                /*
                 * this logic contains a fallback for iOS 12
                 * https://ikyle.me/blog/2022/uiblureffectstyl
                 */
                @"systemUltraThinMaterial": @(UIBlurEffectStyleRegular),
                @"systemThinMaterial": @(UIBlurEffectStyleRegular),
                @"systemMaterial": @(UIBlurEffectStyleProminent),
                @"systemThickMaterial": @(UIBlurEffectStyleProminent),
                @"systemChromeMaterial": @(UIBlurEffectStyleProminent),
                @"systemUltraThinMaterialLight": @(UIBlurEffectStyleLight),
                @"systemThinMaterialLight": @(UIBlurEffectStyleLight),
                @"systemMaterialLight": @(UIBlurEffectStyleLight),
                @"systemThickMaterialLight": @(UIBlurEffectStyleLight),
                @"systemChromeMaterialLight": @(UIBlurEffectStyleLight),
                @"systemUltraThinMaterialDark": @(UIBlurEffectStyleDark),
                @"systemThinMaterialDark": @(UIBlurEffectStyleDark),
                @"systemMaterialDark": @(UIBlurEffectStyleDark),
                @"systemThickMaterialDark": @(UIBlurEffectStyleDark),
                @"systemChromeMaterialDark": @(UIBlurEffectStyleDark),
            };
        }
    });
    
    NSNumber *val = map[style];

    return val ? (UIBlurEffectStyle)val.integerValue : UIBlurEffectStyleLight;
}

@implementation UIVisualEffectView (Valdi)

+ (void)bindAttributes:(id<SCValdiAttributesBinderProtocol>)attributesBinder
{
    // TODO: (nate) Support vibrancy
    [attributesBinder bindAttribute:@"blurStyle"
           invalidateLayoutOnChange:NO
                    withStringBlock:^BOOL(UIVisualEffectView *view, NSString *attributeValue, id<SCValdiAnimatorProtocol> animator) {
        view.effect = [UIBlurEffect effectWithStyle:_SCBlurEffectStyleFromString(attributeValue)];
        return YES;
    }
                         resetBlock:^(UIVisualEffectView *view, id<SCValdiAnimatorProtocol> animator) {
        view.effect = nil;
    }];
}

@end
