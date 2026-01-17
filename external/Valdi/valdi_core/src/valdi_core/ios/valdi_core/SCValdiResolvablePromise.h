// Copyright © 2023 Snap, Inc. All rights reserved.

#import "valdi_core/SCValdiPromise.h"
#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface SCValdiResolvablePromise<ValueType> : SCValdiPromise <ValueType>

- (void)fulfillWithSuccessValue:(ValueType)value;

- (void)fulfillWithError:(NSError*)error;

- (void)setCancelCallback:(dispatch_block_t)cancelCallback;

@end

NS_ASSUME_NONNULL_END
