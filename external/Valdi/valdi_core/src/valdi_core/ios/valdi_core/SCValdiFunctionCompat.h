//
//  SCValdiFunctionCompat.h
//  valdi-ios
//
//  Created by Simon Corsin on 7/23/19.
//

#import "valdi_core/SCValdiFunction.h"
#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

// Implements SCValdiFunction, which bridges to SCValdiAction
@interface SCValdiFunctionCompat : NSObject <SCValdiFunction>

@end

NS_ASSUME_NONNULL_END
