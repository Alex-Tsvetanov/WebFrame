//
//  SCValdiMarshallableObject.m
//  valdi-ios
//
//  Created by Simon Corsin on 8/5/19.
//

#import "valdi_core/SCValdiMarshallableObject.h"
#import "valdi_core/SCValdiMarshallableObjectRegistry.h"

@implementation SCValdiMarshallableObject {
    SCValdiMarshallableObjectRegistry *_objectRegistry;
    void *_storage;
}

- (instancetype)init
{
    self = [super init];

    if (self) {
        _objectRegistry = SCValdiMarshallableObjectRegistryGetSharedInstance();
        _storage = [_objectRegistry allocateStorageForClass:[self class]];
    }

    return self;
}

- (instancetype)initWithObjectRegistry:(SCValdiMarshallableObjectRegistry *__unsafe_unretained)objectRegistry
                               storage:(void *)storage
{
    self = [super init];

    if (self) {
        _objectRegistry = objectRegistry;
        _storage = storage;
    }

    return self;
}

- (instancetype)initWithFieldValues:(const void *)start, ...
{
    self = [super init];

    if (self) {
        _objectRegistry = SCValdiMarshallableObjectRegistryGetSharedInstance();
        va_list v;
        va_start(v, start);
        _storage = [_objectRegistry allocateStorageForClass:[self class] fieldValues:v];
        va_end(v);
    }

    return self;
}

- (void)dealloc
{
    [_objectRegistry deallocateStorage:_storage forClass:[self class]];
}

- (id)copyWithZone:(NSZone *)zone
{
    return self;
}

- (NSInteger)pushToValdiMarshaller:(SCValdiMarshallerRef)marshaller
{
    return [_objectRegistry marshallObject:self toMarshaller:marshaller];
}

- (BOOL)isEqual:(id)object
{
    if (object == self) {
        return YES;
    }

    Class cls = [self class];
    if (cls != [object class]) {
        return NO;
    }

    return [_objectRegistry object:self equalsToObject:object forClass:cls];
}

- (NSString *)description
{
    return SCValdiMarshallableToString(self);
}

+ (instancetype)objectFromDictionary:(NSDictionary<NSString *, id> *)dictionary
{
    id instance;
    SCValdiMarshallerScoped(marshaller, {
        NSInteger objectIndex = SCValdiMarshallerPushUntyped(marshaller, dictionary);
        instance = SCValdiMarshallableObjectUnmarshall(marshaller, objectIndex, self);
    });

    return instance;
}

+ (SCValdiMarshallableObjectDescriptor)valdiMarshallableObjectDescriptor
{
    static SCValdiMarshallableObjectFieldDescriptor kEmptyDescriptor[] = {
        {.name = nil},
    };

    return SCValdiMarshallableObjectDescriptorMake(kEmptyDescriptor, nil, nil, NO);
}

@end

NSInteger SCValdiMarshallableObjectMarshall(SCValdiMarshallerRef marshaller,
                                               __unsafe_unretained SCValdiMarshallableObject *marshallableObject)
{
    return [SCValdiMarshallableObjectRegistryGetSharedInstance() marshallObject:marshallableObject toMarshaller:marshaller];
}

id SCValdiMarshallableObjectUnmarshall(SCValdiMarshallerRef  _Nonnull marshaller,
                                                                               NSInteger objectIndex,
                                                                               Class objectClass)
{
    return [SCValdiMarshallableObjectRegistryGetSharedInstance() unmarshallObjectOfClass:objectClass fromMarshaller:marshaller atIndex:objectIndex];
}

NSInteger SCValdiMarshallableObjectMarshallInterface(SCValdiMarshallerRef _Nonnull marshaller,
                                                               id instance,
                                                               Class objectClass)
{
    return [SCValdiMarshallableObjectRegistryGetSharedInstance() marshallObject:instance ofClass:objectClass toMarshaller:marshaller];
}

@implementation SCValdiProxyMarshallableObject

- (BOOL)isEqual:(id)object
{
    return self == object;
}

@end

@implementation SCValdiMarshallableGenericObject

- (NSInteger)pushToValdiMarshaller:(SCValdiMarshallerRef)marshaller {
    return SCValdiMarshallerPushString(marshaller, [NSString stringWithFormat:@"<%@>", [self class]]);
}

@end
