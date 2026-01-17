
#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

@protocol SCValdiFontManagerProtocol;

/**
 * This is a valdi representation of a UIFont that has not been impacted by DynamicType/accessibility yet.
 * It is made from an "unscaled" UIFont and all its scaling parameters (except the user's settings)
 *
 * When the user settings (UITraitCollection) becomes available, we can then resolve the final scaled UIFont.
 * That resolved scaled UIFont can then be applied to the UILabel/UITextField/UITextView and used for measurement
 */
@interface SCValdiFont : NSObject

@property (readonly, nonatomic) id<SCValdiFontManagerProtocol> fontManager;

- (instancetype)initWithFont:(UIFont*)font fontManager:(id<SCValdiFontManagerProtocol>)fontManager;

- (instancetype)initWithFont:(UIFont*)font
                   textStyle:(UIFontTextStyle)textStyle
                     maxSize:(CGFloat)maxSize
                 fontManager:(id<SCValdiFontManagerProtocol>)fontManager;

- (UIFont*)resolveFontFromTraitCollection:(UITraitCollection*)traitCollection;

+ (SCValdiFont*)fontFromValdiAttribute:(NSString*)attributeValue
                           fontManager:(id<SCValdiFontManagerProtocol>)fontManager;

@end
