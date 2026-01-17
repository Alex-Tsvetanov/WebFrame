//
//  NSURL+Valdi.m
//  Valdi
//
//  Created by Simon Corsin on 5/8/18.
//

#import "valdi_core/NSURL+Valdi.h"

@implementation NSURL (Valdi)

+ (NSURL *)URLFromValdiAttributeValue:(id)attributeValue
{
    if ([attributeValue isKindOfClass:[NSString class]]) {
        // Image src attribute values with colons (e.g. common:back_arrow) will technically be valid urls with
        // valid schemes, therefore we explicitly check for ://
        if (![attributeValue containsString:@"://"]) {
            return nil;
        }
        return [NSURL URLWithString:[attributeValue stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceCharacterSet]]];
    } else if ([attributeValue isKindOfClass:[NSURL class]]) {
        return attributeValue;
    }

    return nil;
}

@end
