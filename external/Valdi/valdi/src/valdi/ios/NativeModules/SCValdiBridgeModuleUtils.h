//
//  SCValdiBridgeModuleUtils.h
//  Valdi
//
//  Created by Simon Corsin on 9/19/18.
//

#import "valdi_core/SCValdiFunctionWithBlock.h"

#import <Foundation/Foundation.h>

#define BRIDGE_METHOD(selectorName)                                                                                    \
    [SCValdiFunctionWithBlock functionWithBlock:^BOOL(SCValdiMarshaller* marshaller) {                                 \
        [self selectorName:marshaller];                                                                                \
        return YES;                                                                                                    \
    }]

@class SCValdiBridgeObserver;

@protocol SCValdiBridgeObserverDelegate <NSObject>

- (void)bridgeObserverDidAddNewCallback:(SCValdiBridgeObserver*)observer;

@end

@interface SCValdiBridgeObserver : NSObject <SCValdiFunction>

@property (weak) id<SCValdiBridgeObserverDelegate> delegate;

- (void)notifyWithMarshaller:(SCValdiMarshaller*)marshaller;

@end
