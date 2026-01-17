
#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

#import "valdi_core/SCValdiFontLoaderProtocol.h"

NS_ASSUME_NONNULL_BEGIN

@protocol SCValdiFontManagerProtocol <NSObject>

- (UIFont* _Nullable)fontWithName:(NSString*)fontName
                         fontSize:(CGFloat)fontSize
                 legibilityWeight:(SCUILegibilityWeight)legibilityWeight;

- (BOOL)shouldBypassContextForLegibilityWeight;

- (UITraitCollection*)defaultTraitCollection;

@end

NS_ASSUME_NONNULL_END
