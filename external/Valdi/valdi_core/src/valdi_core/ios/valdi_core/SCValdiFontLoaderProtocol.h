
#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

typedef NS_ENUM(NSUInteger, SCUILegibilityWeight) {
    SCUILegibilityWeightRegular = 0,
    SCUILegibilityWeightBold,
};

NS_ASSUME_NONNULL_BEGIN

@protocol SCValdiFontLoaderProtocol <NSObject>

- (UIFont* _Nullable)loadFontWithName:(NSString*)fontName fontSize:(CGFloat)fontSize;

- (UIFont* _Nullable)loadFontWithName:(NSString*)fontName
                             fontSize:(CGFloat)fontSize
                     legibilityWeight:(SCUILegibilityWeight)legibilityWeight;

@optional
// Set this to YES if you wish to ignore the environment's trait for UILegibilityWeight. If you are using this in tandem
// with `loadFontWithName:fontSize:legibilityWeight`, you must account explicitly for boldness in the font you return.
- (BOOL)shouldBypassContextForLegibilityWeight;

@end

NS_ASSUME_NONNULL_END
