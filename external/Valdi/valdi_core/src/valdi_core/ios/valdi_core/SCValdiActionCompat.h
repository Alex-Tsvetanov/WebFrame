//
//  SCValdiActionCompat.h
//  valdi-ios
//
//  Created by Simon Corsin on 7/27/19.
//

#import "valdi_core/SCValdiAction.h"
#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

// Implements SCValdiAction, which bridges to SCValdiFunction
@interface SCValdiActionCompat : NSObject <SCValdiAction>

@end

NS_ASSUME_NONNULL_END
