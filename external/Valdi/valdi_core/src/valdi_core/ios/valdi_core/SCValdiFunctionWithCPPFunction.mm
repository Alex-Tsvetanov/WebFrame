//
//  SCValdiFunctionWithCPPFunction.m
//  Valdi
//
//  Created by Simon Corsin on 7/12/18.
//  Copyright © 2018 Snap Inc. All rights reserved.
//

#import "valdi_core/SCValdiFunctionWithCPPFunction.h"
#import "valdi_core/SCValdiValueUtils.h"
#import "valdi_core/SCValdiMarshaller+CPP.h"
#import "valdi_core/cpp/Utils/ValueFunction.hpp"

@implementation SCValdiFunctionWithCPPFunction {
    Valdi::Ref<Valdi::ValueFunction> _function;
}

- (instancetype)initWithCPPFunction:(const Valdi::Ref<Valdi::ValueFunction> &)cppFunction
{
    self = [super init];

    if (self) {
        _function = cppFunction;
    }

    return self;
}

- (BOOL)performWithMarshaller:(SCValdiMarshallerRef)marshaller
{
    return [self performWithMarshaller:*SCValdiMarshallerUnwrap(marshaller) flags:Valdi::ValueFunctionFlagsNone];
}

- (BOOL)performSyncWithMarshaller:(SCValdiMarshallerRef)marshaller propagatesError:(BOOL)propagatesError
{
    Valdi::ValueFunctionFlags flags;
    if (propagatesError) {
        flags = static_cast<Valdi::ValueFunctionFlags>(Valdi::ValueFunctionFlagsCallSync | Valdi::ValueFunctionFlagsPropagatesError);
    } else {
        flags = Valdi::ValueFunctionFlagsCallSync;
    }

    return [self performWithMarshaller:*SCValdiMarshallerUnwrap(marshaller) flags:flags];
}

- (BOOL)performThrottledWithMarshaller:(SCValdiMarshallerRef)marshaller
{
    return [self performWithMarshaller:*SCValdiMarshallerUnwrap(marshaller) flags:Valdi::ValueFunctionFlagsAllowThrottling];
}

- (bool)performWithMarshaller:(Valdi::Marshaller &)marshaller flags:(Valdi::ValueFunctionFlags)flags
{
    marshaller.push((*_function)(Valdi::ValueFunctionCallContext(flags, marshaller.getValues(), marshaller.size(), marshaller.getExceptionTracker())));

    if (flags & Valdi::ValueFunctionFlagsPropagatesError) {
        SCValdiMarshallerCheck(SCValdiMarshallerWrap(&marshaller));
    }

    return YES;
}

- (const Valdi::Ref<Valdi::ValueFunction>&)getFunction
{
    return _function;
}

@end
