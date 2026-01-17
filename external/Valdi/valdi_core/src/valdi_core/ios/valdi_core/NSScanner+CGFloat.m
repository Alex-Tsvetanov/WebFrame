//
//  NSScanner+CGFloat.m
//  Valdi
//
//  Created by Simon Corsin on 4/20/18.
//

#import "valdi_core/NSScanner+CGFloat.h"

@implementation NSScanner (CGFloat)

- (BOOL)scanCGFloat:(CGFloat *)cgFloat
{
#if CGFLOAT_IS_DOUBLE
    return [self scanDouble:cgFloat];
#else
    return [self scanFloat:cgFloat];
#endif
}

@end
