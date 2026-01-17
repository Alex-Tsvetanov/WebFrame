//
//  SCValdiBridgeModuleUtils.m
//  Valdi
//
//  Created by Simon Corsin on 9/19/18.
//

#import "SCValdiBridgeModuleUtils.h"

#import "valdi_core/SCValdiActionWithBlock.h"

@implementation SCValdiBridgeObserver {
    NSMutableArray *_callbacks;
    __weak id<SCValdiBridgeObserverDelegate> _delegate;
}

- (instancetype)init
{
    self = [super init];

    if (self) {
        _callbacks = [NSMutableArray array];
    }

    return self;
}

- (void)_removeCallback:(id<SCValdiFunction>)callback
{
    if (!callback) {
        return;
    }

    @synchronized (_callbacks) {
        [_callbacks removeObject:callback];
    }
}


- (void)setDelegate:(id<SCValdiBridgeObserverDelegate>)delegate
{
    @synchronized (_callbacks) {
        _delegate = delegate;
    }
}

- (id<SCValdiBridgeObserverDelegate>)delegate
{
    @synchronized (_callbacks) {
        return _delegate;
    }
}

- (BOOL)performWithMarshaller:(SCValdiMarshallerRef)marshaller
{
    id<SCValdiFunction> function = SCValdiMarshallerGetFunction(marshaller, 0);

    @synchronized (_callbacks) {
        [_callbacks addObject:function];
        [_delegate bridgeObserverDidAddNewCallback:self];
    }

    NSInteger idx = SCValdiMarshallerPushMap(marshaller, 1);

    __weak id<SCValdiFunction> weakFunction = function;
    SCValdiMarshallerPushFunctionWithBlock(marshaller, ^BOOL(SCValdiMarshallerRef  _Nonnull parameters) {
        [self _removeCallback:weakFunction];
        return NO;
    });
    SCValdiMarshallerPutMapPropertyUninterned(marshaller, @"cancel", idx);

    return YES;
}

- (void)notifyWithMarshaller:(SCValdiMarshaller *)marshaller
{
    if (!marshaller) {
        SCValdiMarshallerScoped(innerMarshaller, {
            [self notifyWithMarshaller:innerMarshaller];
        });
        return;
    }

    NSArray *callbacks;
    @synchronized (_callbacks) {
        callbacks = [_callbacks copy];
    }

    for (id<SCValdiFunction> callback in callbacks) {
        [callback performWithMarshaller:marshaller];
    }
}

@end
