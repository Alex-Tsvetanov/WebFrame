//
//  SCValdiFunction.m
//  valdi-ios
//
//  Created by Simon Corsin on 7/31/19.
//

#import "valdi_core/SCValdiFunction.h"

void SCValdiFunctionSafePerform(id<SCValdiFunction> function, SCValdiMarshallerRef marshaller) {
    if (![function performWithMarshaller:marshaller]) {
        SCValdiMarshallerPushUndefined(marshaller);
    }
}

void SCValdiFunctionSafePerformSync(id<SCValdiFunction> function, SCValdiMarshallerRef marshaller, BOOL propagatesError) {
    if ([function respondsToSelector:@selector(performSyncWithMarshaller:propagatesError:)]) {
        if (![function performSyncWithMarshaller:marshaller propagatesError:propagatesError]) {
            SCValdiMarshallerPushUndefined(marshaller);
        }
    } else {
        SCValdiFunctionSafePerform(function, marshaller);
    }
}

void SCValdiFunctionSafePerformThrottled(id<SCValdiFunction> function, SCValdiMarshallerRef marshaller) {
    if ([function respondsToSelector:@selector(performThrottledWithMarshaller:)]) {
        if (![function performThrottledWithMarshaller:marshaller]) {
            SCValdiMarshallerPushUndefined(marshaller);
        }
    } else {
        SCValdiFunctionSafePerform(function, marshaller);
    }
}
