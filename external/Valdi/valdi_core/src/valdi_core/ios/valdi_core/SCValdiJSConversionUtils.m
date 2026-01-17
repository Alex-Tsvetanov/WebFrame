//
//  SCValdiJSConversionUtils.m
//  valdi-ios
//
//  Created by Simon Corsin on 12/21/18.
//

#import "valdi_core/SCValdiJSConversionUtils.h"
#import "valdi_core/SCValdiUndefinedValue.h"
#import "valdi_core/SCValdiError.h"

void SCValdiThrowInvalidType(id js, NSString *expectedType) {
    SCValdiErrorThrow([NSString stringWithFormat:@"Could not convert instance %@ of type %@ to expected type %@", js, [js class], expectedType]);
}

NSString *SCValdiNSStringFromJS(id js) {
    if ([js isKindOfClass:[NSString class]]) {
        return js;
    }
    if ([js isKindOfClass:[NSNumber class]]) {
        return [js description];
    }
    SCValdiThrowInvalidType(js, @"NSString");
    return nil;
}

NSNumber *SCValdiNSNumberIntegerFromJS(id js) {
    if ([js isKindOfClass:[NSNumber class]]) {
        return js;
    }

    if ([js isKindOfClass:[NSString class]]) {
        return @([js integerValue]);
    }

    SCValdiThrowInvalidType(js, @"NSNumber");
    return nil;
}

NSInteger SCValdiNSIntegerFromJS(id js) {
    if ([js isKindOfClass:[NSNumber class]]) {
        return [js integerValue];
    }

    if ([js isKindOfClass:[NSString class]]) {
        return [js integerValue];
    }

    SCValdiThrowInvalidType(js, @"NSInteger");
    return 0;
}

NSNumber *SCValdiNSNumberDoubleFromJS(id js) {
    if ([js isKindOfClass:[NSNumber class]]) {
        return js;
    }

    if ([js isKindOfClass:[NSString class]]) {
        return @([js doubleValue]);
    }

    SCValdiThrowInvalidType(js, @"NSNumber");
    return nil;
}

double SCValdiDoubleFromJS(id js) {
    if ([js isKindOfClass:[NSNumber class]]) {
        return [js doubleValue];
    }

    if ([js isKindOfClass:[NSString class]]) {
        return [js doubleValue];
    }

    SCValdiThrowInvalidType(js, @"NSInteger");
    return 0;
}

NSNumber *SCValdiNSNumberBoolFromJS(id js) {
    if ([js isKindOfClass:[NSNumber class]]) {
        return js;
    }

    if ([js isKindOfClass:[NSString class]]) {
        return @([js boolValue]);
    }

    SCValdiThrowInvalidType(js, @"NSNumber");
    return nil;
}

BOOL SCValdiBoolFromJS(id js) {
    if ([js isKindOfClass:[NSNumber class]]) {
        return [js boolValue];
    }

    if ([js isKindOfClass:[NSString class]]) {
        return [js boolValue];
    }

    SCValdiThrowInvalidType(js, @"NSNumber");
    return 0;
}

UIColor *SCValdiUIColorFromJS(id js) {
    if ([js isKindOfClass:[NSNumber class]]) {
        int64_t value = [js integerValue];
        CGFloat r = (CGFloat)((value >> 24) & 0xFF) / 255.0;
        CGFloat g = (CGFloat)((value >> 16) & 0xFF) / 255.0;
        CGFloat b = (CGFloat)((value >> 8) & 0xFF) / 255.0;
        CGFloat a = (CGFloat)(value & 0xFF) / 255.0;

        return [UIColor colorWithRed:r green:g blue:b alpha:a];
    }
    SCValdiThrowInvalidType(js, @"NSNumber");
    return nil;
}

NSArray *SCValdiMapArray(NSArray *array, id(^block)(id)) {
    NSUInteger arrayCount = array.count;
    NSMutableArray *outArray = [NSMutableArray arrayWithCapacity:arrayCount];
    for (id value in array) {
        id outValue = block(value);
        if (!outValue){
            outValue = [NSNull null];
        }
        [outArray addObject:outValue];
    }

    return outArray;
}

id SCValdiGetParameterValueOrNull(NSArray *array, NSInteger index) {
    if (index >= (NSInteger)array.count) {
        return nil;
    }

    return array[index];
}

id SCValdiUnwrapNull(id value) {
    if ([value isKindOfClass:[NSNull class]] || [value isKindOfClass:[SCValdiUndefinedValue class]]) {
        return nil;
    }
    return value;
}

id SCValdiCastToClassOrThrow(id instance, Class cls) {
    if ([instance isKindOfClass:cls]) {
        return instance;
    }
    SCValdiThrowInvalidType(instance, NSStringFromClass(cls));
    return nil;
}

id SCValdiCastToProtocolOrThrow(id instance, Protocol *protocol) {
    if ([instance conformsToProtocol:protocol]) {
        return instance;
    }
    SCValdiThrowInvalidType(instance, NSStringFromProtocol(protocol));
    return nil;
}

id SCValdiEnsureInstanceImplementsSelector(id instance, SEL selector) {
    if (![instance respondsToSelector:selector]) {
        NSString *selectorName = NSStringFromSelector(selector);
        NSString *className = NSStringFromClass([instance class]);
        SCValdiErrorThrow([NSString stringWithFormat:@"Unimplemented method %@ in instance of type %@", selectorName, className]);
    }

    return instance;
}

@interface SCValdiWrappedObjCInstance: NSObject

@property (strong, nonatomic) id objcInstance;

@end

@implementation SCValdiWrappedObjCInstance

@end

id SCValdiWrapNativeInstance(id instance) {
    SCValdiWrappedObjCInstance *wrappedInstance = [SCValdiWrappedObjCInstance new];
    wrappedInstance.objcInstance = instance;
    return wrappedInstance;
}

id SCValdiUnwrapNativeInstance(id jsInstance) {
    return SCValdiCastToClass(jsInstance, SCValdiWrappedObjCInstance).objcInstance;
}
