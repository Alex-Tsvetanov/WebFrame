//
//  SCValdiCancelable.h
//  valdi-ios
//
//  Created by Simon Corsin on 4/1/19.
//

#import "valdi_core/SCNValdiCoreCancelable.h"
#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@protocol SCValdiCancelable <NSObject, SCNValdiCoreCancelable>

- (void)cancel;

@end

NS_ASSUME_NONNULL_END
