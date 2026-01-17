//
//  SCValdiMarshallableObject.h
//  valdi-ios
//
//  Created by Simon Corsin on 8/5/19.
//

#import "valdi_core/SCValdiMarshallable.h"
#import "valdi_core/SCValdiMarshallableObjectDescriptor.h"
#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@protocol SCValdiMarshallableObject <NSObject>

@optional
+ (SCValdiMarshallableObjectDescriptor)valdiMarshallableObjectDescriptor;

@end

@class SCValdiMarshallableObjectRegistry;

@interface SCValdiMarshallableObject : NSObject <SCValdiMarshallable, NSCopying, SCValdiMarshallableObject>

- (instancetype)init;
- (instancetype)initWithFieldValues:(const void* _Nullable)start, ...;
- (instancetype)initWithObjectRegistry:(__unsafe_unretained SCValdiMarshallableObjectRegistry*)objectRegistry
                               storage:(void*)storage;

/**
 * Unmarshall the object from its dictionary representation.
 * Will throw on failure.
 */
+ (instancetype)objectFromDictionary:(NSDictionary<NSString*, id>*)dictionary;

@end

@interface SCValdiProxyMarshallableObject : SCValdiMarshallableObject

@end

extern NSInteger SCValdiMarshallableObjectMarshall(SCValdiMarshallerRef marshaller,
                                                   __unsafe_unretained SCValdiMarshallableObject* marshallableObject);
extern id SCValdiMarshallableObjectUnmarshall(SCValdiMarshallerRef _Nonnull marshaller,
                                              NSInteger objectIndex,
                                              Class objectClass);

extern NSInteger SCValdiMarshallableObjectMarshallInterface(SCValdiMarshallerRef _Nonnull marshaller,
                                                            id instance,
                                                            Class objectClass);

/// For the time being, Valdi-generated generic classes have to conform to SCValdiMarshallableObject
/// even though calling plain pushToValdiMarshaller: on them doesn't make sense, as the marshaller will
/// not have enough specialization information to correctly marshall/unmarshall a given instance.
@interface SCValdiMarshallableGenericObject : SCValdiMarshallableObject

/// If you'd like to marshall an instance of your generic class, put it in an instance of a wrapper class
/// that has a specialized generic property.
- (NSInteger)pushToValdiMarshaller:(SCValdiMarshallerRef)marshaller
    __attribute__((unavailable("pushToValdiMarshaller: is not available for generic objects")));

@end

NS_ASSUME_NONNULL_END
