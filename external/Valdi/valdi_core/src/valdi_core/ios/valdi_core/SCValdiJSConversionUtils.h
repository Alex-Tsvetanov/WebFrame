//
//  SCValdiJSConversionUtils.h
//  valdi-ios
//
//  Created by Simon Corsin on 12/21/18.
//

#import <Foundation/Foundation.h>
#import <UIKit/UIColor.h>

#define SCValdiCastToClass(instance, cls) ((cls*)SCValdiCastToClassOrThrow(instance, [cls class]))
#define SCValdiCastToProtocol(instance, aProtocol)                                                                     \
    ((id<aProtocol>)SCValdiCastToProtocolOrThrow(instance, @protocol(aProtocol)))

extern NSString* SCValdiNSStringFromJS(id js);
extern NSNumber* SCValdiNSNumberIntegerFromJS(id js);
extern NSInteger SCValdiNSIntegerFromJS(id js);
extern NSNumber* SCValdiNSNumberDoubleFromJS(id js);
extern double SCValdiDoubleFromJS(id js);
extern NSNumber* SCValdiNSNumberBoolFromJS(id js);
extern BOOL SCValdiBoolFromJS(id js);

extern UIColor* SCValdiUIColorFromJS(id js);

extern NSArray* SCValdiMapArray(NSArray* array, id (^block)(id));

extern id SCValdiGetParameterValueOrNull(NSArray* array, NSInteger index);
extern id SCValdiUnwrapNull(id value);

extern id SCValdiCastToClassOrThrow(id instance, Class cls);
extern id SCValdiCastToProtocolOrThrow(id instance, Protocol* protocol);
extern id SCValdiEnsureInstanceImplementsSelector(id instance, SEL selector);

extern id SCValdiWrapNativeInstance(id instance);
extern id SCValdiUnwrapNativeInstance(id jsInstance);
