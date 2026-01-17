//
//  SCValdiUndefinedValue.m
//  valdi-ios
//
//  Created by Jeff Johnson on 8/30/18.
//

#import "valdi_core/SCValdiUndefinedValue.h"

@implementation SCValdiUndefinedValue {
}

+ (SCValdiUndefinedValue *)undefined
{
    static dispatch_once_t onceToken;
    static SCValdiUndefinedValue *undefinedValue;
    dispatch_once(&onceToken, ^{
        undefinedValue = [SCValdiUndefinedValue new];
    });
    
    return undefinedValue;
}

@end
