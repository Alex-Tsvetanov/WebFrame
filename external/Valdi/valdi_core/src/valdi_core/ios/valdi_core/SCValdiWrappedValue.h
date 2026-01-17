//
//  SCValdiWrappedValue.h
//  valdi-ios
//
//  Created by Simon Corsin on 8/29/18.
//

#import "valdi_core/SCValdiNativeConvertible.h"
#import <Foundation/Foundation.h>

@interface SCValdiWrappedValue : NSObject <SCValdiNativeConvertible>

- (NSString*)debugString;

@end
