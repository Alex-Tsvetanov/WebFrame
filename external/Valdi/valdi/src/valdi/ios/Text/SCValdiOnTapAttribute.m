//
//  SCValdiOnTapAttribute.m
//  valdi-ios
//
//  Created by Simon Corsin on 12/21/22.
//

#import "valdi/ios/Text/SCValdiOnTapAttribute.h"

@implementation SCValdiOnTapAttribute

- (instancetype)initWithCallback:(id<SCValdiFunction>)callback
{
    self = [super init];

    if (self) {
        _callback = callback;
    }

    return self;
}

@end
