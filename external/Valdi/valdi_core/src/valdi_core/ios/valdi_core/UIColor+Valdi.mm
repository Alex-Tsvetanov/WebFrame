//
//  UIColor+Valdi.mm
//  Valdi
//
//  Created by Simon Corsin on 12/12/17.
//  Copyright © 2017 Snap Inc. All rights reserved.
//

#import "valdi_core/UIColor+Valdi.h"
#include <cstdint>

@implementation UIColor (Valdi)

- (int64_t)valdiAttributeValue
{
    CGFloat r, g, b, a;
    [self getRed:&r green:&g blue:&b alpha:&a];

    return (lroundf)(r * 255) << 24 | (lroundf)(g * 255) << 16 | (lroundf)(b * 255) << 8 | (lroundf)(a * 255);
}

UIColor *UIColorFromValdiAttributeValue(int64_t value)
{
    CGFloat r = (CGFloat)((value >> 24) & 0xFF) / 255.0;
    CGFloat g = (CGFloat)((value >> 16) & 0xFF) / 255.0;
    CGFloat b = (CGFloat)((value >> 8) & 0xFF) / 255.0;
    CGFloat a = (CGFloat)(value & 0xFF) / 255.0;

    return [UIColor colorWithRed:r green:g blue:b alpha:a];
}

@end
