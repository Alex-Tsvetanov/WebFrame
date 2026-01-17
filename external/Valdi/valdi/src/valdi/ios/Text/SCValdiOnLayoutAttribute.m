
#import "valdi/ios/Text/SCValdiOnLayoutAttribute.h"

@implementation SCValdiOnLayoutAttribute

- (instancetype)initWithCallback:(id<SCValdiFunction>)callback
{
    self = [super init];

    if (self) {
        _callback = callback;
    }

    return self;
}

@end
