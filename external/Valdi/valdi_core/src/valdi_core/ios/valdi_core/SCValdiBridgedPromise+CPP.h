// Copyright © 2023 Snap, Inc. All rights reserved.

#import "valdi_core/SCValdiBridgedPromise.h"
#import "valdi_core/SCValdiObjCValue.h"
#import "valdi_core/cpp/Utils/Promise.hpp"
#import "valdi_core/cpp/Utils/ValueMarshaller.hpp"
#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface SCValdiBridgedPromise (CPP)

- (instancetype)initWithPromise:(const Valdi::Ref<Valdi::Promise>&)promise
                valueMarshaller:(const Valdi::Ref<Valdi::ValueMarshaller<ValdiIOS::ObjCValue>>&)valueMarshaller;

@end

@interface SCValdiBridgedPromiseCallback : SCValdiPromiseCallback

- (instancetype)initWithPromiseCallback:(const Valdi::Ref<Valdi::PromiseCallback>&)promiseCallback
                        valueMarshaller:(const Valdi::Ref<Valdi::ValueMarshaller<ValdiIOS::ObjCValue>>&)valueMarshaller;

@end

namespace ValdiIOS {

Valdi::Ref<Valdi::Promise> PromiseFromSCValdiPromise(
    id<SCValdiPromise> promise, const Valdi::Ref<Valdi::ValueMarshaller<ValdiIOS::ObjCValue>>& valueMarshaller);

}

NS_ASSUME_NONNULL_END
