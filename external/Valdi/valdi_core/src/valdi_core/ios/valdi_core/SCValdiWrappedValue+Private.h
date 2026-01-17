//
//  SCValdiWrappedValue+Private.h
//  valdi
//
//  Created by Simon Corsin on 8/29/18.
//

#import "valdi_core/SCValdiWrappedValue.h"
#import "valdi_core/cpp/Utils/Value.hpp"

@interface SCValdiWrappedValue ()

@property (readonly, nonatomic) const Valdi::Value& value;

- (instancetype)initWithValue:(Valdi::Value)value;

@end
