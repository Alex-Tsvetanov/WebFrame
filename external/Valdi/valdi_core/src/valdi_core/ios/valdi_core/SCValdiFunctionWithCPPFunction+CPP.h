//
//  SCValdiFunctionWithCPPFunction+CPP.h
//  valdi
//
//  Created by Simon Corsin on 7/23/19.
//

#import "valdi_core/SCValdiFunctionWithCPPFunction.h"
#import <Foundation/Foundation.h>

#import "valdi_core/cpp/Utils/Marshaller.hpp"
#import "valdi_core/cpp/Utils/Value.hpp"
#import "valdi_core/cpp/Utils/ValueFunction.hpp"

@interface SCValdiFunctionWithCPPFunction ()

- (instancetype)initWithCPPFunction:(const Valdi::Ref<Valdi::ValueFunction>&)cppFunction;

- (bool)performWithMarshaller:(Valdi::Marshaller&)marshaller flags:(Valdi::ValueFunctionFlags)flags;

- (const Valdi::Ref<Valdi::ValueFunction>&)getFunction;

@end
