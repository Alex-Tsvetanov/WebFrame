// Copyright © 2023 Snap, Inc. All rights reserved.

#import "valdi_core/SCValdiPromise.h"
#import "valdi_core/SCValdiError.h"

@interface SCValdiResolvedPromise: SCValdiPromise
@end

@implementation SCValdiResolvedPromise {
    id _value;
}

- (instancetype)initWithValue:(id)value
{
    self = [super init];

    if (self) {
        _value = value;
    }

    return self;
}

- (void)onCompleteWithCallback:(id<SCValdiPromiseCallback>)callback
{
    [callback onSuccessWithValue:_value];
}

- (void)cancel
{
}

- (BOOL)isCancelable
{
    return NO;
}

- (void)setPeer:(void*)peer
{
    // NO-OP
}

- (void*)getPeer
{
    // NO-OP
    return nil;
}
@end

@interface SCValdiRejectedPromise: SCValdiPromise
@end

@implementation SCValdiRejectedPromise {
    NSError *_error;
}

- (instancetype)initWithError:(NSError *)error
{
    self = [super init];

    if (self) {
        _error = error;
    }

    return self;
}

- (void)onCompleteWithCallback:(id<SCValdiPromiseCallback>)callback
{
    [callback onFailureWithError:_error];
}

- (void)cancel
{
}

- (BOOL)isCancelable
{
    return NO;
}

- (void)setPeer:(void*)peer
{
    // NO-OP
}

- (void*)getPeer
{
    // NO-OP
    return nil;
}
@end

@implementation SCValdiPromiseCallback

- (void)onSuccessWithValue:(id)value
{
    SCValdiErrorThrowUnimplementedMethod();
}

- (void)onFailureWithError:(NSError *)error
{
    SCValdiErrorThrowUnimplementedMethod();
}

@end

@implementation SCValdiPromise

- (void)onCompleteWithCallback:(SCValdiPromiseCallback<id> *)callback
{
    SCValdiErrorThrowUnimplementedMethod();
}

- (void)onCompleteWithCallbackBlock:(SCValdiPromiseCallbackBlock)block
{
    SCValdiErrorThrowUnimplementedMethod();
}

- (void)cancel
{

}

- (BOOL)isCancelable
{
    return NO;
}

- (void)setPeer:(void*)peer
{
    SCValdiErrorThrowUnimplementedMethod();
}

- (void*)getPeer
{
    SCValdiErrorThrowUnimplementedMethod();
}

+ (SCValdiPromise<id> *)resolvedPromiseWithValue:(id)value
{
    return [[SCValdiResolvedPromise alloc] initWithValue:value];
}

+ (SCValdiPromise<id> *)rejectedPromiseWithError:(NSError *)error
{
    return [[SCValdiRejectedPromise alloc] initWithError:error];
}

@end

void SCValdiPromiseCallbackForwardSuccess(__unsafe_unretained id promiseCallback, __unsafe_unretained id value)
{
    if ([promiseCallback respondsToSelector:@selector(onSuccessWithValue:)]) {
        [(id<SCValdiPromiseCallback>)promiseCallback onSuccessWithValue:value];
    } else {
        ((SCValdiPromiseCallbackBlock)promiseCallback)(value, nil);
    }
}

void SCValdiPromiseCallbackForwardFailure(__unsafe_unretained id promiseCallback, __unsafe_unretained NSError *error)
{
    if ([promiseCallback respondsToSelector:@selector(onFailureWithError:)]) {
        [(id<SCValdiPromiseCallback>)promiseCallback onFailureWithError:error];
    } else {
        ((SCValdiPromiseCallbackBlock)promiseCallback)(nil, error);
    }
}
