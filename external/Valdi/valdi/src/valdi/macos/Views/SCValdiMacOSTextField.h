//
//  SCValdiMacOSTextField.h
//  valdi-macos
//
//  Created by Simon Corsin on 10/13/20.
//

#import "valdi/macos/SCValdiMacOSAttributesBinder.h"
#import <Cocoa/Cocoa.h>

NS_ASSUME_NONNULL_BEGIN

@interface SCValdiMacOSTextField : NSTextField

+ (void)bindAttributes:(SCValdiMacOSAttributesBinder*)attributesBinder;

@end

NS_ASSUME_NONNULL_END
