//
//  SCValdiFunctionWithBlock.m
//  valdi-ios
//
//  Created by Simon Corsin on 7/19/19.
//

#import "valdi_core/SCValdiFunctionWithBlock.h"

@implementation SCValdiFunctionWithBlock {
    SCValdiFunctionBlock _block;
}

- (instancetype)initWithBlock:(SCValdiFunctionBlock)block
{
    self = [super init];

    if (self) {
        _block = block;
    }

    return self;
}

+ (instancetype)functionWithBlock:(SCValdiFunctionBlock)block
{
    return [[self alloc] initWithBlock:block];
}

- (BOOL)performWithMarshaller:(SCValdiMarshallerRef)marshaller
{
    return _block(marshaller);
}

@end
