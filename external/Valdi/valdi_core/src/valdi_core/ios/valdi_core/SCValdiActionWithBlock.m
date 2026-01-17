//
//  SCValdiActionWithBlock.m
//  Valdi
//
//  Created by Simon Corsin on 5/7/18.
//

#import "valdi_core/SCValdiActionWithBlock.h"

#import "valdi_core/SCValdiActionUtils.h"

@implementation SCValdiActionWithBlock {
    SCValdiActionBlock _block;
}

- (instancetype)initWithBlock:(SCValdiActionBlock)block
{
    self = [super init];

    if (self) {
        _block = block;
    }

    return self;
}

+ (instancetype)actionWithBlock:(SCValdiActionBlock)block
{
    return [[self alloc] initWithBlock:block];
}

- (void)performWithSender:(id)sender
{
    [self performWithParameters:SCValdiActionParametersWithSender(sender)];
}

- (id)performWithParameters:(NSArray<id> *)parameters
{
    if (_block) {
        return _block(parameters);
    }
    return nil;
}

@end
