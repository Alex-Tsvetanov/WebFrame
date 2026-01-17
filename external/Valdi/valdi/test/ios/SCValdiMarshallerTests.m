#import <Foundation/Foundation.h>
#import <XCTest/XCTest.h>

#import "valdi_core/SCValdiMarshallableObjectRegistry.h"
#import "valdi_core/SCValdiMarshaller.h"
#import "valdi_core/SCValdiMarshallableObject.h"
#import "valdi_core/SCValdiFunction.h"
#import "valdi_core/SCValdiBridgeFunction.h"
#import "valdi_core/SCValdiFunctionWithBlock.h"
#import "valdi_core/SCValdiUndefinedValue.h"
#import "valdi_core/SCValdiError.h"
#import "valdi_core/SCValdiMarshallableObjectUtils.h"
#import "valdi_core/SCValdiPromise.h"
#import "valdi_core/SCValdiResolvablePromise.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
#pragma clang diagnostic ignored "-Wgnu-statement-expression"

@class SCValdiMarshallerTestGenericObject<T1,T2>;
@class SCValdiMarshallerTestGenericArrays<T1,T2>;

@interface SCValdiMarshallerTestChildObject: SCValdiMarshallableObject

@property (nonatomic, assign) double childDouble;
@property (nonatomic, copy) NSArray<NSString *> *stringArray;

@end

typedef enum : NSInteger {
    SCValdiTestEnumValueA = 42,
    SCValdiTestEnumValueB = 84,
} SCValdiTestEnum;

VALDI_INT_ENUM(SCValdiTestEnum, SCValdiTestEnumValueA, SCValdiTestEnumValueB)

NSString * SCValdiTestStringEnumValueA = @"ValueA";
NSString * SCValdiTestStringEnumValueB = @"ValueB";

VALDI_STRING_ENUM(SCValdiTestStringEnum, SCValdiTestStringEnumValueA, SCValdiTestStringEnumValueB)

typedef NSString *(^SCValdiMarshallerTestBlock)(NSString *str, double i);
typedef void(^SCValdiMarshallerBuiltinBlock)(NSString *str);
typedef NSString *(^SCValdiMarshallerTestGenericObjectParameterBlock)(SCValdiMarshallerTestGenericObject *genericObject);
typedef SCValdiMarshallerTestGenericObject *(^SCValdiMarshallerTestGenericObjectReturningBlock)(NSString *str, double i);

typedef double(^SCValdiMarshallerMethodProxy)(NSString *str, BOOL b);

@interface SCValdiMarshallerTestObject: SCValdiMarshallableObject

@property (nonatomic, assign) double aDouble;
@property (nonatomic, strong) NSNumber *anOptionalDouble;

@property (nonatomic, assign) int64_t aLong;
@property (nonatomic, strong) NSNumber *anOptionalLong;

@property (nonatomic, assign) BOOL aBool;
@property (nonatomic, strong) NSNumber *anOptionalBool;

@property (nonatomic, strong) NSString *aString;

@property (nonatomic, strong) NSData *bytes;

@property (nonatomic, strong) NSDictionary *aMap;

@property (nonatomic, strong) NSArray<NSString *> *anArray;
@property (nonatomic, strong) SCValdiMarshallerTestChildObject *childObject;

@property (nonatomic, copy) SCValdiMarshallerTestBlock block;
@property (nonatomic, copy) SCValdiMarshallerBuiltinBlock builtinBlock;

@property (nonatomic, copy) SCValdiMarshallerTestGenericObjectParameterBlock genericObjectParameterBlock;
@property (nonatomic, copy) SCValdiMarshallerTestGenericObjectReturningBlock genericObjectReturningBlock;

@property (nonatomic, assign) SCValdiTestEnum intEnum;
@property (nonatomic, strong) NSNumber *optionalIntEnum;
@property (nonatomic, copy) NSString *stringEnum;

@end

@implementation SCValdiMarshallerTestObject

@dynamic aDouble;
@dynamic anOptionalDouble;
@dynamic aLong;
@dynamic anOptionalLong;
@dynamic aBool;
@dynamic anOptionalBool;
@dynamic aString;
@dynamic bytes;
@dynamic aMap;
@dynamic anArray;
@dynamic childObject;
@dynamic block;
@dynamic builtinBlock;
@dynamic genericObjectParameterBlock;
@dynamic genericObjectReturningBlock;
@dynamic intEnum;
@dynamic optionalIntEnum;
@dynamic stringEnum;

@end

@protocol SCValdiMarshallerTestBridgeObject <NSObject>

- (double)submitString:(NSString *)str andBool:(BOOL)b;

@end

@interface SCValdiMarshallerTestBridgeObject: SCValdiProxyMarshallableObject

@end

@implementation SCValdiMarshallerTestBridgeObject

@end

@interface SCValdiMarshallerTestBridgeObjectImpl: NSObject<SCValdiMarshallerTestBridgeObject>

@property (nonatomic, copy) SCValdiMarshallerMethodProxy methodProxy;

@end

@implementation SCValdiMarshallerTestBridgeObjectImpl

- (double)submitString:(NSString *)str andBool:(BOOL)b
{
    return _methodProxy(str, b);
}

@end

@implementation SCValdiMarshallerTestChildObject

@dynamic childDouble;
@dynamic stringArray;

@end

@interface SCValdiMarshallerTestBox<__covariant T>: SCValdiMarshallableObject

@property (nonatomic, strong) T value;

@end

@implementation SCValdiMarshallerTestBox

@dynamic value;

@end

@interface SCValdiMarshallerTestBoxes<__covariant T>: SCValdiMarshallableObject

@property (nonatomic, strong) SCValdiMarshallerTestBox<T> *childBox1;
@property (nonatomic, strong) SCValdiMarshallerTestBox<T> *childBox2;

@end

@implementation SCValdiMarshallerTestBoxes

@dynamic childBox1;
@dynamic childBox2;

@end

@interface SCValdiMarshallerTestBoxesParent: SCValdiMarshallableObject

@property (nonatomic, strong) SCValdiMarshallerTestBoxes *genericBoxes;

@end

@implementation SCValdiMarshallerTestBoxesParent

@dynamic genericBoxes;

@end

@interface SCValdiMarshallerTestGenericArray<__covariant T1, __covariant T2>: SCValdiMarshallableObject

@property (nonatomic, strong) NSArray<SCValdiMarshallerTestBox<T1> *> * childArray1;
@property (nonatomic, strong) NSArray<SCValdiMarshallerTestBox<T2> *> * childArray2;

@end

@implementation SCValdiMarshallerTestGenericArray

@dynamic childArray1;
@dynamic childArray2;

@end

@interface SCValdiMarshallerTestGenericArrayParent: SCValdiMarshallableObject

@property (nonatomic, strong) SCValdiMarshallerTestGenericArray *genericArray;

@end

@implementation SCValdiMarshallerTestGenericArrayParent

@dynamic genericArray;

@end

@interface SCValdiMarshallerTestGenericObject<__covariant T1, __covariant T2>: SCValdiMarshallableObject

@property (nonatomic, strong) T1 childObject1;
@property (nonatomic, strong) T2 childObject2;

@end

@implementation SCValdiMarshallerTestGenericObject

@dynamic childObject1;
@dynamic childObject2;

@end

@interface SCValdiMarshallerTestGenericObjectParent: SCValdiMarshallableObject

@property (nonatomic, strong) SCValdiMarshallerTestGenericObject *genericObject;

@end

@implementation SCValdiMarshallerTestGenericObjectParent

@dynamic genericObject;

@end

@interface SCValdiMarshallerTestBridgeFunction: SCValdiBridgeFunction

@end

@implementation SCValdiMarshallerTestBridgeFunction: SCValdiBridgeFunction

- (NSString *)getMeAString
{
    NSString * (^callBlock)() = SCValdiFieldValueGetObject(SCValdiGetMarshallableObjectFieldsStorage(self)[1]);
    return callBlock();
}

@end

@interface SCValdiMarshallerTestPromise: SCValdiMarshallableObject

@property (strong, nonatomic) SCValdiPromise<NSString *> *promise;
@property (strong, nonatomic) SCValdiPromise<SCValdiUndefinedValue *> *voidPromise;

@end

@implementation SCValdiMarshallerTestPromise

@dynamic promise;

+ (SCValdiMarshallableObjectDescriptor)valdiMarshallableObjectDescriptor
{
    static SCValdiMarshallableObjectFieldDescriptor kFields[] = {
        {.name = "promise", .selName = nil, .type = "p<s>"},
        {.name = "voidPromise", .selName = nil, .type = "p<v>"},
        {.name = nil}
    };

    return SCValdiMarshallableObjectDescriptorMake(&kFields[0], nil, nil, SCValdiMarshallableObjectTypeClass);
}

@end

@interface SCValdiMarshallerTests: XCTestCase

@end

@implementation SCValdiMarshallerTests

static void SCValdiMarshallerRegisterTestBox(SCValdiMarshallableObjectRegistry *registry) {
    SCValdiMarshallableObjectFieldDescriptor genericFields[] = {
        {.name = "value", .selName = nil, .type = "r:0"},
        {.name = nil}
    };
    [registry registerClass:[SCValdiMarshallerTestBox class] objectDescriptor:SCValdiMarshallableObjectDescriptorMake(genericFields, nil, nil, SCValdiMarshallableObjectTypeClass)];
}

static void SCValdiMarshallerRegisterTestBoxes(SCValdiMarshallableObjectRegistry *registry) {
    const char *identifiers[] = {
        "SCValdiMarshallerTestBox",
        nil
    };
    SCValdiMarshallableObjectFieldDescriptor genericFields[] = {
        {.name = "childBox1", .selName = nil, .type = "g:'[0]'<r: 0>"},
        {.name = "childBox2", .selName = nil, .type = "g:'[0]'<r: 1>"},
        {.name = nil}
    };
    [registry registerClass:[SCValdiMarshallerTestBoxes class] objectDescriptor:SCValdiMarshallableObjectDescriptorMake(genericFields, identifiers, nil, SCValdiMarshallableObjectTypeClass)];
}

static void SCValdiMarshallerRegisterTestGenericArray(SCValdiMarshallableObjectRegistry *registry) {
    const char *identifiers[] = {
        "SCValdiMarshallerTestBox",
        nil
    };
    SCValdiMarshallableObjectFieldDescriptor genericFields[] = {
        {.name = "childArray1", .selName = nil, .type = "a<g:'[0]'<r:0>>"},
        {.name = "childArray2", .selName = nil, .type = "a<g:'[0]'<r:1>>"},
        {.name = nil}
    };
    [registry registerClass:[SCValdiMarshallerTestGenericArray class] objectDescriptor:SCValdiMarshallableObjectDescriptorMake(genericFields, identifiers, nil, SCValdiMarshallableObjectTypeClass)];
}

static void SCValdiMarshallerRegisterTestGenericObject(SCValdiMarshallableObjectRegistry *registry) {
    SCValdiMarshallableObjectFieldDescriptor genericFields[] = {
        {.name = "childObject1", .selName = nil, .type = "r:0"},
        {.name = "childObject2", .selName = nil, .type = "r:1"},
        {.name = nil}
    };
    [registry registerClass:[SCValdiMarshallerTestGenericObject class] objectDescriptor:SCValdiMarshallableObjectDescriptorMake(genericFields, nil, nil, SCValdiMarshallableObjectTypeClass)];
}

- (NSString *)_makeHeapAllocatedString
{
    NSMutableString *mut = [NSMutableString new];
    [mut appendString:@"Hello this is a big string"];
    return [mut copy];
}

- (void)setUp
{
    [super setUp];

    self.continueAfterFailure = NO;
}

- (void)_getObjectRegistry:(void(^)(SCValdiMarshallableObjectRegistry *registry))cb
{
    __weak SCValdiMarshallableObjectRegistry *weakRegistry;
    @autoreleasepool {
        SCValdiMarshallableObjectRegistry *registry = [SCValdiMarshallableObjectRegistry new];
        weakRegistry = registry;
        cb(registry);
    }

    XCTAssertNil(weakRegistry);
}

- (void)testCanMarshallDouble
{
    [self _getObjectRegistry:^(SCValdiMarshallableObjectRegistry *registry) {
        SCValdiMarshallableObjectFieldDescriptor fields[] = {
            {.name = "aDouble", .selName = nil, .type = "d"},
            {.name = nil}
        };

        [registry registerClass:[SCValdiMarshallerTestObject class] objectDescriptor:SCValdiMarshallableObjectDescriptorMake(fields, nil, nil, SCValdiMarshallableObjectTypeClass)];

        SCValdiMarshallerTestObject *testObject = [registry makeObjectOfClass:[SCValdiMarshallerTestObject class]];
        testObject.aDouble = 42.0;

        NSString *result;
        SCValdiMarshallerScoped(marshaller, {
            NSInteger objectIndex = [registry marshallObject:testObject ofClass:[SCValdiMarshallerTestObject class] toMarshaller:marshaller];
            result = SCValdiMarshallerToString(marshaller, objectIndex, NO);
        });

        XCTAssertEqualObjects(@"<typedObject SCValdiMarshallerTestObject: { aDouble: 42.000000} >", result);
    }];
}

- (void)testCanUnmarshallDouble
{
    [self _getObjectRegistry:^(SCValdiMarshallableObjectRegistry *registry) {

        SCValdiMarshallableObjectFieldDescriptor fields[] = {
            {.name = "aDouble", .selName = nil, .type = "d"},
            {.name = nil}
        };

        [registry registerClass:[SCValdiMarshallerTestObject class] objectDescriptor:SCValdiMarshallableObjectDescriptorMake(fields, nil, nil, SCValdiMarshallableObjectTypeClass)];

        SCValdiMarshallerTestObject *object;

        SCValdiMarshallerScoped(marshaller, {
            NSInteger idx = SCValdiMarshallerPushMap(marshaller, 1);
            SCValdiMarshallerPushDouble(marshaller, 42.0);
            SCValdiMarshallerPutMapPropertyUninterned(marshaller, @"aDouble", idx);

            object = [registry unmarshallObjectOfClass:[SCValdiMarshallerTestObject class] fromMarshaller:marshaller atIndex:idx];
        });

        XCTAssertTrue([object isKindOfClass:[SCValdiMarshallerTestObject class]]);
        XCTAssertEqual(42.0, object.aDouble);
    }];
}

- (void)testCanMarshallBool
{
    [self _getObjectRegistry:^(SCValdiMarshallableObjectRegistry *registry) {

        SCValdiMarshallableObjectFieldDescriptor fields[] = {
            {.name = "aBool", .selName = nil, .type = "b"},
            {.name = nil}
        };

        [registry registerClass:[SCValdiMarshallerTestObject class] objectDescriptor:SCValdiMarshallableObjectDescriptorMake(fields, nil, nil, SCValdiMarshallableObjectTypeClass)];

        SCValdiMarshallerTestObject *testObject = [registry makeObjectOfClass:[SCValdiMarshallerTestObject class]];
        testObject.aBool = YES;

        NSString *result;
        SCValdiMarshallerScoped(marshaller, {
            NSInteger objectIndex = [registry marshallObject:testObject ofClass:[SCValdiMarshallerTestObject class] toMarshaller:marshaller];
            result = SCValdiMarshallerToString(marshaller, objectIndex, NO);
        });

        XCTAssertEqualObjects(@"<typedObject SCValdiMarshallerTestObject: { aBool: true} >", result);
    }];
}

- (void)testCanUnmarshallBool
{
    [self _getObjectRegistry:^(SCValdiMarshallableObjectRegistry *registry) {

        SCValdiMarshallableObjectFieldDescriptor fields[] = {
            {.name = "aBool", .selName = nil, .type = "b"},
            {.name = nil}
        };

        [registry registerClass:[SCValdiMarshallerTestObject class] objectDescriptor:SCValdiMarshallableObjectDescriptorMake(fields, nil, nil, SCValdiMarshallableObjectTypeClass)];

        SCValdiMarshallerTestObject *object;

        SCValdiMarshallerScoped(marshaller, {
            NSInteger idx = SCValdiMarshallerPushMap(marshaller, 1);
            SCValdiMarshallerPushBool(marshaller, YES);
            SCValdiMarshallerPutMapPropertyUninterned(marshaller, @"aBool", idx);

            object = [registry unmarshallObjectOfClass:[SCValdiMarshallerTestObject class] fromMarshaller:marshaller atIndex:idx];
        });

        XCTAssertTrue([object isKindOfClass:[SCValdiMarshallerTestObject class]]);
        XCTAssertEqual(YES, object.aBool);
    }];
}

- (void)testCanMarshallLong
{
    [self _getObjectRegistry:^(SCValdiMarshallableObjectRegistry *registry) {

        SCValdiMarshallableObjectFieldDescriptor fields[] = {
            {.name = "aLong", .selName = nil, .type = "l"},
            {.name = nil}
        };

        [registry registerClass:[SCValdiMarshallerTestObject class] objectDescriptor:SCValdiMarshallableObjectDescriptorMake(fields, nil, nil, SCValdiMarshallableObjectTypeClass)];

        SCValdiMarshallerTestObject *testObject = [registry makeObjectOfClass:[SCValdiMarshallerTestObject class]];
        testObject.aLong = 13371337424242;

        NSString *result;
        SCValdiMarshallerScoped(marshaller, {
            NSInteger objectIndex = [registry marshallObject:testObject ofClass:[SCValdiMarshallerTestObject class] toMarshaller:marshaller];
            result = SCValdiMarshallerToString(marshaller, objectIndex, NO);
        });

        XCTAssertEqualObjects(@"<typedObject SCValdiMarshallerTestObject: { aLong: 13371337424242} >", result);
    }];
}

- (void)testCanUnmarshallLong
{
    [self _getObjectRegistry:^(SCValdiMarshallableObjectRegistry *registry) {

        SCValdiMarshallableObjectFieldDescriptor fields[] = {
            {.name = "aLong", .selName = nil, .type = "l"},
            {.name = nil}
        };

        [registry registerClass:[SCValdiMarshallerTestObject class] objectDescriptor:SCValdiMarshallableObjectDescriptorMake(fields, nil, nil, SCValdiMarshallableObjectTypeClass)];

        SCValdiMarshallerTestObject *object;

        SCValdiMarshallerScoped(marshaller, {
            NSInteger idx = SCValdiMarshallerPushMap(marshaller, 1);
            SCValdiMarshallerPushLong(marshaller, 13371337424242);
            SCValdiMarshallerPutMapPropertyUninterned(marshaller, @"aLong", idx);

            object = [registry unmarshallObjectOfClass:[SCValdiMarshallerTestObject class] fromMarshaller:marshaller atIndex:idx];
        });

        XCTAssertTrue([object isKindOfClass:[SCValdiMarshallerTestObject class]]);
        XCTAssertEqual(13371337424242, object.aLong);
    }];
}

- (void)testCanMarshallOptionalDouble
{
    [self _getObjectRegistry:^(SCValdiMarshallableObjectRegistry *registry) {

        SCValdiMarshallableObjectFieldDescriptor fields[] = {
            {.name = "anOptionalDouble", .selName = nil, .type = "d@?"},
            {.name = nil}
        };

        [registry registerClass:[SCValdiMarshallerTestObject class] objectDescriptor:SCValdiMarshallableObjectDescriptorMake(fields, nil, nil, SCValdiMarshallableObjectTypeClass)];

        SCValdiMarshallerTestObject *testObject = [registry makeObjectOfClass:[SCValdiMarshallerTestObject class]];
        testObject.anOptionalDouble = @(42.0);

        NSString *result;
        SCValdiMarshallerScoped(marshaller, {
            NSInteger objectIndex = [registry marshallObject:testObject ofClass:[SCValdiMarshallerTestObject class] toMarshaller:marshaller];
            result = SCValdiMarshallerToString(marshaller, objectIndex, NO);
        });

        XCTAssertEqualObjects(@"<typedObject SCValdiMarshallerTestObject: { anOptionalDouble: 42.000000} >", result);

        testObject.anOptionalDouble = nil;

        SCValdiMarshallerScoped(marshaller2, {
            NSInteger objectIndex = [registry marshallObject:testObject ofClass:[SCValdiMarshallerTestObject class] toMarshaller:marshaller2];
            result = SCValdiMarshallerToString(marshaller2, objectIndex, NO);
        });

        XCTAssertEqualObjects(@"<typedObject SCValdiMarshallerTestObject: { } >", result);
    }];
}

- (void)testCanUnmarshallOptionalDouble
{
    [self _getObjectRegistry:^(SCValdiMarshallableObjectRegistry *registry) {

        SCValdiMarshallableObjectFieldDescriptor fields[] = {
            {.name = "anOptionalDouble", .selName = nil, .type = "d@?"},
            {.name = nil}
        };

        [registry registerClass:[SCValdiMarshallerTestObject class] objectDescriptor:SCValdiMarshallableObjectDescriptorMake(fields, nil, nil, SCValdiMarshallableObjectTypeClass)];

        SCValdiMarshallerTestObject *object;

        SCValdiMarshallerScoped(marshaller, {
            NSInteger idx = SCValdiMarshallerPushMap(marshaller, 1);
            SCValdiMarshallerPushDouble(marshaller, 42.0);
            SCValdiMarshallerPutMapPropertyUninterned(marshaller, @"anOptionalDouble", idx);

            object = [registry unmarshallObjectOfClass:[SCValdiMarshallerTestObject class] fromMarshaller:marshaller atIndex:idx];
        });

        XCTAssertTrue([object isKindOfClass:[SCValdiMarshallerTestObject class]]);
        XCTAssertEqualObjects(@42.0, object.anOptionalDouble);

        SCValdiMarshallerScoped(marshaller2, {
            NSInteger idx = SCValdiMarshallerPushMap(marshaller2, 1);
            SCValdiMarshallerPushUndefined(marshaller2);
            SCValdiMarshallerPutMapPropertyUninterned(marshaller2, @"anOptionalDouble", idx);

            object = [registry unmarshallObjectOfClass:[SCValdiMarshallerTestObject class] fromMarshaller:marshaller2 atIndex:idx];
        });

        XCTAssertTrue([object isKindOfClass:[SCValdiMarshallerTestObject class]]);
        XCTAssertNil(object.anOptionalDouble);
    }];
}

- (void)testCanMarshallAnUnimplementedOptionalProperty
{
        [self _getObjectRegistry:^(SCValdiMarshallableObjectRegistry *registry) {

        SCValdiMarshallableObjectFieldDescriptor fields[] = {
            {.name = "anOptionalBool", .selName = nil, .type = "b@?"},
            {.name = nil}
        };

        [registry registerClass:[SCValdiMarshallerTestObject class] objectDescriptor:SCValdiMarshallableObjectDescriptorMake(fields, nil, nil, SCValdiMarshallableObjectTypeClass)];

        SCValdiMarshallerTestObject *testObject = [registry makeObjectOfClass:[SCValdiMarshallerTestObject class]];
        testObject.anOptionalBool = @(YES);

        NSString *result;
        SCValdiMarshallerScoped(marshaller, {
            NSInteger objectIndex = [registry marshallObject:testObject ofClass:[SCValdiMarshallerTestObject class] toMarshaller:marshaller];
            result = SCValdiMarshallerToString(marshaller, objectIndex, NO);
        });

        XCTAssertEqualObjects(@"<typedObject SCValdiMarshallerTestObject: { anOptionalBool: true} >", result);

        testObject.anOptionalBool = nil;

        SCValdiMarshallerScoped(marshaller2, {
            NSInteger objectIndex = [registry marshallObject:testObject ofClass:[SCValdiMarshallerTestObject class] toMarshaller:marshaller2];
            result = SCValdiMarshallerToString(marshaller2, objectIndex, NO);
        });

        XCTAssertEqualObjects(@"<typedObject SCValdiMarshallerTestObject: { } >", result);
    }];
}

- (void)testCanUnmarshallOptionalBool
{
    [self _getObjectRegistry:^(SCValdiMarshallableObjectRegistry *registry) {

        SCValdiMarshallableObjectFieldDescriptor fields[] = {
            {.name = "anOptionalBool", .selName = nil, .type = "b@?"},
            {.name = nil}
        };

        [registry registerClass:[SCValdiMarshallerTestObject class] objectDescriptor:SCValdiMarshallableObjectDescriptorMake(fields, nil, nil, SCValdiMarshallableObjectTypeClass)];

        SCValdiMarshallerTestObject *object;

        SCValdiMarshallerScoped(marshaller, {
            NSInteger idx = SCValdiMarshallerPushMap(marshaller, 1);
            SCValdiMarshallerPushDouble(marshaller, 42.0);
            SCValdiMarshallerPutMapPropertyUninterned(marshaller, @"anOptionalBool", idx);

            object = [registry unmarshallObjectOfClass:[SCValdiMarshallerTestObject class] fromMarshaller:marshaller atIndex:idx];
        });

        XCTAssertTrue([object isKindOfClass:[SCValdiMarshallerTestObject class]]);
        XCTAssertEqualObjects(@YES, object.anOptionalBool);

        SCValdiMarshallerScoped(marshaller2, {
            NSInteger idx = SCValdiMarshallerPushMap(marshaller2, 1);
            SCValdiMarshallerPushUndefined(marshaller2);
            SCValdiMarshallerPutMapPropertyUninterned(marshaller2, @"anOptionalBool", idx);

            object = [registry unmarshallObjectOfClass:[SCValdiMarshallerTestObject class] fromMarshaller:marshaller2 atIndex:idx];
        });

        XCTAssertTrue([object isKindOfClass:[SCValdiMarshallerTestObject class]]);
        XCTAssertNil(object.anOptionalBool);
    }];
}

- (void)testCanMarshallOptionalLong
{
    [self _getObjectRegistry:^(SCValdiMarshallableObjectRegistry *registry) {

        SCValdiMarshallableObjectFieldDescriptor fields[] = {
            {.name = "anOptionalLong", .selName = nil, .type = "l@?"},
            {.name = nil}
        };

        [registry registerClass:[SCValdiMarshallerTestObject class] objectDescriptor:SCValdiMarshallableObjectDescriptorMake(fields, nil, nil, SCValdiMarshallableObjectTypeClass)];

        SCValdiMarshallerTestObject *testObject = [registry makeObjectOfClass:[SCValdiMarshallerTestObject class]];
        testObject.anOptionalLong = @(13371337424242);

        NSString *result;
        SCValdiMarshallerScoped(marshaller, {
            NSInteger objectIndex = [registry marshallObject:testObject ofClass:[SCValdiMarshallerTestObject class] toMarshaller:marshaller];
            result = SCValdiMarshallerToString(marshaller, objectIndex, NO);
        });

        XCTAssertEqualObjects(@"<typedObject SCValdiMarshallerTestObject: { anOptionalLong: 13371337424242} >", result);

        testObject.anOptionalLong = nil;

        SCValdiMarshallerScoped(marshaller2, {
            NSInteger objectIndex = [registry marshallObject:testObject ofClass:[SCValdiMarshallerTestObject class] toMarshaller:marshaller2];
            result = SCValdiMarshallerToString(marshaller2, objectIndex, NO);
        });

        XCTAssertEqualObjects(@"<typedObject SCValdiMarshallerTestObject: { } >", result);
    }];
}

- (void)testCanUnmarshallOptionalLong
{
    [self _getObjectRegistry:^(SCValdiMarshallableObjectRegistry *registry) {

        SCValdiMarshallableObjectFieldDescriptor fields[] = {
            {.name = "anOptionalLong", .selName = nil, .type = "l@?"},
            {.name = nil}
        };

        [registry registerClass:[SCValdiMarshallerTestObject class] objectDescriptor:SCValdiMarshallableObjectDescriptorMake(fields, nil, nil, SCValdiMarshallableObjectTypeClass)];

        SCValdiMarshallerTestObject *object;

        SCValdiMarshallerScoped(marshaller, {
            NSInteger idx = SCValdiMarshallerPushMap(marshaller, 1);
            SCValdiMarshallerPushLong(marshaller, 13371337424242);
            SCValdiMarshallerPutMapPropertyUninterned(marshaller, @"anOptionalLong", idx);

            object = [registry unmarshallObjectOfClass:[SCValdiMarshallerTestObject class] fromMarshaller:marshaller atIndex:idx];
        });

        XCTAssertTrue([object isKindOfClass:[SCValdiMarshallerTestObject class]]);
        XCTAssertEqualObjects(@13371337424242, object.anOptionalLong);

        SCValdiMarshallerScoped(marshaller2, {
            NSInteger idx = SCValdiMarshallerPushMap(marshaller2, 1);
            SCValdiMarshallerPushUndefined(marshaller2);
            SCValdiMarshallerPutMapPropertyUninterned(marshaller2, @"anOptionalLong", idx);

            object = [registry unmarshallObjectOfClass:[SCValdiMarshallerTestObject class] fromMarshaller:marshaller2 atIndex:idx];
        });

        XCTAssertTrue([object isKindOfClass:[SCValdiMarshallerTestObject class]]);
        XCTAssertNil(object.anOptionalLong);
    }];
}

- (void)testCanMarshallString
{
    [self _getObjectRegistry:^(SCValdiMarshallableObjectRegistry *registry) {

        SCValdiMarshallableObjectFieldDescriptor fields[] = {
            {.name = "aString", .selName = nil, .type = "s"},
            {.name = nil}
        };

        [registry registerClass:[SCValdiMarshallerTestObject class] objectDescriptor:SCValdiMarshallableObjectDescriptorMake(fields, nil, nil, SCValdiMarshallableObjectTypeClass)];

        SCValdiMarshallerTestObject *testObject = [registry makeObjectOfClass:[SCValdiMarshallerTestObject class]];
        testObject.aString = @"Hello";

        NSString *result;
        SCValdiMarshallerScoped(marshaller, {
            NSInteger objectIndex = [registry marshallObject:testObject ofClass:[SCValdiMarshallerTestObject class] toMarshaller:marshaller];
            result = SCValdiMarshallerToString(marshaller, objectIndex, NO);
        });

        XCTAssertEqualObjects(@"<typedObject SCValdiMarshallerTestObject: { aString: Hello} >", result);
    }];
}

- (void)testCanUnmarshallString
{
    [self _getObjectRegistry:^(SCValdiMarshallableObjectRegistry *registry) {

        SCValdiMarshallableObjectFieldDescriptor fields[] = {
            {.name = "aString", .selName = nil, .type = "s"},
            {.name = nil}
        };

        [registry registerClass:[SCValdiMarshallerTestObject class] objectDescriptor:SCValdiMarshallableObjectDescriptorMake(fields, nil, nil, SCValdiMarshallableObjectTypeClass)];

        SCValdiMarshallerTestObject *object;

        SCValdiMarshallerScoped(marshaller, {
            NSInteger idx = SCValdiMarshallerPushMap(marshaller, 1);
            SCValdiMarshallerPushString(marshaller, @"Hello");
            SCValdiMarshallerPutMapPropertyUninterned(marshaller, @"aString", idx);

            object = [registry unmarshallObjectOfClass:[SCValdiMarshallerTestObject class] fromMarshaller:marshaller atIndex:idx];
        });

        XCTAssertTrue([object isKindOfClass:[SCValdiMarshallerTestObject class]]);
        XCTAssertEqualObjects(@"Hello", object.aString);
    }];
}

- (void)testCanMarshallBytes
{
    [self _getObjectRegistry:^(SCValdiMarshallableObjectRegistry *registry) {

        SCValdiMarshallableObjectFieldDescriptor fields[] = {
            {.name = "bytes", .selName = nil, .type = "t"},
            {.name = nil}
        };

        [registry registerClass:[SCValdiMarshallerTestObject class] objectDescriptor:SCValdiMarshallableObjectDescriptorMake(fields, nil, nil, SCValdiMarshallableObjectTypeClass)];

        SCValdiMarshallerTestObject *testObject = [registry makeObjectOfClass:[SCValdiMarshallerTestObject class]];
        testObject.bytes = [NSData dataWithBytesNoCopy:"hello" length:4 freeWhenDone:NO];

        NSString *result;
        SCValdiMarshallerScoped(marshaller, {
            NSInteger objectIndex = [registry marshallObject:testObject ofClass:[SCValdiMarshallerTestObject class] toMarshaller:marshaller];
            result = SCValdiMarshallerToString(marshaller, objectIndex, NO);
        });

        XCTAssertEqualObjects(@"<typedObject SCValdiMarshallerTestObject: { bytes: <TypedArray: type=Uint8Array size=4>} >", result);
    }];
}

- (void)testCanUnmarshallBytes
{
    [self _getObjectRegistry:^(SCValdiMarshallableObjectRegistry *registry) {

        SCValdiMarshallableObjectFieldDescriptor fields[] = {
            {.name = "bytes", .selName = nil, .type = "t"},
            {.name = nil}
        };

        [registry registerClass:[SCValdiMarshallerTestObject class] objectDescriptor:SCValdiMarshallableObjectDescriptorMake(fields, nil, nil, SCValdiMarshallableObjectTypeClass)];

        SCValdiMarshallerTestObject *object;

        SCValdiMarshallerScoped(marshaller, {
            NSInteger idx = SCValdiMarshallerPushMap(marshaller, 1);
            SCValdiMarshallerPushData(marshaller, [NSData dataWithBytesNoCopy:"hello" length:5 freeWhenDone:NO]);
            SCValdiMarshallerPutMapPropertyUninterned(marshaller, @"bytes", idx);

            object = [registry unmarshallObjectOfClass:[SCValdiMarshallerTestObject class] fromMarshaller:marshaller atIndex:idx];
        });

        XCTAssertTrue([object isKindOfClass:[SCValdiMarshallerTestObject class]]);
        XCTAssertNotNil(object.bytes);
        XCTAssertEqual(5, object.bytes.length);

        NSString *data = [[NSString alloc] initWithData:object.bytes encoding:NSUTF8StringEncoding];
        XCTAssertEqualObjects(@"hello", data);
    }];
}

- (void)testCanMarshallMap
{
    [self _getObjectRegistry:^(SCValdiMarshallableObjectRegistry *registry) {

        SCValdiMarshallableObjectFieldDescriptor fields[] = {
            {.name = "aMap", .selName = nil, .type = "m<s, u>"},
            {.name = nil}
        };

        [registry registerClass:[SCValdiMarshallerTestObject class] objectDescriptor:SCValdiMarshallableObjectDescriptorMake(fields, nil, nil, SCValdiMarshallableObjectTypeClass)];

        SCValdiMarshallerTestObject *testObject = [registry makeObjectOfClass:[SCValdiMarshallerTestObject class]];
        testObject.aMap = @{@"hello": @"world"};

        NSString *result;
        SCValdiMarshallerScoped(marshaller, {
            NSInteger objectIndex = [registry marshallObject:testObject ofClass:[SCValdiMarshallerTestObject class] toMarshaller:marshaller];
            result = SCValdiMarshallerToString(marshaller, objectIndex, NO);
        });

        XCTAssertEqualObjects(@"<typedObject SCValdiMarshallerTestObject: { aMap: { hello: world} } >", result);
    }];
}

- (void)testCanUnmarshallMap
{
    [self _getObjectRegistry:^(SCValdiMarshallableObjectRegistry *registry) {

        SCValdiMarshallableObjectFieldDescriptor fields[] = {
            {.name = "aMap", .selName = nil, .type = "m<s, u>"},
            {.name = nil}
        };

        [registry registerClass:[SCValdiMarshallerTestObject class] objectDescriptor:SCValdiMarshallableObjectDescriptorMake(fields, nil, nil, SCValdiMarshallableObjectTypeClass)];

        SCValdiMarshallerTestObject *object;

        SCValdiMarshallerScoped(marshaller, {
            NSInteger idx = SCValdiMarshallerPushMap(marshaller, 1);
            SCValdiMarshallerPushUntyped(marshaller, @{@"hello": @"world"});
            SCValdiMarshallerPutMapPropertyUninterned(marshaller, @"aMap", idx);

            object = [registry unmarshallObjectOfClass:[SCValdiMarshallerTestObject class] fromMarshaller:marshaller atIndex:idx];
        });

        XCTAssertTrue([object isKindOfClass:[SCValdiMarshallerTestObject class]]);
        XCTAssertNotNil(object.aMap);

        NSDictionary *expectedMap = @{@"hello": @"world"};
        XCTAssertEqualObjects(expectedMap, object.aMap);
    }];
}

- (void)testDoesntLeakObjectsDuringMarshalling
{
    [self _getObjectRegistry:^(SCValdiMarshallableObjectRegistry *registry) {

        SCValdiMarshallableObjectFieldDescriptor fields[] = {
            {.name = "aString", .selName = nil, .type = "s"},
            {.name = nil}
        };

        [registry registerClass:[SCValdiMarshallerTestObject class] objectDescriptor:SCValdiMarshallableObjectDescriptorMake(fields, nil, nil, SCValdiMarshallableObjectTypeClass)];

        NSString *str;
        @autoreleasepool {
            str = [self _makeHeapAllocatedString] ;
        }

        XCTAssertEqual(1, CFGetRetainCount((__bridge CFTypeRef)(str)));

        SCValdiMarshallerTestObject *testObject = [registry makeObjectOfClass:[SCValdiMarshallerTestObject class]];
        testObject.aString = str;

        XCTAssertEqual(2, CFGetRetainCount((__bridge CFTypeRef)(str)));

        @autoreleasepool {
            SCValdiMarshallerScoped(marshaller, {
                [registry marshallObject:testObject ofClass:[SCValdiMarshallerTestObject class] toMarshaller:marshaller];
            });
        }

        XCTAssertEqual(2, CFGetRetainCount((__bridge CFTypeRef)(str)));
    }];
}

- (void)testRetainReleaseValueOnSetter
{
    [self _getObjectRegistry:^(SCValdiMarshallableObjectRegistry *registry) {

        SCValdiMarshallableObjectFieldDescriptor fields[] = {
            {.name = "aString", .selName = nil, .type = "s"},
            {.name = nil}
        };

        [registry registerClass:[SCValdiMarshallerTestObject class] objectDescriptor:SCValdiMarshallableObjectDescriptorMake(fields, nil, nil, SCValdiMarshallableObjectTypeClass)];

        NSString *str;
        @autoreleasepool {
            str = [self _makeHeapAllocatedString];
        }

        XCTAssertEqual(1, CFGetRetainCount((__bridge CFTypeRef)(str)));

        SCValdiMarshallerTestObject *testObject = [registry makeObjectOfClass:[SCValdiMarshallerTestObject class]];
        testObject.aString = str;

        XCTAssertEqual(2, CFGetRetainCount((__bridge CFTypeRef)(str)));

        testObject.aString = nil;

        XCTAssertEqual(1, CFGetRetainCount((__bridge CFTypeRef)(str)));
    }];
}

- (void)testRespectCopySemantics
{
    [self _getObjectRegistry:^(SCValdiMarshallableObjectRegistry *registry) {

        SCValdiMarshallableObjectFieldDescriptor fields[] = {
            {.name = "aString", .selName = nil, .type = "s"},
            {.name = nil}
        };

        [registry registerClass:[SCValdiMarshallerTestObject class] objectDescriptor:SCValdiMarshallableObjectDescriptorMake(fields, nil, nil, SCValdiMarshallableObjectTypeClass)];

        NSMutableString *str = [NSMutableString new];
        [str appendString:@"This is a long string that cannot be a tagged pointer"];

        SCValdiMarshallerTestObject *testObject;
        NSString *copiedString;

        @autoreleasepool {
            testObject = [registry makeObjectOfClass:[SCValdiMarshallerTestObject class]];
            testObject.aString = str;
            copiedString = testObject.aString;
        }

        // They should be equal in content
        XCTAssertEqualObjects(str, copiedString);
        // But the string itself should have been copied and thus the actual instance should be different
        XCTAssertNotEqual(str, copiedString);

        testObject = nil;

        XCTAssertEqual(1, CFGetRetainCount((__bridge CFTypeRef)(copiedString)));
    }];
}

- (void)testReleaseRetainedValuesOnDealloc
{
    [self _getObjectRegistry:^(SCValdiMarshallableObjectRegistry *registry) {

        SCValdiMarshallableObjectFieldDescriptor fields[] = {
            {.name = "aString", .selName = nil, .type = "s"},
            {.name = nil}
        };

        [registry registerClass:[SCValdiMarshallerTestObject class] objectDescriptor:SCValdiMarshallableObjectDescriptorMake(fields, nil, nil, SCValdiMarshallableObjectTypeClass)];

        NSString *str;
        @autoreleasepool {
            str = [self _makeHeapAllocatedString];
        }

        XCTAssertEqual(1, CFGetRetainCount((__bridge CFTypeRef)(str)));

        @autoreleasepool {
            SCValdiMarshallerTestObject *testObject = [registry makeObjectOfClass:[SCValdiMarshallerTestObject class]];
            testObject.aString = str;

            XCTAssertEqual(2, CFGetRetainCount((__bridge CFTypeRef)(str)));
            testObject = nil;
        }

        XCTAssertEqual(1, CFGetRetainCount((__bridge CFTypeRef)(str)));
    }];
}

- (void)testCanMarshallArray
{
    [self _getObjectRegistry:^(SCValdiMarshallableObjectRegistry *registry) {

        SCValdiMarshallableObjectFieldDescriptor fields[] = {
            {.name = "anArray", .selName = nil, .type = "a<s>"},
            {.name = nil}
        };

        [registry registerClass:[SCValdiMarshallerTestObject class] objectDescriptor:SCValdiMarshallableObjectDescriptorMake(fields, nil, nil, SCValdiMarshallableObjectTypeClass)];

        SCValdiMarshallerTestObject *testObject = [registry makeObjectOfClass:[SCValdiMarshallerTestObject class]];
        testObject.anArray = @[@"Hello", @"World"];

        NSString *result;
        SCValdiMarshallerScoped(marshaller, {
            NSInteger objectIndex = [registry marshallObject:testObject ofClass:[SCValdiMarshallerTestObject class] toMarshaller:marshaller];
            result = SCValdiMarshallerToString(marshaller, objectIndex, NO);
        });

        XCTAssertEqualObjects(@"<typedObject SCValdiMarshallerTestObject: { anArray: [ Hello, World ]} >", result);
    }];
}

- (void)testCanUnmarshallArray
{
    [self _getObjectRegistry:^(SCValdiMarshallableObjectRegistry *registry) {

        SCValdiMarshallableObjectFieldDescriptor fields[] = {
            {.name = "anArray", .selName = nil, .type = "a<s>"},
            {.name = nil}
        };

        [registry registerClass:[SCValdiMarshallerTestObject class] objectDescriptor:SCValdiMarshallableObjectDescriptorMake(fields, nil, nil, SCValdiMarshallableObjectTypeClass)];

        SCValdiMarshallerTestObject *object;

        SCValdiMarshallerScoped(marshaller, {
            NSInteger idx = SCValdiMarshallerPushMap(marshaller, 1);
            NSInteger arrayIndex = SCValdiMarshallerPushArray(marshaller, 2);

            SCValdiMarshallerPushString(marshaller, @"Hello");
            SCValdiMarshallerSetArrayItem(marshaller, arrayIndex, 0);
            SCValdiMarshallerPushString(marshaller, @"World");
            SCValdiMarshallerSetArrayItem(marshaller, arrayIndex, 1);

            SCValdiMarshallerPutMapPropertyUninterned(marshaller, @"anArray", idx);

            object = [registry unmarshallObjectOfClass:[SCValdiMarshallerTestObject class] fromMarshaller:marshaller atIndex:idx];
        });

        XCTAssertTrue([object isKindOfClass:[SCValdiMarshallerTestObject class]]);

        NSArray *expectedArray = @[@"Hello", @"World"];
        XCTAssertEqualObjects(expectedArray, object.anArray);
    }];
}

- (void)testCanMarshallArrayOfOptionals
{
    [self _getObjectRegistry:^(SCValdiMarshallableObjectRegistry *registry) {

        SCValdiMarshallableObjectFieldDescriptor fields[] = {
            {.name = "anArray", .selName = nil, .type = "a<s?>"},
            {.name = nil}
        };

        [registry registerClass:[SCValdiMarshallerTestObject class] objectDescriptor:SCValdiMarshallableObjectDescriptorMake(fields, nil, nil, SCValdiMarshallableObjectTypeClass)];

        SCValdiMarshallerTestObject *testObject = [registry makeObjectOfClass:[SCValdiMarshallerTestObject class]];
        testObject.anArray = @[@"Hello", [NSNull null],  @"World"];

        NSString *result;
        SCValdiMarshallerScoped(marshaller, {
            NSInteger objectIndex = [registry marshallObject:testObject ofClass:[SCValdiMarshallerTestObject class] toMarshaller:marshaller];
            result = SCValdiMarshallerToString(marshaller, objectIndex, NO);
        });

        XCTAssertEqualObjects(@"<typedObject SCValdiMarshallerTestObject: { anArray: [ Hello, <undefined>, World ]} >", result);
    }];
}

- (void)testCanUnmarshallArrayOfOptional
{
    [self _getObjectRegistry:^(SCValdiMarshallableObjectRegistry *registry) {

        SCValdiMarshallableObjectFieldDescriptor fields[] = {
            {.name = "anArray", .selName = nil, .type = "a<s?>"},
            {.name = nil}
        };

        [registry registerClass:[SCValdiMarshallerTestObject class] objectDescriptor:SCValdiMarshallableObjectDescriptorMake(fields, nil, nil, SCValdiMarshallableObjectTypeClass)];

        SCValdiMarshallerTestObject *object;

        SCValdiMarshallerScoped(marshaller, {
            NSInteger idx = SCValdiMarshallerPushMap(marshaller, 1);
            NSInteger arrayIndex = SCValdiMarshallerPushArray(marshaller, 3);

            SCValdiMarshallerPushString(marshaller, @"Hello");
            SCValdiMarshallerSetArrayItem(marshaller, arrayIndex, 0);
            SCValdiMarshallerPushUndefined(marshaller);
            SCValdiMarshallerSetArrayItem(marshaller, arrayIndex, 1);
            SCValdiMarshallerPushString(marshaller, @"World");
            SCValdiMarshallerSetArrayItem(marshaller, arrayIndex, 2);

            SCValdiMarshallerPutMapPropertyUninterned(marshaller, @"anArray", idx);

            object = [registry unmarshallObjectOfClass:[SCValdiMarshallerTestObject class] fromMarshaller:marshaller atIndex:idx];
        });

        XCTAssertTrue([object isKindOfClass:[SCValdiMarshallerTestObject class]]);

        NSArray *expectedArray = @[@"Hello", [NSNull null], @"World"];
        XCTAssertEqualObjects(expectedArray, object.anArray);
    }];
}

- (void)testCanMarshallChildObject
{
    [self _getObjectRegistry:^(SCValdiMarshallableObjectRegistry *registry) {

        const char *identifiers[] = {"SCValdiMarshallerTestChildObject", nil};

        SCValdiMarshallableObjectFieldDescriptor fields[] = {
            {.name = "childObject", .selName = nil, .type = "r:'[0]'"},
            {.name = nil}
        };
        SCValdiMarshallableObjectFieldDescriptor childFields[] = {
            {.name = "childDouble", .selName = nil, .type = "d"},
            {.name = nil}
        };

        [registry registerClass:[SCValdiMarshallerTestObject class] objectDescriptor:SCValdiMarshallableObjectDescriptorMake(fields, identifiers, nil, SCValdiMarshallableObjectTypeClass)];
        [registry registerClass:[SCValdiMarshallerTestChildObject class] objectDescriptor:SCValdiMarshallableObjectDescriptorMake(childFields, nil, nil, SCValdiMarshallableObjectTypeClass)];

        SCValdiMarshallerTestObject *testObject = [registry makeObjectOfClass:[SCValdiMarshallerTestObject class]];
        testObject.childObject = [registry makeObjectOfClass:[SCValdiMarshallerTestChildObject class]];
        testObject.childObject.childDouble = 42.0;

        NSString *result;
        SCValdiMarshallerScoped(marshaller, {
            NSInteger objectIndex = [registry marshallObject:testObject ofClass:[SCValdiMarshallerTestObject class] toMarshaller:marshaller];
            result = SCValdiMarshallerToString(marshaller, objectIndex, NO);
        });

        XCTAssertEqualObjects(@"<typedObject SCValdiMarshallerTestObject: { childObject: <typedObject SCValdiMarshallerTestChildObject: { childDouble: 42.000000} >} >", result);
    }];
}

- (void)testCanUnmarshallChildObject
{
    [self _getObjectRegistry:^(SCValdiMarshallableObjectRegistry *registry) {

        const char *identifiers[] = {"SCValdiMarshallerTestChildObject", nil};

        SCValdiMarshallableObjectFieldDescriptor fields[] = {
            {.name = "childObject", .selName = nil, .type = "r:'[0]'"},
            {.name = nil}
        };
        SCValdiMarshallableObjectFieldDescriptor childFields[] = {
            {.name = "childDouble", .selName = nil, .type = "d"},
            {.name = nil}
        };

        [registry registerClass:[SCValdiMarshallerTestObject class] objectDescriptor:SCValdiMarshallableObjectDescriptorMake(fields, identifiers, nil, SCValdiMarshallableObjectTypeClass)];
        [registry registerClass:[SCValdiMarshallerTestChildObject class] objectDescriptor:SCValdiMarshallableObjectDescriptorMake(childFields, nil, nil, SCValdiMarshallableObjectTypeClass)];

        SCValdiMarshallerTestObject *object;

        SCValdiMarshallerScoped(marshaller, {
            NSInteger idx = SCValdiMarshallerPushMap(marshaller, 1);

            NSInteger childIndex = SCValdiMarshallerPushMap(marshaller, 1);

            SCValdiMarshallerPushDouble(marshaller, 42.0);
            SCValdiMarshallerPutMapPropertyUninterned(marshaller, @"childDouble", childIndex);

            SCValdiMarshallerPutMapPropertyUninterned(marshaller, @"childObject", idx);

            object = [registry unmarshallObjectOfClass:[SCValdiMarshallerTestObject class] fromMarshaller:marshaller atIndex:idx];
        });

        XCTAssertTrue([object isKindOfClass:[SCValdiMarshallerTestObject class]]);
        XCTAssertTrue([object.childObject isKindOfClass:[SCValdiMarshallerTestChildObject class]]);
        XCTAssertEqual(42.0, object.childObject.childDouble);
    }];
}

- (void)testImplementsEqualsTo
{
    [self _getObjectRegistry:^(SCValdiMarshallableObjectRegistry *registry) {

        const char *identifiers[] = {"SCValdiMarshallerTestChildObject", nil};

        SCValdiMarshallableObjectFieldDescriptor fields[] = {
            {.name = "aDouble", .selName = nil, .type = "d"},
            {.name = "anOptionalDouble", .selName = nil, .type = "d@?"},
            {.name = "aBool", .selName = nil, .type = "b"},
            {.name = "anOptionalBool", .selName = nil, .type = "b@?"},
            {.name = "bytes", .selName = nil, .type = "t"},
            {.name = "anArray", .selName = nil, .type = "a<s>"},
            {.name = "childObject", .selName = nil, .type = "r:'[0]'"},
            {.name = nil}
        };
        SCValdiMarshallableObjectFieldDescriptor childFields[] = {
            {.name = "childDouble", .selName = nil, .type = "d"},
            {.name = nil}
        };

        [registry registerClass:[SCValdiMarshallerTestObject class] objectDescriptor:SCValdiMarshallableObjectDescriptorMake(fields, identifiers, nil, SCValdiMarshallableObjectTypeClass)];
        [registry registerClass:[SCValdiMarshallerTestChildObject class] objectDescriptor:SCValdiMarshallableObjectDescriptorMake(childFields, nil, nil, SCValdiMarshallableObjectTypeClass)];

        SCValdiMarshallerTestObject *testObject = [registry makeObjectOfClass:[SCValdiMarshallerTestObject class]];
        testObject.childObject = [registry makeObjectOfClass:[SCValdiMarshallerTestChildObject class]];

        SCValdiMarshallerTestObject *otherTestObject = [registry makeObjectOfClass:[SCValdiMarshallerTestObject class]];
        otherTestObject.childObject = [registry makeObjectOfClass:[SCValdiMarshallerTestChildObject class]];

        XCTAssertEqualObjects(testObject, otherTestObject);

        testObject.aDouble = 1;
        XCTAssertNotEqualObjects(testObject, otherTestObject);

        otherTestObject.aDouble = 1;
        XCTAssertEqualObjects(testObject, otherTestObject);

        testObject.anOptionalDouble = @(1337.0);
        XCTAssertNotEqualObjects(testObject, otherTestObject);

        otherTestObject.anOptionalDouble = @(1337.0);
        XCTAssertEqualObjects(testObject, otherTestObject);

        testObject.aBool = YES;
        XCTAssertNotEqualObjects(testObject, otherTestObject);

        otherTestObject.aBool = YES;
        XCTAssertEqualObjects(testObject, otherTestObject);

        testObject.anOptionalBool = @(YES);
        XCTAssertNotEqualObjects(testObject, otherTestObject);

        otherTestObject.anOptionalBool = @(YES);
        XCTAssertEqualObjects(testObject, otherTestObject);

        testObject.bytes = [@"Hello World" dataUsingEncoding:NSUTF8StringEncoding];
        XCTAssertNotEqualObjects(testObject, otherTestObject);

        otherTestObject.bytes = [@"Hello World" dataUsingEncoding:NSUTF8StringEncoding];
        XCTAssertEqualObjects(testObject, otherTestObject);

        testObject.anArray = @[];
        XCTAssertNotEqualObjects(testObject, otherTestObject);

        otherTestObject.anArray = @[];
        XCTAssertEqualObjects(testObject, otherTestObject);

        testObject.anArray = @[@"Hello"];
        XCTAssertNotEqualObjects(testObject, otherTestObject);

        otherTestObject.anArray = @[@"Hello"];
        XCTAssertEqualObjects(testObject, otherTestObject);

        testObject.childObject.childDouble = 42;
        XCTAssertNotEqualObjects(testObject, otherTestObject);

        otherTestObject.childObject.childDouble = 42;
        XCTAssertEqualObjects(testObject, otherTestObject);
    }];
}

SCValdiFieldValue invokeBlock(const void *function, SCValdiFieldValue* fields) {
    id result = ((id(*)(const void *, const void *, double))function)(SCValdiFieldValueGetPtr(fields[0]), SCValdiFieldValueGetPtr(fields[1]), SCValdiFieldValueGetDouble(fields[2]));

    return SCValdiFieldValueMakeObject(result);
}

id createBlock(SCValdiBlockTrampoline trampoline) {
    return ^NSString *(NSString *str, double d) {
        return SCValdiFieldValueGetObject(trampoline(0, SCValdiFieldValueMakeUnretainedObject(str), SCValdiFieldValueMakeDouble(d)));
    };
}

- (void)testCanMarshallBlock
{
    [self _getObjectRegistry:^(SCValdiMarshallableObjectRegistry *registry) {

        SCValdiMarshallableObjectFieldDescriptor fields[] = {
            {.name = "block", .selName = nil, .type = "f(s, d): s"},
            {.name = nil}
        };

        SCValdiMarshallableObjectBlockSupport blockSupport[] = {
            {.typeEncoding = "ood:o", .invoker = &invokeBlock, .factory = &createBlock },
            {.typeEncoding = nil}
        };

        [registry registerClass:[SCValdiMarshallerTestObject class] objectDescriptor:SCValdiMarshallableObjectDescriptorMake(fields, nil, blockSupport, SCValdiMarshallableObjectTypeClass)];

        SCValdiMarshallerTestObject *testObject = [registry makeObjectOfClass:[SCValdiMarshallerTestObject class]];

        __block NSString *submittedStr = nil;
        __block NSNumber *submittedI = nil;
        testObject.block = ^NSString *(NSString *str, double i) {
            submittedStr = str;
            submittedI = @(i);

            return @"A Result";
        };

        NSDictionary *result;

        SCValdiMarshallerScoped(marshaller, {
            NSInteger objectIndex = [registry marshallObject:testObject toMarshaller:marshaller];
            result = SCValdiMarshallerGetUntyped(marshaller, objectIndex);
        });

        XCTAssertTrue([result isKindOfClass:[NSDictionary class]]);

        id<SCValdiFunction> function = result[@"block"];

        XCTAssertTrue([function respondsToSelector:@selector(performWithMarshaller:)]);

        XCTAssertNil(submittedStr);
        XCTAssertNil(submittedI);

        id callResult;

        SCValdiMarshallerScoped(marshaller2, {
            SCValdiMarshallerPushString(marshaller2, @"Hello");
            SCValdiMarshallerPushDouble(marshaller2, 42.0);
            if ([function performWithMarshaller:marshaller2]) {
                callResult = SCValdiMarshallerGetUntyped(marshaller2, -1);
            }
        });

        XCTAssertNotNil(submittedStr);
        XCTAssertNotNil(submittedI);

        XCTAssertEqualObjects(@"Hello", submittedStr);
        XCTAssertEqualObjects(submittedI, @(42));

        XCTAssertNotNil(callResult);
        XCTAssertEqualObjects(@"A Result", callResult);
    }];
}

- (void)testCanUnmarshallBlock
{
    [self _getObjectRegistry:^(SCValdiMarshallableObjectRegistry *registry) {

        SCValdiMarshallableObjectFieldDescriptor fields[] = {
            {.name = "block", .selName = nil, .type = "f(s, d): s"},
            {.name = nil}
        };

        SCValdiMarshallableObjectBlockSupport blockSupport[] = {
            {.typeEncoding = "ood:o", .invoker = &invokeBlock, .factory = &createBlock },
            {.typeEncoding = nil}
        };

        [registry registerClass:[SCValdiMarshallerTestObject class] objectDescriptor:SCValdiMarshallableObjectDescriptorMake(fields, nil,blockSupport, SCValdiMarshallableObjectTypeClass)];

        __block NSString *submittedStr = nil;
        __block NSNumber *submittedI = nil;
        id<SCValdiFunction> function = [SCValdiFunctionWithBlock functionWithBlock:^BOOL(SCValdiMarshallerRef  _Nonnull parameters) {
            submittedStr = SCValdiMarshallerGetString(parameters, 0);
            submittedI = SCValdiMarshallerGetOptionalDouble(parameters, 1);

            SCValdiMarshallerPushString(parameters, @"A Result");
            return YES;
        }];

        SCValdiMarshallerTestObject *object;

        SCValdiMarshallerScoped(marshaller, {
            NSInteger idx = SCValdiMarshallerPushMap(marshaller, 1);

            SCValdiMarshallerPushFunction(marshaller, function);
            SCValdiMarshallerPutMapPropertyUninterned(marshaller, @"block", idx);

            object = [registry unmarshallObjectOfClass:[SCValdiMarshallerTestObject class] fromMarshaller:marshaller atIndex:idx];
        });

        XCTAssertTrue([object isKindOfClass:[SCValdiMarshallerTestObject class]]);
        XCTAssertNotNil(object.block);

        NSString *callResult = object.block(@"Hello", 42.0);

        XCTAssertNotNil(submittedStr);
        XCTAssertNotNil(submittedI);
        XCTAssertNotNil(callResult);

        XCTAssertEqualObjects(@"Hello", submittedStr);
        XCTAssertEqualObjects(submittedI, @(42));
        XCTAssertEqualObjects(@"A Result", callResult);
    }];
}

- (void)testCanMarshallBlockWithBuiltinSupport
{
    [self _getObjectRegistry:^(SCValdiMarshallableObjectRegistry *registry) {

        SCValdiMarshallableObjectFieldDescriptor fields[] = {
            {.name = "builtinBlock", .selName = nil, .type = "f(s): v"},
            {.name = nil}
        };

        [registry registerClass:[SCValdiMarshallerTestObject class] objectDescriptor:SCValdiMarshallableObjectDescriptorMake(fields, nil, nil, SCValdiMarshallableObjectTypeClass)];

        SCValdiMarshallerTestObject *testObject = [registry makeObjectOfClass:[SCValdiMarshallerTestObject class]];

        __block NSString *submittedStr = nil;
        testObject.builtinBlock = ^(NSString *str) {
            submittedStr = str;
        };

        NSDictionary *result;

        SCValdiMarshallerScoped(marshaller, {
            NSInteger objectIndex = [registry marshallObject:testObject toMarshaller:marshaller];
            result = SCValdiMarshallerGetUntyped(marshaller, objectIndex);
        });

        XCTAssertTrue([result isKindOfClass:[NSDictionary class]]);

        id<SCValdiFunction> function = result[@"builtinBlock"];

        XCTAssertTrue([function respondsToSelector:@selector(performWithMarshaller:)]);

        XCTAssertNil(submittedStr);

        id callResult;

        SCValdiMarshallerScoped(marshaller2, {
            SCValdiMarshallerPushString(marshaller2, @"Hello");
            if ([function performWithMarshaller:marshaller2]) {
                callResult = SCValdiMarshallerGetUntyped(marshaller2, -1);
            }
        });

        XCTAssertEqual(callResult, [SCValdiUndefinedValue undefined]);

        XCTAssertNotNil(submittedStr);

        XCTAssertEqualObjects(@"Hello", submittedStr);
    }];
}

- (void)testCanUnmarshallBlockWithBuiltinSupport
{
    [self _getObjectRegistry:^(SCValdiMarshallableObjectRegistry *registry) {

        SCValdiMarshallableObjectFieldDescriptor fields[] = {
            {.name = "builtinBlock", .selName = nil, .type = "f(s): v"},
            {.name = nil}
        };

        [registry registerClass:[SCValdiMarshallerTestObject class] objectDescriptor:SCValdiMarshallableObjectDescriptorMake(fields, nil, nil, SCValdiMarshallableObjectTypeClass)];

        __block NSString *submittedStr;
        id<SCValdiFunction> function = [SCValdiFunctionWithBlock functionWithBlock:^BOOL(SCValdiMarshallerRef  _Nonnull parameters) {
            submittedStr = SCValdiMarshallerGetString(parameters, 0);
            return YES;
        }];

        SCValdiMarshallerTestObject *object;

        SCValdiMarshallerScoped(marshaller, {
            NSInteger idx = SCValdiMarshallerPushMap(marshaller, 1);

            SCValdiMarshallerPushFunction(marshaller, function);
            SCValdiMarshallerPutMapPropertyUninterned(marshaller, @"builtinBlock", idx);

            object = [registry unmarshallObjectOfClass:[SCValdiMarshallerTestObject class] fromMarshaller:marshaller atIndex:idx];
        });

        XCTAssertNil(submittedStr);

        XCTAssertTrue([object isKindOfClass:[SCValdiMarshallerTestObject class]]);
        XCTAssertNotNil(object.builtinBlock);

        object.builtinBlock(@"Hello");

        XCTAssertNotNil(submittedStr);

        XCTAssertEqualObjects(@"Hello", submittedStr);
    }];
}

- (void)testBridgedObjectGetUnwrappedOnUnmarshall
{
    [self _getObjectRegistry:^(SCValdiMarshallableObjectRegistry *registry) {

        SCValdiMarshallableObjectFieldDescriptor fields[] = {
            {.name = nil}
        };

        [registry registerClass:[SCValdiMarshallerTestObject class] objectDescriptor:SCValdiMarshallableObjectDescriptorMake(fields, nil, nil, SCValdiMarshallableObjectTypeProtocol)];

        SCValdiMarshallerTestObject *testObject = [registry makeObjectOfClass:[SCValdiMarshallerTestObject class]];

        id unmarshalledObject;
        {
            SCValdiMarshallerScoped(marshaller, {
                NSInteger objectIndex = [registry marshallObject:testObject ofClass:[SCValdiMarshallerTestObject class] toMarshaller:marshaller];
                unmarshalledObject = [registry unmarshallObjectOfClass:[SCValdiMarshallerTestObject class] fromMarshaller:marshaller atIndex:objectIndex];
            });
        }

        XCTAssertEqual(testObject, unmarshalledObject);
    }];
}

id createBridgeBlock(SCValdiBlockTrampoline trampoline) {
    return ^double (const void *p0, const void *p1, BOOL p2) {
        return SCValdiFieldValueGetDouble(trampoline(nil, SCValdiFieldValueMakePtr(p0), SCValdiFieldValueMakePtr(p1), SCValdiFieldValueMakeBool(p2)));
    };
}

SCValdiFieldValue invokeBridgeBlock(const void *function, SCValdiFieldValue* fields) {
    double result = ((double(*)(const void *, const void *, const void *, BOOL))function)(SCValdiFieldValueGetPtr(fields[0]), SCValdiFieldValueGetPtr(fields[1]), SCValdiFieldValueGetPtr(fields[2]), SCValdiFieldValueGetBool(fields[3]));

    return SCValdiFieldValueMakeDouble(result);
}

static void registerBridgeClass(SCValdiMarshallableObjectRegistry *registry) {
    SCValdiMarshallableObjectFieldDescriptor fields[] = {
        {.name = "submit", .selName = "submitString:andBool:", .type = "f*(s, b): d"},
        {.name = nil}
    };

    SCValdiMarshallableObjectBlockSupport blockSupport[] = {
        {.typeEncoding = "ooob:d", .invoker = &invokeBridgeBlock, .factory = &createBridgeBlock },
        {.typeEncoding = nil}
    };

    [registry registerClass:[SCValdiMarshallerTestBridgeObject class] objectDescriptor:SCValdiMarshallableObjectDescriptorMake(fields, nil, blockSupport, SCValdiMarshallableObjectTypeProtocol)];

}

- (void)testCanMarshallBridgedObject
{
    [self _getObjectRegistry:^(SCValdiMarshallableObjectRegistry *registry) {
        registerBridgeClass(registry);

        SCValdiMarshallerTestBridgeObjectImpl *testObject = [SCValdiMarshallerTestBridgeObjectImpl new];

        NSDictionary *result;
        {
            SCValdiMarshallerScoped(marshaller, {
                NSInteger objectIndex = [registry marshallObject:testObject ofClass:[SCValdiMarshallerTestBridgeObject class] toMarshaller:marshaller];
                SCValdiMarshallerCheck(marshaller);
                id instance = SCValdiMarshallerGetUntyped(marshaller, objectIndex);
                // Instance should be retrievable from the proxy
                XCTAssertEqual(testObject, instance);

                NSInteger typedObjectIndex = SCValdiMarshallerUnwrapProxy(marshaller, objectIndex);

                result = SCValdiMarshallerGetUntyped(marshaller, typedObjectIndex);
            });
        }

        XCTAssertTrue([result isKindOfClass:[NSDictionary class]]);

        id<SCValdiFunction> function = result[@"submit"];

        XCTAssertNotNil(function);
        XCTAssertTrue([function respondsToSelector:@selector(performWithMarshaller:)]);

        __block NSString *submittedStr = nil;
        __block BOOL submittedBool = NO;
        testObject.methodProxy = ^double(NSString *str, BOOL b) {
            submittedStr = str;
            submittedBool = b;
            return 42.0;
        };

        {
            SCValdiMarshallerScoped(marshaller, {
                SCValdiMarshallerPushString(marshaller, @"Hello");
                SCValdiMarshallerPushBool(marshaller, YES);
                BOOL didPush = [function performWithMarshaller:marshaller];

                SCValdiMarshallerCheck(marshaller);
                XCTAssertTrue(didPush);
                XCTAssertTrue(SCValdiMarshallerIsDouble(marshaller, -1));
                XCTAssertEqual(42.0, SCValdiMarshallerGetDouble(marshaller, -1));
                XCTAssertEqualObjects(@"Hello", submittedStr);
                XCTAssertTrue(submittedBool);
            });
        }
    }];
}

- (void)testCanUnmarshallBridgeObject
{
    [self _getObjectRegistry:^(SCValdiMarshallableObjectRegistry *registry) {
        registerBridgeClass(registry);

        // Use a second registry, so that we can simulate an externally provided proxy object.
        // We use the first registry to marshall our proxy object, and use the second one
        // to unmarshall it. Since they will use a different object store under the hood,
        // unmarshalling the created proxy should not trigger a cache hit and unwrap the
        // underlying instance.
        [self _getObjectRegistry:^(SCValdiMarshallableObjectRegistry *secondRegistry) {
            registerBridgeClass(secondRegistry);

            SCValdiMarshallerTestBridgeObjectImpl *testObject = [SCValdiMarshallerTestBridgeObjectImpl new];
            id<SCValdiMarshallerTestBridgeObject> testObjectUnmarshalled;

            SCValdiMarshallerScoped(marshaller, {
                NSInteger objectIndex = [registry marshallObject:testObject ofClass:[SCValdiMarshallerTestBridgeObject class] toMarshaller:marshaller];
                SCValdiMarshallerCheck(marshaller);

                // Now unmarshall from the second registry
                testObjectUnmarshalled = [secondRegistry unmarshallObjectOfClass:[SCValdiMarshallerTestBridgeObject class] fromMarshaller:marshaller atIndex:objectIndex];
            });

            XCTAssertNotEqual(testObject, testObjectUnmarshalled);

            __block NSString *submittedStr = nil;
            __block BOOL submittedBool = NO;
            testObject.methodProxy = ^double(NSString *str, BOOL b) {
                submittedStr = str;
                submittedBool = b;
                return 42.0;
            };

            // We should be able to invoke functions on the unmarshalled object, which will call our source test object
            double result = [testObjectUnmarshalled submitString:@"Hello" andBool:YES];

            XCTAssertEqual(42.0, result);
            XCTAssertEqualObjects(@"Hello", submittedStr);
            XCTAssertTrue(submittedBool);
        }];

    }];
}

SCValdiFieldValue SCValdiFunctionInvokeOBO_d(const void *function, SCValdiFieldValue* fields)
{
    double result = ((double(*)(const void *, BOOL, const void *))function)(SCValdiFieldValueGetPtr(fields[0]),
                                                                              SCValdiFieldValueGetBool(fields[1]),
                                                                              SCValdiFieldValueGetPtr(fields[2])
                                                                              );
    return SCValdiFieldValueMakeDouble(result);
}

id SCValdiBlockCreateOBO_d(SCValdiBlockTrampoline trampoline)
{
    return ^double (BOOL p0, const void *p1) {
        return SCValdiFieldValueGetDouble(trampoline(nil, SCValdiFieldValueMakeBool(p0), SCValdiFieldValueMakePtr(p1)));
    };
}

- (void)testCanCreateFunctionTrampoline
{
    [self _getObjectRegistry:^(SCValdiMarshallableObjectRegistry *registry) {

        SCValdiMarshallableObjectBlockSupport blockSupport[] = {
            { .typeEncoding = "obo:d", .invoker = &SCValdiFunctionInvokeOBO_d, .factory = &SCValdiBlockCreateOBO_d },
            { .typeEncoding = nil }
        };

        SCValdiMarshallableObjectFieldDescriptor fields[] = {
            {.name = "someCallback", .selName = nil, .type = "f(b, s): d"},
            {.name = "getMeAString", .selName = nil, .type = "f(): s"},
            {.name = nil}
        };

        [registry registerClass:[SCValdiMarshallerTestBridgeFunction class] objectDescriptor:SCValdiMarshallableObjectDescriptorMake(fields, nil, blockSupport, SCValdiMarshallableObjectTypeFunction)];

        NSMutableArray *submittedParameters = [NSMutableArray array];
        id<SCValdiFunction> function = [SCValdiFunctionWithBlock functionWithBlock:^BOOL(SCValdiMarshallerRef  _Nonnull parameters) {
            for (NSUInteger i = 0; i < SCValdiMarshallerGetCount(parameters); i++) {
                [submittedParameters addObject:SCValdiMarshallerGetUntyped(parameters, i)];
            }

            SCValdiMarshallerPushDouble(parameters, 42);
            return YES;
        }];

        id<SCValdiFunction> function2 = [SCValdiFunctionWithBlock functionWithBlock:^BOOL(SCValdiMarshallerRef  _Nonnull parameters) {
            SCValdiMarshallerPushString(parameters, @"Magic!");
            return YES;
        }];

        SCValdiMarshallerTestBridgeFunction *bridgeFunction;
        SCValdiMarshallerScoped(marshaller, {
            NSInteger mapIndex = SCValdiMarshallerPushMap(marshaller, 1);
            SCValdiMarshallerPushFunction(marshaller, function);
            SCValdiMarshallerPutMapPropertyUninterned(marshaller, @"someCallback", mapIndex);
            SCValdiMarshallerPushFunction(marshaller, function2);
            SCValdiMarshallerPutMapPropertyUninterned(marshaller, @"getMeAString", mapIndex);
            bridgeFunction = [registry unmarshallObjectOfClass:[SCValdiMarshallerTestBridgeFunction class] fromMarshaller:marshaller atIndex:mapIndex];
            SCValdiMarshallerCheck(marshaller);
        });

        XCTAssertNotNil(bridgeFunction);
        double (^block)(BOOL, NSString *) = bridgeFunction.callBlock;
        XCTAssertNotNil(block);

        double result = block(YES, @"Hello");

        NSArray *expectedParameters = @[@YES, @"Hello"];
        XCTAssertEqualObjects(expectedParameters, submittedParameters);
        XCTAssertEqual(42.0, result);

        XCTAssertEqualObjects(@"Magic!", [bridgeFunction getMeAString]);
    }];
}

- (void)testCanMarshallIntEnum
{
    [self _getObjectRegistry:^(SCValdiMarshallableObjectRegistry *registry) {
        const char *identifiers[] = {"SCValdiTestEnum", nil};
        SCValdiMarshallableObjectFieldDescriptor fields[] = {
            {.name = "intEnum", .selName = nil, .type = "r<e>:'[0]'"},
            {.name = nil}
        };

        [registry registerClass:[SCValdiMarshallerTestObject class] objectDescriptor:SCValdiMarshallableObjectDescriptorMake(fields, identifiers, nil, SCValdiMarshallableObjectTypeClass)];

        SCValdiMarshallerTestObject *testObject = [registry makeObjectOfClass:[SCValdiMarshallerTestObject class]];
        testObject.intEnum = SCValdiTestEnumValueB;

        NSString *result;
        SCValdiMarshallerScoped(marshaller, {
            NSInteger objectIndex = [registry marshallObject:testObject ofClass:[SCValdiMarshallerTestObject class] toMarshaller:marshaller];
            result = SCValdiMarshallerToString(marshaller, objectIndex, NO);
        });

        XCTAssertEqualObjects(@"<typedObject SCValdiMarshallerTestObject: { intEnum: 84} >", result);
    }];
}

- (void)testCanUnmarshallIntEnum
{
    [self _getObjectRegistry:^(SCValdiMarshallableObjectRegistry *registry) {
        const char *identifiers[] = {"SCValdiTestEnum", nil};
        SCValdiMarshallableObjectFieldDescriptor fields[] = {
            {.name = "intEnum", .selName = nil, .type = "r<e>:'[0]'"},
            {.name = nil}
        };

        [registry registerClass:[SCValdiMarshallerTestObject class] objectDescriptor:SCValdiMarshallableObjectDescriptorMake(fields, identifiers, nil, SCValdiMarshallableObjectTypeClass)];

        SCValdiMarshallerTestObject *object;

        SCValdiMarshallerScoped(marshaller, {
            NSInteger idx = SCValdiMarshallerPushMap(marshaller, 1);
            SCValdiMarshallerPushInt(marshaller, 42);
            SCValdiMarshallerPutMapPropertyUninterned(marshaller, @"intEnum", idx);

            object = [registry unmarshallObjectOfClass:[SCValdiMarshallerTestObject class] fromMarshaller:marshaller atIndex:idx];
        });

        XCTAssertTrue([object isKindOfClass:[SCValdiMarshallerTestObject class]]);
        XCTAssertEqual(SCValdiTestEnumValueA, object.intEnum);
    }];
}

- (void)testCanMarshallOptionalIntEnum
{
    [self _getObjectRegistry:^(SCValdiMarshallableObjectRegistry *registry) {
        const char *identifiers[] = {"SCValdiTestEnum", nil};
       SCValdiMarshallableObjectFieldDescriptor fields[] = {
            {.name = "optionalIntEnum", .selName = nil, .type = "r@?<e>:'[0]'"},
            {.name = nil}
        };

        [registry registerClass:[SCValdiMarshallerTestObject class] objectDescriptor:SCValdiMarshallableObjectDescriptorMake(fields, identifiers, nil, SCValdiMarshallableObjectTypeClass)];

        SCValdiMarshallerTestObject *testObject = [registry makeObjectOfClass:[SCValdiMarshallerTestObject class]];
        testObject.optionalIntEnum = @(SCValdiTestEnumValueB);

        NSString *result;
        SCValdiMarshallerScoped(marshaller, {
            NSInteger objectIndex = [registry marshallObject:testObject ofClass:[SCValdiMarshallerTestObject class] toMarshaller:marshaller];
            result = SCValdiMarshallerToString(marshaller, objectIndex, NO);
        });

        XCTAssertEqualObjects(@"<typedObject SCValdiMarshallerTestObject: { optionalIntEnum: 84} >", result);
    }];
}

- (void)testCanUnmarshallOptionalIntEnum
{
    [self _getObjectRegistry:^(SCValdiMarshallableObjectRegistry *registry) {
        const char *identifiers[] = {"SCValdiTestEnum", nil};
       SCValdiMarshallableObjectFieldDescriptor fields[] = {
            {.name = "optionalIntEnum", .selName = nil, .type = "r@?<e>:'[0]'"},
            {.name = nil}
        };

        [registry registerClass:[SCValdiMarshallerTestObject class] objectDescriptor:SCValdiMarshallableObjectDescriptorMake(fields, identifiers, nil, SCValdiMarshallableObjectTypeClass)];

        SCValdiMarshallerTestObject *object;

        SCValdiMarshallerScoped(marshaller, {
            NSInteger idx = SCValdiMarshallerPushMap(marshaller, 1);
            SCValdiMarshallerPushInt(marshaller, 42);
            SCValdiMarshallerPutMapPropertyUninterned(marshaller, @"optionalIntEnum", idx);

            object = [registry unmarshallObjectOfClass:[SCValdiMarshallerTestObject class] fromMarshaller:marshaller atIndex:idx];
        });

        XCTAssertTrue([object isKindOfClass:[SCValdiMarshallerTestObject class]]);
        XCTAssertNotNil(object.optionalIntEnum);
        XCTAssertEqual(SCValdiTestEnumValueA, [object.optionalIntEnum integerValue]);
    }];
}

- (void)testCanMarshallStringEnum
{
    [self _getObjectRegistry:^(SCValdiMarshallableObjectRegistry *registry) {
        const char *identifiers[] = {"SCValdiTestStringEnum", nil};
       SCValdiMarshallableObjectFieldDescriptor fields[] = {
            {.name = "stringEnum", .selName = nil, .type = "r:'[0]'"},
            {.name = nil}
        };

        [registry registerClass:[SCValdiMarshallerTestObject class] objectDescriptor:SCValdiMarshallableObjectDescriptorMake(fields, identifiers, nil, SCValdiMarshallableObjectTypeClass)];

        SCValdiMarshallerTestObject *testObject = [registry makeObjectOfClass:[SCValdiMarshallerTestObject class]];
        testObject.stringEnum = SCValdiTestStringEnumValueB;

        NSString *result;
        SCValdiMarshallerScoped(marshaller, {
            NSInteger objectIndex = [registry marshallObject:testObject ofClass:[SCValdiMarshallerTestObject class] toMarshaller:marshaller];
            result = SCValdiMarshallerToString(marshaller, objectIndex, NO);
        });

        XCTAssertEqualObjects(@"<typedObject SCValdiMarshallerTestObject: { stringEnum: ValueB} >", result);
    }];
}

- (void)testCanUnmarshallStringEnum
{
    [self _getObjectRegistry:^(SCValdiMarshallableObjectRegistry *registry) {
        const char *identifiers[] = {"SCValdiTestStringEnum", nil};
       SCValdiMarshallableObjectFieldDescriptor fields[] = {
            {.name = "stringEnum", .selName = nil, .type = "r:'[0]'"},
            {.name = nil}
        };

        [registry registerClass:[SCValdiMarshallerTestObject class] objectDescriptor:SCValdiMarshallableObjectDescriptorMake(fields, identifiers, nil, SCValdiMarshallableObjectTypeClass)];

        SCValdiMarshallerTestObject *object;

        SCValdiMarshallerScoped(marshaller, {
            NSInteger idx = SCValdiMarshallerPushMap(marshaller, 1);
            SCValdiMarshallerPushString(marshaller, @"ValueA");
            SCValdiMarshallerPutMapPropertyUninterned(marshaller, @"stringEnum", idx);

            object = [registry unmarshallObjectOfClass:[SCValdiMarshallerTestObject class] fromMarshaller:marshaller atIndex:idx];
        });

        XCTAssertTrue([object isKindOfClass:[SCValdiMarshallerTestObject class]]);
        XCTAssertEqual(SCValdiTestStringEnumValueA, object.stringEnum);
    }];
}

- (void)testInitWithFieldValues
{
    [self _getObjectRegistry:^(SCValdiMarshallableObjectRegistry *registry) {
        const char *identifiers[] = {"SCValdiMarshallerTestChildObject", nil};
       SCValdiMarshallableObjectFieldDescriptor fields[] = {
            {.name = "aDouble", .selName = nil, .type = "d"},
            {.name = "anOptionalDouble", .selName = nil, .type = "d@?"},
            {.name = "aBool", .selName = nil, .type = "b"},
            {.name = "anOptionalBool", .selName = nil, .type = "b@?"},
            {.name = "bytes", .selName = nil, .type = "t"},
            {.name = "anArray", .selName = nil, .type = "a<s>"},
            {.name = "childObject", .selName = nil, .type = "r:'[0]'"},
            {.name = nil}
        };

        SCValdiMarshallableObjectFieldDescriptor childObjectFields[] = {
            {.name = "childDouble", .selName = nil, .type = "d"},
            {.name = nil}
        };

        [registry registerClass:[SCValdiMarshallerTestObject class] objectDescriptor:SCValdiMarshallableObjectDescriptorMake(fields, identifiers, nil, SCValdiMarshallableObjectTypeClass)];
        [registry registerClass:[SCValdiMarshallerTestChildObject class] objectDescriptor:SCValdiMarshallableObjectDescriptorMake(childObjectFields, nil, nil, SCValdiMarshallableObjectTypeClass)];

        SCValdiMarshallerTestChildObject *childObject = [registry makeObjectWithFieldValuesOfClass:[SCValdiMarshallerTestChildObject class], 42.0];

        XCTAssertEqual(42.0, childObject.childDouble);

        NSData *data = [NSData dataWithBytes:"hello" length:5];
        NSArray *array = @[@"Hello"];

        SCValdiMarshallerTestObject *testObject = [registry makeObjectWithFieldValuesOfClass:[SCValdiMarshallerTestObject class], 84.0, nil, YES, @(NO), data, array, childObject];

        XCTAssertEqual(84.0, testObject.aDouble);
        XCTAssertNil(testObject.anOptionalDouble);
        XCTAssertEqual(YES, testObject.aBool);
        XCTAssertEqualObjects(@NO, testObject.anOptionalBool);
        XCTAssertEqual(data, testObject.bytes);
        XCTAssertEqual(array, testObject.anArray);
        XCTAssertEqual(childObject, testObject.childObject);
    }];
}

- (void)testRetainReleaseObjectsInInitWithFieldValues
{
    [self _getObjectRegistry:^(SCValdiMarshallableObjectRegistry *registry) {
        const char *identifiers[] = {"SCValdiMarshallerTestChildObject", nil};
       SCValdiMarshallableObjectFieldDescriptor fields[] = {
            {.name = "childObject", .selName = nil, .type = "r:'[0]'"},
            {.name = nil}
        };

        SCValdiMarshallableObjectFieldDescriptor childObjectFields[] = {
            {.name = "childDouble", .selName = nil, .type = "d"},
            {.name = nil}
        };

        [registry registerClass:[SCValdiMarshallerTestObject class] objectDescriptor:SCValdiMarshallableObjectDescriptorMake(fields, identifiers, nil, SCValdiMarshallableObjectTypeClass)];
        [registry registerClass:[SCValdiMarshallerTestChildObject class] objectDescriptor:SCValdiMarshallableObjectDescriptorMake(childObjectFields, nil, nil, SCValdiMarshallableObjectTypeClass)];

        SCValdiMarshallerTestChildObject *childObject;

        @autoreleasepool {
            childObject = [registry makeObjectWithFieldValuesOfClass:[SCValdiMarshallerTestChildObject class], 42.0];
        }

        SCValdiMarshallerTestObject *testObject;

        XCTAssertEqual(1, CFGetRetainCount((__bridge CFTypeRef)childObject));

        @autoreleasepool {
            testObject = [registry makeObjectWithFieldValuesOfClass:[SCValdiMarshallerTestObject class], childObject];
        }

        XCTAssertEqual(2, CFGetRetainCount((__bridge CFTypeRef)childObject));

        XCTAssertNotNil(testObject);
        testObject = nil;

        XCTAssertEqual(1, CFGetRetainCount((__bridge CFTypeRef)childObject));
    }];
}

- (NSString *)_performAndGetErrorMessageWithBlock:(dispatch_block_t)block
{
    @try {
        block();
    } @catch (NSException *exc) {
        return exc.description;
    }
    XCTAssertFalse(YES, @"Exception not thrown");
    return nil;
}

- (void)testRaisesHumanReadableErrorMessagesOnUnmarshall
{
    SCValdiMarshallableObjectRegistry *registry = [SCValdiMarshallableObjectRegistry new];
    const char *identifiers[] = {"SCValdiMarshallerTestChildObject", nil};

    SCValdiMarshallableObjectFieldDescriptor fields[] = {
        {.name = "childObject", .selName = nil, .type = "r:'[0]'"},
        {.name = nil}
    };
    SCValdiMarshallableObjectFieldDescriptor childFields[] = {
        {.name = "stringArray", .selName = nil, .type = "a<s>"},
        {.name = nil}
    };

    [registry registerClass:[SCValdiMarshallerTestObject class] objectDescriptor:SCValdiMarshallableObjectDescriptorMake(fields, identifiers, nil, SCValdiMarshallableObjectTypeClass)];
    [registry registerClass:[SCValdiMarshallerTestChildObject class] objectDescriptor:SCValdiMarshallableObjectDescriptorMake(childFields, nil, nil, SCValdiMarshallableObjectTypeClass)];

    NSString *errorMessage = [self _performAndGetErrorMessageWithBlock:^{
        NSDictionary *json = @{
            @"childObject": @{
                    @"stringArray": @"A String"
            }
        };

        SCValdiMarshallerScoped(marshaller, {
            NSInteger index = SCValdiMarshallerPushUntyped(marshaller, json);
            [registry unmarshallObjectOfClass:[SCValdiMarshallerTestObject class] fromMarshaller:marshaller atIndex:index];

        })
    }];

    XCTAssertEqualObjects(@"Failed to unmarshall property 'childObject' of class 'SCValdiMarshallerTestObject'\n[caused by]: Failed to unmarshall property 'stringArray' of class 'SCValdiMarshallerTestChildObject'\n[caused by]: Cannot convert type 'interned string' to type 'array'", errorMessage);
}

- (void)testRaisesHumanReadableErrorMessagesOnFunctionCall
{
    [self _getObjectRegistry:^(SCValdiMarshallableObjectRegistry *registry) {

        SCValdiMarshallableObjectFieldDescriptor fields[] = {
            {.name = "block", .selName = nil, .type = "f(s, d): s"},
            {.name = nil}
        };

        SCValdiMarshallableObjectBlockSupport blockSupport[] = {
            { .typeEncoding = "ood:o", .invoker = &invokeBlock, .factory = &createBlock },
            { .typeEncoding = nil }
        };

        [registry registerClass:[SCValdiMarshallerTestObject class] objectDescriptor:SCValdiMarshallableObjectDescriptorMake(fields, nil, blockSupport, SCValdiMarshallableObjectTypeClass)];


        SCValdiMarshallerTestObject *testObject = [registry makeObjectOfClass:[SCValdiMarshallerTestObject class]];
        testObject.block = ^NSString *(NSString *str, double i) {
            return str;
        };

        id<SCValdiFunction> marshalledFunction;

        {
            NSDictionary *marshalledObject;
            SCValdiMarshallerScoped(marshaller, {
                NSInteger objectIndex = [registry marshallObject:testObject toMarshaller:marshaller];
                marshalledObject = SCValdiMarshallerGetUntyped(marshaller, objectIndex);
            })

            marshalledFunction = marshalledObject[@"block"];
        }

        // Call it with incorrect parameters

        NSString *errorMessage = [self _performAndGetErrorMessageWithBlock:^{
            SCValdiMarshallerScoped(marshaller, {
                SCValdiMarshallerPushInt(marshaller, 42);
                [marshalledFunction performSyncWithMarshaller:marshaller propagatesError:YES];
            })
        }];

        XCTAssertEqualObjects(@"Failed to unmarshall parameter '0' of function block\n[caused by]: Cannot convert type 'int' to type 'string'", errorMessage);
    }];
}

- (void)testCanMarshallGenericObjectWithStrings
{
    [self _getObjectRegistry:^(SCValdiMarshallableObjectRegistry *registry) {

        SCValdiMarshallerRegisterTestGenericObject(registry);

        const char *identifiers[] = {
            "SCValdiMarshallerTestGenericObject",
            nil
        };

        SCValdiMarshallableObjectFieldDescriptor fields[] = {
            {.name = "genericObject", .selName = nil, .type = "g:'[0]'<s, s>"},
            {.name = nil}
        };

        [registry registerClass:[SCValdiMarshallerTestGenericObjectParent class] objectDescriptor:SCValdiMarshallableObjectDescriptorMake(fields, identifiers, nil, SCValdiMarshallableObjectTypeClass)];

        SCValdiMarshallerTestGenericObjectParent *testObject = [registry makeObjectOfClass:[SCValdiMarshallerTestGenericObjectParent class]];
        SCValdiMarshallerTestGenericObject *genericObject = [registry makeObjectOfClass:[SCValdiMarshallerTestGenericObject class]];
        genericObject.childObject1 = @"Hello";
        genericObject.childObject2 = @"World";
        testObject.genericObject = genericObject;

        NSString *result;
        SCValdiMarshallerScoped(marshaller, {
            NSInteger objectIndex = [registry marshallObject:testObject ofClass:[SCValdiMarshallerTestGenericObjectParent class] toMarshaller:marshaller];
            result = SCValdiMarshallerToString(marshaller, objectIndex, NO);
        });

        XCTAssertEqualObjects(@"<typedObject SCValdiMarshallerTestGenericObjectParent: { genericObject: <typedObject SCValdiMarshallerTestGenericObject: { childObject1: Hello, childObject2: World} >} >", result);
    }];
}

- (void)testCanUnmarshallGenericObjectWithStrings
{
    [self _getObjectRegistry:^(SCValdiMarshallableObjectRegistry *registry) {

        SCValdiMarshallerRegisterTestGenericObject(registry);

        const char *identifiers[] = {
            "SCValdiMarshallerTestGenericObject",
            nil
        };

        SCValdiMarshallableObjectFieldDescriptor fields[] = {
            {.name = "genericObject", .selName = nil, .type = "g:'[0]'<s, s>"},
            {.name = nil}
        };

        [registry registerClass:[SCValdiMarshallerTestGenericObjectParent class] objectDescriptor:SCValdiMarshallableObjectDescriptorMake(fields, identifiers, nil, SCValdiMarshallableObjectTypeClass)];

        SCValdiMarshallerTestGenericObjectParent *object;

        SCValdiMarshallerScoped(marshaller, {
            NSInteger idx = SCValdiMarshallerPushMap(marshaller, 1);
            NSInteger innerIndex = SCValdiMarshallerPushMap(marshaller, 2);
            SCValdiMarshallerPushString(marshaller, @"Hello");
            SCValdiMarshallerPutMapPropertyUninterned(marshaller, @"childObject1", innerIndex);
            SCValdiMarshallerPushString(marshaller, @"World");
            SCValdiMarshallerPutMapPropertyUninterned(marshaller, @"childObject2", innerIndex);

            SCValdiMarshallerPutMapPropertyUninterned(marshaller, @"genericObject" , idx);

            object = [registry unmarshallObjectOfClass:[SCValdiMarshallerTestGenericObjectParent class] fromMarshaller:marshaller atIndex:idx];
        });

        XCTAssertTrue([object isKindOfClass:[SCValdiMarshallerTestGenericObjectParent class]]);
        XCTAssertTrue([object.genericObject isKindOfClass:[SCValdiMarshallerTestGenericObject class]]);
        XCTAssertEqualObjects(@"Hello", [object.genericObject childObject1]);
        XCTAssertEqualObjects(@"World", [object.genericObject childObject2]);
    }];
}

- (void)testCanMarshallGenericObjectWithNumbers
{
    [self _getObjectRegistry:^(SCValdiMarshallableObjectRegistry *registry) {

        SCValdiMarshallerRegisterTestGenericObject(registry);

        const char *identifiers[] = {
            "SCValdiMarshallerTestGenericObject",
            nil
        };

        SCValdiMarshallableObjectFieldDescriptor fields[] = {
            {.name = "genericObject", .selName = nil, .type = "g:'[0]'<d@, d@>"},
            {.name = nil}
        };

        [registry registerClass:[SCValdiMarshallerTestGenericObjectParent class] objectDescriptor:SCValdiMarshallableObjectDescriptorMake(fields, identifiers, nil, SCValdiMarshallableObjectTypeClass)];

        SCValdiMarshallerTestGenericObjectParent *testObject = [registry makeObjectOfClass:[SCValdiMarshallerTestGenericObjectParent class]];
        SCValdiMarshallerTestGenericObject *genericObject = [registry makeObjectOfClass:[SCValdiMarshallerTestGenericObject class]];
        genericObject.childObject1 = @(50);
        genericObject.childObject2 = @(100);
        testObject.genericObject = genericObject;

        NSString *result;
        SCValdiMarshallerScoped(marshaller, {
            NSInteger objectIndex = [registry marshallObject:testObject ofClass:[SCValdiMarshallerTestGenericObjectParent class] toMarshaller:marshaller];
            result = SCValdiMarshallerToString(marshaller, objectIndex, NO);
        });

        XCTAssertEqualObjects(@"<typedObject SCValdiMarshallerTestGenericObjectParent: { genericObject: <typedObject SCValdiMarshallerTestGenericObject: { childObject1: 50.000000, childObject2: 100.000000} >} >", result);
    }];
}

- (void)testCanUnmarshallGenericObjectWithNumbers
{
    [self _getObjectRegistry:^(SCValdiMarshallableObjectRegistry *registry) {

        SCValdiMarshallerRegisterTestGenericObject(registry);

        const char *identifiers[] = {
            "SCValdiMarshallerTestGenericObject",
            nil
        };

        SCValdiMarshallableObjectFieldDescriptor fields[] = {
            {.name = "genericObject", .selName = nil, .type = "g:'[0]'<d@, d@>"},
            {.name = nil}
        };

        [registry registerClass:[SCValdiMarshallerTestGenericObjectParent class] objectDescriptor:SCValdiMarshallableObjectDescriptorMake(fields, identifiers, nil, SCValdiMarshallableObjectTypeClass)];

        SCValdiMarshallerTestGenericObjectParent *object;

        SCValdiMarshallerScoped(marshaller, {
            NSInteger idx = SCValdiMarshallerPushMap(marshaller, 1);
            NSInteger innerIndex = SCValdiMarshallerPushMap(marshaller, 2);
            SCValdiMarshallerPushDouble(marshaller, 60);
            SCValdiMarshallerPutMapPropertyUninterned(marshaller, @"childObject1", innerIndex);
            SCValdiMarshallerPushDouble(marshaller, 70);
            SCValdiMarshallerPutMapPropertyUninterned(marshaller, @"childObject2", innerIndex);

            SCValdiMarshallerPutMapPropertyUninterned(marshaller, @"genericObject" , idx);

            object = [registry unmarshallObjectOfClass:[SCValdiMarshallerTestGenericObjectParent class] fromMarshaller:marshaller atIndex:idx];
        });

        XCTAssertTrue([object isKindOfClass:[SCValdiMarshallerTestGenericObjectParent class]]);
        XCTAssertTrue([object.genericObject isKindOfClass:[SCValdiMarshallerTestGenericObject class]]);
        XCTAssertEqualObjects(@60, [object.genericObject childObject1]);
        XCTAssertEqualObjects(@70, [object.genericObject childObject2]);
    }];
}

- (void)testCanMarshallGenericObjectWithOptionalNumbers
{
    [self _getObjectRegistry:^(SCValdiMarshallableObjectRegistry *registry) {

        SCValdiMarshallerRegisterTestGenericObject(registry);

        const char *identifiers[] = {
            "SCValdiMarshallerTestGenericObject",
            nil
        };

        SCValdiMarshallableObjectFieldDescriptor fields[] = {
            {.name = "genericObject", .selName = nil, .type = "g:'[0]'<d@?, d@?>"},
            {.name = nil}
        };

        [registry registerClass:[SCValdiMarshallerTestGenericObjectParent class] objectDescriptor:SCValdiMarshallableObjectDescriptorMake(fields, identifiers, nil, SCValdiMarshallableObjectTypeClass)];

        SCValdiMarshallerTestGenericObjectParent *testObject = [registry makeObjectOfClass:[SCValdiMarshallerTestGenericObjectParent class]];
        SCValdiMarshallerTestGenericObject *genericObject = [registry makeObjectOfClass:[SCValdiMarshallerTestGenericObject class]];
        genericObject.childObject1 = nil;
        genericObject.childObject2 = @50;
        testObject.genericObject = genericObject;

        NSString *result;
        SCValdiMarshallerScoped(marshaller, {
            NSInteger objectIndex = [registry marshallObject:testObject ofClass:[SCValdiMarshallerTestGenericObjectParent class] toMarshaller:marshaller];
            result = SCValdiMarshallerToString(marshaller, objectIndex, NO);
        });

        XCTAssertEqualObjects(@"<typedObject SCValdiMarshallerTestGenericObjectParent: { genericObject: <typedObject SCValdiMarshallerTestGenericObject: { childObject2: 50.000000} >} >", result);
    }];
}

- (void)testCanUnmarshallGenericObjectWithOptionalNumbers
{
    [self _getObjectRegistry:^(SCValdiMarshallableObjectRegistry *registry) {

        SCValdiMarshallerRegisterTestGenericObject(registry);

        const char *identifiers[] = {
            "SCValdiMarshallerTestGenericObject",
            nil
        };

        SCValdiMarshallableObjectFieldDescriptor fields[] = {
            {.name = "genericObject", .selName = nil, .type = "g:'[0]'<d@?, d@?>"},
            {.name = nil}
        };

        [registry registerClass:[SCValdiMarshallerTestGenericObjectParent class] objectDescriptor:SCValdiMarshallableObjectDescriptorMake(fields, identifiers, nil, SCValdiMarshallableObjectTypeClass)];

        SCValdiMarshallerTestGenericObjectParent *object;

        SCValdiMarshallerScoped(marshaller, {
            NSInteger idx = SCValdiMarshallerPushMap(marshaller, 1);
            NSInteger innerIndex = SCValdiMarshallerPushMap(marshaller, 2);
            SCValdiMarshallerPushUndefined(marshaller);
            SCValdiMarshallerPutMapPropertyUninterned(marshaller, @"childObject1", innerIndex);
            SCValdiMarshallerPushOptionalDouble(marshaller, @70);
            SCValdiMarshallerPutMapPropertyUninterned(marshaller, @"childObject2", innerIndex);

            SCValdiMarshallerPutMapPropertyUninterned(marshaller, @"genericObject" , idx);

            object = [registry unmarshallObjectOfClass:[SCValdiMarshallerTestGenericObjectParent class] fromMarshaller:marshaller atIndex:idx];
        });

        XCTAssertTrue([object isKindOfClass:[SCValdiMarshallerTestGenericObjectParent class]]);
        XCTAssertTrue([object.genericObject isKindOfClass:[SCValdiMarshallerTestGenericObject class]]);
        XCTAssertEqualObjects(nil, [object.genericObject childObject1]);
        XCTAssertEqualObjects(@70, [object.genericObject childObject2]);
    }];
}

- (void)testCanMarshallGenericObjectWithOptionalStrings
{
    [self _getObjectRegistry:^(SCValdiMarshallableObjectRegistry *registry) {

        SCValdiMarshallerRegisterTestGenericObject(registry);

        const char *identifiers[] = {
            "SCValdiMarshallerTestGenericObject",
            nil
        };

        SCValdiMarshallableObjectFieldDescriptor fields[] = {
            {.name = "genericObject", .selName = nil, .type = "g:'[0]'<s?, s?>"},
            {.name = nil}
        };

        [registry registerClass:[SCValdiMarshallerTestGenericObjectParent class] objectDescriptor:SCValdiMarshallableObjectDescriptorMake(fields, identifiers, nil, SCValdiMarshallableObjectTypeClass)];

        SCValdiMarshallerTestGenericObjectParent *testObject = [registry makeObjectOfClass:[SCValdiMarshallerTestGenericObjectParent class]];
        SCValdiMarshallerTestGenericObject *genericObject = [registry makeObjectOfClass:[SCValdiMarshallerTestGenericObject class]];
        genericObject.childObject1 = nil;
        genericObject.childObject2 = @"bar";
        testObject.genericObject = genericObject;

        NSString *result;
        SCValdiMarshallerScoped(marshaller, {
            NSInteger objectIndex = [registry marshallObject:testObject ofClass:[SCValdiMarshallerTestGenericObjectParent class] toMarshaller:marshaller];
            result = SCValdiMarshallerToString(marshaller, objectIndex, NO);
        });

        XCTAssertEqualObjects(@"<typedObject SCValdiMarshallerTestGenericObjectParent: { genericObject: <typedObject SCValdiMarshallerTestGenericObject: { childObject2: bar} >} >", result);
    }];
}

- (void)testCanUnmarshallGenericObjectWithOptionalStrings
{
    [self _getObjectRegistry:^(SCValdiMarshallableObjectRegistry *registry) {

        SCValdiMarshallerRegisterTestGenericObject(registry);

        const char *identifiers[] = {
            "SCValdiMarshallerTestGenericObject",
            nil
        };

        SCValdiMarshallableObjectFieldDescriptor fields[] = {
            {.name = "genericObject", .selName = nil, .type = "g:'[0]'<s?, s?>"},
            {.name = nil}
        };

        [registry registerClass:[SCValdiMarshallerTestGenericObjectParent class] objectDescriptor:SCValdiMarshallableObjectDescriptorMake(fields, identifiers, nil, SCValdiMarshallableObjectTypeClass)];

        SCValdiMarshallerTestGenericObjectParent *object;

        SCValdiMarshallerScoped(marshaller, {
            NSInteger idx = SCValdiMarshallerPushMap(marshaller, 1);
            NSInteger innerIndex = SCValdiMarshallerPushMap(marshaller, 2);
            SCValdiMarshallerPushUndefined(marshaller);
            SCValdiMarshallerPutMapPropertyUninterned(marshaller, @"childObject1", innerIndex);
            SCValdiMarshallerPushOptionalString(marshaller, @"bar");
            SCValdiMarshallerPutMapPropertyUninterned(marshaller, @"childObject2", innerIndex);

            SCValdiMarshallerPutMapPropertyUninterned(marshaller, @"genericObject" , idx);

            object = [registry unmarshallObjectOfClass:[SCValdiMarshallerTestGenericObjectParent class] fromMarshaller:marshaller atIndex:idx];
        });

        XCTAssertTrue([object isKindOfClass:[SCValdiMarshallerTestGenericObjectParent class]]);
        XCTAssertTrue([object.genericObject isKindOfClass:[SCValdiMarshallerTestGenericObject class]]);
        XCTAssertEqualObjects(nil, [object.genericObject childObject1]);
        XCTAssertEqualObjects(@"bar", [object.genericObject childObject2]);
    }];
}

- (void)testCanMarshallGenericObjectWithBool
{
    [self _getObjectRegistry:^(SCValdiMarshallableObjectRegistry *registry) {

        SCValdiMarshallerRegisterTestGenericObject(registry);

        const char *identifiers[] = {
            "SCValdiMarshallerTestGenericObject",
            nil
        };

        SCValdiMarshallableObjectFieldDescriptor fields[] = {
            {.name = "genericObject", .selName = nil, .type = "g:'[0]'<b@, b@>"},
            {.name = nil}
        };

        [registry registerClass:[SCValdiMarshallerTestGenericObjectParent class] objectDescriptor:SCValdiMarshallableObjectDescriptorMake(fields, identifiers, nil, SCValdiMarshallableObjectTypeClass)];

        SCValdiMarshallerTestGenericObjectParent *testObject = [registry makeObjectOfClass:[SCValdiMarshallerTestGenericObjectParent class]];
        SCValdiMarshallerTestGenericObject *genericObject = [registry makeObjectOfClass:[SCValdiMarshallerTestGenericObject class]];
        genericObject.childObject1 = @YES;
        genericObject.childObject2 = @NO;
        testObject.genericObject = genericObject;

        NSString *result;
        SCValdiMarshallerScoped(marshaller, {
            NSInteger objectIndex = [registry marshallObject:testObject ofClass:[SCValdiMarshallerTestGenericObjectParent class] toMarshaller:marshaller];
            result = SCValdiMarshallerToString(marshaller, objectIndex, NO);
        });

        XCTAssertEqualObjects(@"<typedObject SCValdiMarshallerTestGenericObjectParent: { genericObject: <typedObject SCValdiMarshallerTestGenericObject: { childObject1: true, childObject2: false} >} >", result);
    }];
}

- (void)testCanUnmarshallGenericObjectWithBool
{
    [self _getObjectRegistry:^(SCValdiMarshallableObjectRegistry *registry) {

        SCValdiMarshallerRegisterTestGenericObject(registry);

        const char *identifiers[] = {
            "SCValdiMarshallerTestGenericObject",
            nil
        };

        SCValdiMarshallableObjectFieldDescriptor fields[] = {
            {.name = "genericObject", .selName = nil, .type = "g:'[0]'<b@, b@>"},
            {.name = nil}
        };

        [registry registerClass:[SCValdiMarshallerTestGenericObjectParent class] objectDescriptor:SCValdiMarshallableObjectDescriptorMake(fields, identifiers, nil, SCValdiMarshallableObjectTypeClass)];

        SCValdiMarshallerTestGenericObjectParent *object;

        SCValdiMarshallerScoped(marshaller, {
            NSInteger idx = SCValdiMarshallerPushMap(marshaller, 1);
            NSInteger innerIndex = SCValdiMarshallerPushMap(marshaller, 2);
            SCValdiMarshallerPushBool(marshaller, YES);
            SCValdiMarshallerPutMapPropertyUninterned(marshaller, @"childObject1", innerIndex);
            SCValdiMarshallerPushBool(marshaller, NO);
            SCValdiMarshallerPutMapPropertyUninterned(marshaller, @"childObject2", innerIndex);

            SCValdiMarshallerPutMapPropertyUninterned(marshaller, @"genericObject" , idx);

            object = [registry unmarshallObjectOfClass:[SCValdiMarshallerTestGenericObjectParent class] fromMarshaller:marshaller atIndex:idx];
        });

        XCTAssertTrue([object isKindOfClass:[SCValdiMarshallerTestGenericObjectParent class]]);
        XCTAssertTrue([object.genericObject isKindOfClass:[SCValdiMarshallerTestGenericObject class]]);
        XCTAssertEqualObjects(@YES, [object.genericObject childObject1]);
        XCTAssertEqualObjects(@NO, [object.genericObject childObject2]);
    }];
}

- (void)testCanMarshallGenericObjectWithOptionalBool
{
    [self _getObjectRegistry:^(SCValdiMarshallableObjectRegistry *registry) {

        SCValdiMarshallerRegisterTestGenericObject(registry);

        const char *identifiers[] = {
            "SCValdiMarshallerTestGenericObject",
            nil
        };

        SCValdiMarshallableObjectFieldDescriptor fields[] = {
            {.name = "genericObject", .selName = nil, .type = "g:'[0]'<b@?, b@?>"},
            {.name = nil}
        };

        [registry registerClass:[SCValdiMarshallerTestGenericObjectParent class] objectDescriptor:SCValdiMarshallableObjectDescriptorMake(fields, identifiers, nil, SCValdiMarshallableObjectTypeClass)];

        SCValdiMarshallerTestGenericObjectParent *testObject = [registry makeObjectOfClass:[SCValdiMarshallerTestGenericObjectParent class]];
        SCValdiMarshallerTestGenericObject *genericObject = [registry makeObjectOfClass:[SCValdiMarshallerTestGenericObject class]];
        genericObject.childObject1 = nil;
        genericObject.childObject2 = @NO;
        testObject.genericObject = genericObject;

        NSString *result;
        SCValdiMarshallerScoped(marshaller, {
            NSInteger objectIndex = [registry marshallObject:testObject ofClass:[SCValdiMarshallerTestGenericObjectParent class] toMarshaller:marshaller];
            result = SCValdiMarshallerToString(marshaller, objectIndex, NO);
        });

        XCTAssertEqualObjects(@"<typedObject SCValdiMarshallerTestGenericObjectParent: { genericObject: <typedObject SCValdiMarshallerTestGenericObject: { childObject2: false} >} >", result);
    }];
}

- (void)testCanUnmarshallGenericObjectWithOptionalBool
{
    [self _getObjectRegistry:^(SCValdiMarshallableObjectRegistry *registry) {

        SCValdiMarshallerRegisterTestGenericObject(registry);

        const char *identifiers[] = {
            "SCValdiMarshallerTestGenericObject",
            nil
        };

        SCValdiMarshallableObjectFieldDescriptor fields[] = {
            {.name = "genericObject", .selName = nil, .type = "g:'[0]'<b@?, b@?>"},
            {.name = nil}
        };

        [registry registerClass:[SCValdiMarshallerTestGenericObjectParent class] objectDescriptor:SCValdiMarshallableObjectDescriptorMake(fields, identifiers, nil, SCValdiMarshallableObjectTypeClass)];

        SCValdiMarshallerTestGenericObjectParent *object;

        SCValdiMarshallerScoped(marshaller, {
            NSInteger idx = SCValdiMarshallerPushMap(marshaller, 1);
            NSInteger innerIndex = SCValdiMarshallerPushMap(marshaller, 2);
            SCValdiMarshallerPushOptionalBool(marshaller, nil);
            SCValdiMarshallerPutMapPropertyUninterned(marshaller, @"childObject1", innerIndex);
            SCValdiMarshallerPushOptionalBool(marshaller, @NO);
            SCValdiMarshallerPutMapPropertyUninterned(marshaller, @"childObject2", innerIndex);

            SCValdiMarshallerPutMapPropertyUninterned(marshaller, @"genericObject" , idx);

            object = [registry unmarshallObjectOfClass:[SCValdiMarshallerTestGenericObjectParent class] fromMarshaller:marshaller atIndex:idx];
        });

        XCTAssertTrue([object isKindOfClass:[SCValdiMarshallerTestGenericObjectParent class]]);
        XCTAssertTrue([object.genericObject isKindOfClass:[SCValdiMarshallerTestGenericObject class]]);
        XCTAssertEqualObjects(nil, [object.genericObject childObject1]);
        XCTAssertEqualObjects(@NO, [object.genericObject childObject2]);
    }];
}

- (void)testCanMarshallGenericObjectWithIntEnum
{
    [self _getObjectRegistry:^(SCValdiMarshallableObjectRegistry *registry) {

        SCValdiMarshallerRegisterTestGenericObject(registry);

        const char *identifiers[] = {
            "SCValdiMarshallerTestGenericObject",
            "SCValdiTestEnum",
            nil
        };

        SCValdiMarshallableObjectFieldDescriptor fields[] = {
            {.name = "genericObject", .selName = nil, .type = "g:'[0]'<r@:'[1]', r@:'[1]'>"},
            {.name = nil}
        };

        [registry registerClass:[SCValdiMarshallerTestGenericObjectParent class] objectDescriptor:SCValdiMarshallableObjectDescriptorMake(fields, identifiers, nil, SCValdiMarshallableObjectTypeClass)];

        SCValdiMarshallerTestGenericObjectParent *testObject = [registry makeObjectOfClass:[SCValdiMarshallerTestGenericObjectParent class]];
        SCValdiMarshallerTestGenericObject *genericObject = [registry makeObjectOfClass:[SCValdiMarshallerTestGenericObject class]];
        genericObject.childObject1 = @(SCValdiTestEnumValueA);
        genericObject.childObject2 = @(SCValdiTestEnumValueB);
        testObject.genericObject = genericObject;

        NSString *result;
        SCValdiMarshallerScoped(marshaller, {
            NSInteger objectIndex = [registry marshallObject:testObject ofClass:[SCValdiMarshallerTestGenericObjectParent class] toMarshaller:marshaller];
            result = SCValdiMarshallerToString(marshaller, objectIndex, NO);
        });

        XCTAssertEqualObjects(@"<typedObject SCValdiMarshallerTestGenericObjectParent: { genericObject: <typedObject SCValdiMarshallerTestGenericObject: { childObject1: 42, childObject2: 84} >} >", result);
    }];
}

- (void)testCanUnmarshallGenericObjectWithIntEnum
{
    [self _getObjectRegistry:^(SCValdiMarshallableObjectRegistry *registry) {

        SCValdiMarshallerRegisterTestGenericObject(registry);

        const char *identifiers[] = {
            "SCValdiMarshallerTestGenericObject",
            "SCValdiTestEnum",
            nil
        };

        SCValdiMarshallableObjectFieldDescriptor fields[] = {
            {.name = "genericObject", .selName = nil, .type = "g:'[0]'<r@:'[1]', r@:'[1]'>"},
            {.name = nil}
        };

        [registry registerClass:[SCValdiMarshallerTestGenericObjectParent class] objectDescriptor:SCValdiMarshallableObjectDescriptorMake(fields, identifiers, nil, SCValdiMarshallableObjectTypeClass)];

        SCValdiMarshallerTestGenericObjectParent *object;

        SCValdiMarshallerScoped(marshaller, {
            NSInteger idx = SCValdiMarshallerPushMap(marshaller, 1);
            NSInteger innerIndex = SCValdiMarshallerPushMap(marshaller, 2);
            SCValdiMarshallerPushEnumInt(marshaller, SCValdiTestEnumValueA);
            SCValdiMarshallerPutMapPropertyUninterned(marshaller, @"childObject1", innerIndex);
            SCValdiMarshallerPushEnumInt(marshaller, SCValdiTestEnumValueB);
            SCValdiMarshallerPutMapPropertyUninterned(marshaller, @"childObject2", innerIndex);

            SCValdiMarshallerPutMapPropertyUninterned(marshaller, @"genericObject" , idx);

            object = [registry unmarshallObjectOfClass:[SCValdiMarshallerTestGenericObjectParent class] fromMarshaller:marshaller atIndex:idx];
        });

        XCTAssertTrue([object isKindOfClass:[SCValdiMarshallerTestGenericObjectParent class]]);
        XCTAssertTrue([object.genericObject isKindOfClass:[SCValdiMarshallerTestGenericObject class]]);
        XCTAssertEqualObjects(@(SCValdiTestEnumValueA), [object.genericObject childObject1]);
        XCTAssertEqualObjects(@(SCValdiTestEnumValueB), [object.genericObject childObject2]);
    }];
}

- (void)testCanMarshallGenericObjectWithOptionalIntEnum
{
    [self _getObjectRegistry:^(SCValdiMarshallableObjectRegistry *registry) {

        SCValdiMarshallerRegisterTestGenericObject(registry);

        const char *identifiers[] = {
            "SCValdiMarshallerTestGenericObject",
            "SCValdiTestEnum",
            nil
        };

        SCValdiMarshallableObjectFieldDescriptor fields[] = {
            {.name = "genericObject", .selName = nil, .type = "g:'[0]'<r@?:'[1]', r@?:'[1]'>"},
            {.name = nil}
        };

        [registry registerClass:[SCValdiMarshallerTestGenericObjectParent class] objectDescriptor:SCValdiMarshallableObjectDescriptorMake(fields, identifiers, nil, SCValdiMarshallableObjectTypeClass)];

        SCValdiMarshallerTestGenericObjectParent *testObject = [registry makeObjectOfClass:[SCValdiMarshallerTestGenericObjectParent class]];
        SCValdiMarshallerTestGenericObject *genericObject = [registry makeObjectOfClass:[SCValdiMarshallerTestGenericObject class]];
        genericObject.childObject1 = nil;
        genericObject.childObject2 = @(SCValdiTestEnumValueB);
        testObject.genericObject = genericObject;

        NSString *result;
        SCValdiMarshallerScoped(marshaller, {
            NSInteger objectIndex = [registry marshallObject:testObject ofClass:[SCValdiMarshallerTestGenericObjectParent class] toMarshaller:marshaller];
            result = SCValdiMarshallerToString(marshaller, objectIndex, NO);
        });

        XCTAssertEqualObjects(@"<typedObject SCValdiMarshallerTestGenericObjectParent: { genericObject: <typedObject SCValdiMarshallerTestGenericObject: { childObject2: 84} >} >", result);
    }];
}

- (void)testCanUnmarshallGenericObjectWithOptionalIntEnum
{
    [self _getObjectRegistry:^(SCValdiMarshallableObjectRegistry *registry) {

        SCValdiMarshallerRegisterTestGenericObject(registry);

        const char *identifiers[] = {
            "SCValdiMarshallerTestGenericObject",
            "SCValdiTestEnum",
            nil
        };

        SCValdiMarshallableObjectFieldDescriptor fields[] = {
            {.name = "genericObject", .selName = nil, .type = "g:'[0]'<r@?:'[1]', r@?:'[1]'>"},
            {.name = nil}
        };

        [registry registerClass:[SCValdiMarshallerTestGenericObjectParent class] objectDescriptor:SCValdiMarshallableObjectDescriptorMake(fields, identifiers, nil, SCValdiMarshallableObjectTypeClass)];

        SCValdiMarshallerTestGenericObjectParent *object;

        SCValdiMarshallerScoped(marshaller, {
            NSInteger idx = SCValdiMarshallerPushMap(marshaller, 1);
            NSInteger innerIndex = SCValdiMarshallerPushMap(marshaller, 2);
            SCValdiMarshallerPushUndefined(marshaller);
            SCValdiMarshallerPutMapPropertyUninterned(marshaller, @"childObject1", innerIndex);
            SCValdiMarshallerPushOptionalDouble(marshaller, @(SCValdiTestEnumValueB));
            SCValdiMarshallerPutMapPropertyUninterned(marshaller, @"childObject2", innerIndex);

            SCValdiMarshallerPutMapPropertyUninterned(marshaller, @"genericObject" , idx);

            object = [registry unmarshallObjectOfClass:[SCValdiMarshallerTestGenericObjectParent class] fromMarshaller:marshaller atIndex:idx];
        });

        XCTAssertTrue([object isKindOfClass:[SCValdiMarshallerTestGenericObjectParent class]]);
        XCTAssertTrue([object.genericObject isKindOfClass:[SCValdiMarshallerTestGenericObject class]]);
        XCTAssertEqualObjects(nil, [object.genericObject childObject1]);
        XCTAssertEqualObjects(@(SCValdiTestEnumValueB), [object.genericObject childObject2]);
    }];
}

- (void)testCanMarshallGenericObjectWithStringEnum
{
    [self _getObjectRegistry:^(SCValdiMarshallableObjectRegistry *registry) {

        SCValdiMarshallerRegisterTestGenericObject(registry);

        const char *identifiers[] = {
            "SCValdiMarshallerTestGenericObject",
            "SCValdiTestStringEnum",
            nil
        };

        SCValdiMarshallableObjectFieldDescriptor fields[] = {
            {.name = "genericObject", .selName = nil, .type = "g:'[0]'<r:'[1]', r:'[1]'>"},
            {.name = nil}
        };

        [registry registerClass:[SCValdiMarshallerTestGenericObjectParent class] objectDescriptor:SCValdiMarshallableObjectDescriptorMake(fields, identifiers, nil, SCValdiMarshallableObjectTypeClass)];

        SCValdiMarshallerTestGenericObjectParent *testObject = [registry makeObjectOfClass:[SCValdiMarshallerTestGenericObjectParent class]];
        SCValdiMarshallerTestGenericObject *genericObject = [registry makeObjectOfClass:[SCValdiMarshallerTestGenericObject class]];
        genericObject.childObject1 = SCValdiTestStringEnumValueA;
        genericObject.childObject2 = SCValdiTestStringEnumValueB;
        testObject.genericObject = genericObject;

        NSString *result;
        SCValdiMarshallerScoped(marshaller, {
            NSInteger objectIndex = [registry marshallObject:testObject ofClass:[SCValdiMarshallerTestGenericObjectParent class] toMarshaller:marshaller];
            result = SCValdiMarshallerToString(marshaller, objectIndex, NO);
        });

        XCTAssertEqualObjects(@"<typedObject SCValdiMarshallerTestGenericObjectParent: { genericObject: <typedObject SCValdiMarshallerTestGenericObject: { childObject1: ValueA, childObject2: ValueB} >} >", result);
    }];
}

- (void)testCanUnmarshallGenericObjectWithStringEnum
{
    [self _getObjectRegistry:^(SCValdiMarshallableObjectRegistry *registry) {

        SCValdiMarshallerRegisterTestGenericObject(registry);

        const char *identifiers[] = {
            "SCValdiMarshallerTestGenericObject",
            "SCValdiTestStringEnum",
            nil
        };

        SCValdiMarshallableObjectFieldDescriptor fields[] = {
            {.name = "genericObject", .selName = nil, .type = "g:'[0]'<r:'[1]', r:'[1]'>"},
            {.name = nil}
        };

        [registry registerClass:[SCValdiMarshallerTestGenericObjectParent class] objectDescriptor:SCValdiMarshallableObjectDescriptorMake(fields, identifiers, nil, SCValdiMarshallableObjectTypeClass)];

        SCValdiMarshallerTestGenericObjectParent *object;

        SCValdiMarshallerScoped(marshaller, {
            NSInteger idx = SCValdiMarshallerPushMap(marshaller, 1);
            NSInteger innerIndex = SCValdiMarshallerPushMap(marshaller, 2);
            SCValdiMarshallerPushString(marshaller, SCValdiTestStringEnumValueA);
            SCValdiMarshallerPutMapPropertyUninterned(marshaller, @"childObject1", innerIndex);
            SCValdiMarshallerPushString(marshaller, SCValdiTestStringEnumValueB);
            SCValdiMarshallerPutMapPropertyUninterned(marshaller, @"childObject2", innerIndex);

            SCValdiMarshallerPutMapPropertyUninterned(marshaller, @"genericObject" , idx);

            object = [registry unmarshallObjectOfClass:[SCValdiMarshallerTestGenericObjectParent class] fromMarshaller:marshaller atIndex:idx];
        });

        XCTAssertTrue([object isKindOfClass:[SCValdiMarshallerTestGenericObjectParent class]]);
        XCTAssertTrue([object.genericObject isKindOfClass:[SCValdiMarshallerTestGenericObject class]]);
        XCTAssertEqualObjects(SCValdiTestStringEnumValueA, [object.genericObject childObject1]);
        XCTAssertEqualObjects(SCValdiTestStringEnumValueB, [object.genericObject childObject2]);
    }];
}

- (void)testCanMarshallGenericObjectWithArray
{
    [self _getObjectRegistry:^(SCValdiMarshallableObjectRegistry *registry) {

        SCValdiMarshallerRegisterTestGenericObject(registry);

        const char *identifiers[] = {
            "SCValdiMarshallerTestGenericObject",
            nil
        };

        SCValdiMarshallableObjectFieldDescriptor fields[] = {
            {.name = "genericObject", .selName = nil, .type = "g:'[0]'<a<s>, a<s>>"},
            {.name = nil}
        };

        [registry registerClass:[SCValdiMarshallerTestGenericObjectParent class] objectDescriptor:SCValdiMarshallableObjectDescriptorMake(fields, identifiers, nil, SCValdiMarshallableObjectTypeClass)];

        SCValdiMarshallerTestGenericObjectParent *testObject = [registry makeObjectOfClass:[SCValdiMarshallerTestGenericObjectParent class]];
        SCValdiMarshallerTestGenericObject *genericObject = [registry makeObjectOfClass:[SCValdiMarshallerTestGenericObject class]];
        genericObject.childObject1 = @[@"Hello", @"World"];
        genericObject.childObject2 = @[@"Goodbye"];
        testObject.genericObject = genericObject;

        NSString *result;
        SCValdiMarshallerScoped(marshaller, {
            NSInteger objectIndex = [registry marshallObject:testObject ofClass:[SCValdiMarshallerTestGenericObjectParent class] toMarshaller:marshaller];
            result = SCValdiMarshallerToString(marshaller, objectIndex, NO);
        });

        XCTAssertEqualObjects(@"<typedObject SCValdiMarshallerTestGenericObjectParent: { genericObject: <typedObject SCValdiMarshallerTestGenericObject: { childObject1: [ Hello, World ], childObject2: [ Goodbye ]} >} >", result);
    }];
}

- (void)testCanUnmarshallGenericObjectWithArray
{
    [self _getObjectRegistry:^(SCValdiMarshallableObjectRegistry *registry) {

        SCValdiMarshallerRegisterTestGenericObject(registry);

        const char *identifiers[] = {
            "SCValdiMarshallerTestGenericObject",
            nil
        };

        SCValdiMarshallableObjectFieldDescriptor fields[] = {
            {.name = "genericObject", .selName = nil, .type = "g:'[0]'<a<s>, a<s>>"},
            {.name = nil}
        };

        [registry registerClass:[SCValdiMarshallerTestGenericObjectParent class] objectDescriptor:SCValdiMarshallableObjectDescriptorMake(fields, identifiers, nil, SCValdiMarshallableObjectTypeClass)];

        SCValdiMarshallerTestGenericObjectParent *object;

        SCValdiMarshallerScoped(marshaller, {
            NSInteger idx = SCValdiMarshallerPushMap(marshaller, 1);
            NSInteger innerIndex = SCValdiMarshallerPushMap(marshaller, 2);

            NSInteger firstArrayIndex = SCValdiMarshallerPushArray(marshaller, 2);
            SCValdiMarshallerPushString(marshaller, @"Hello");
            SCValdiMarshallerSetArrayItem(marshaller, firstArrayIndex, 0);
            SCValdiMarshallerPushString(marshaller, @"World");
            SCValdiMarshallerSetArrayItem(marshaller, firstArrayIndex, 1);
            SCValdiMarshallerPutMapPropertyUninterned(marshaller, @"childObject1", innerIndex);

            SCValdiMarshallerPushArray(marshaller, 0);
            SCValdiMarshallerPutMapPropertyUninterned(marshaller, @"childObject2", innerIndex);

            SCValdiMarshallerPutMapPropertyUninterned(marshaller, @"genericObject" , idx);

            object = [registry unmarshallObjectOfClass:[SCValdiMarshallerTestGenericObjectParent class] fromMarshaller:marshaller atIndex:idx];
        });

        XCTAssertTrue([object isKindOfClass:[SCValdiMarshallerTestGenericObjectParent class]]);
        XCTAssertTrue([object.genericObject isKindOfClass:[SCValdiMarshallerTestGenericObject class]]);
        NSArray *expectedFirstArray = @[@"Hello", @"World"];
        XCTAssertEqualObjects(expectedFirstArray, [object.genericObject childObject1]);
        NSArray *expectedSecondArray = @[];
        XCTAssertEqualObjects(expectedSecondArray, [object.genericObject childObject2]);
    }];
}

- (void)testCanMarshallGenericObjectWithArrayOfOptionals
{
    [self _getObjectRegistry:^(SCValdiMarshallableObjectRegistry *registry) {

        SCValdiMarshallerRegisterTestGenericObject(registry);

        const char *identifiers[] = {
            "SCValdiMarshallerTestGenericObject",
            nil
        };

        SCValdiMarshallableObjectFieldDescriptor fields[] = {
            {.name = "genericObject", .selName = nil, .type = "g:'[0]'<a<s?>, a<s?>>"},
            {.name = nil}
        };

        [registry registerClass:[SCValdiMarshallerTestGenericObjectParent class] objectDescriptor:SCValdiMarshallableObjectDescriptorMake(fields, identifiers, nil, SCValdiMarshallableObjectTypeClass)];

        SCValdiMarshallerTestGenericObjectParent *testObject = [registry makeObjectOfClass:[SCValdiMarshallerTestGenericObjectParent class]];
        SCValdiMarshallerTestGenericObject *genericObject = [registry makeObjectOfClass:[SCValdiMarshallerTestGenericObject class]];
        genericObject.childObject1 = @[@"Hello", [NSNull null], @"World"];
        genericObject.childObject2 = @[[NSNull null]];
        testObject.genericObject = genericObject;

        NSString *result;
        SCValdiMarshallerScoped(marshaller, {
            NSInteger objectIndex = [registry marshallObject:testObject ofClass:[SCValdiMarshallerTestGenericObjectParent class] toMarshaller:marshaller];
            result = SCValdiMarshallerToString(marshaller, objectIndex, NO);
        });

        XCTAssertEqualObjects(@"<typedObject SCValdiMarshallerTestGenericObjectParent: { genericObject: <typedObject SCValdiMarshallerTestGenericObject: { childObject1: [ Hello, <undefined>, World ], childObject2: [ <undefined> ]} >} >", result);
    }];
}

- (void)testCanUnmarshallGenericObjectWithArrayOfOptionals
{
    [self _getObjectRegistry:^(SCValdiMarshallableObjectRegistry *registry) {

        SCValdiMarshallerRegisterTestGenericObject(registry);

        const char *identifiers[] = {
            "SCValdiMarshallerTestGenericObject",
            nil
        };

        SCValdiMarshallableObjectFieldDescriptor fields[] = {
            {.name = "genericObject", .selName = nil, .type = "g:'[0]'<a<s?>, a<s?>>"},
            {.name = nil}
        };

        [registry registerClass:[SCValdiMarshallerTestGenericObjectParent class] objectDescriptor:SCValdiMarshallableObjectDescriptorMake(fields, identifiers, nil, SCValdiMarshallableObjectTypeClass)];

        SCValdiMarshallerTestGenericObjectParent *object;

        SCValdiMarshallerScoped(marshaller, {
            NSInteger idx = SCValdiMarshallerPushMap(marshaller, 1);
            NSInteger innerIndex = SCValdiMarshallerPushMap(marshaller, 2);

            NSInteger firstArrayIndex = SCValdiMarshallerPushArray(marshaller, 3);
            SCValdiMarshallerPushString(marshaller, @"Hello");
            SCValdiMarshallerSetArrayItem(marshaller, firstArrayIndex, 0);
            SCValdiMarshallerPushUndefined(marshaller);
            SCValdiMarshallerSetArrayItem(marshaller, firstArrayIndex, 1);
            SCValdiMarshallerPushString(marshaller, @"World");
            SCValdiMarshallerSetArrayItem(marshaller, firstArrayIndex, 2);
            SCValdiMarshallerPutMapPropertyUninterned(marshaller, @"childObject1", innerIndex);

            SCValdiMarshallerPushArray(marshaller, 0);
            SCValdiMarshallerPutMapPropertyUninterned(marshaller, @"childObject2", innerIndex);

            SCValdiMarshallerPutMapPropertyUninterned(marshaller, @"genericObject" , idx);

            object = [registry unmarshallObjectOfClass:[SCValdiMarshallerTestGenericObjectParent class] fromMarshaller:marshaller atIndex:idx];
        });

        XCTAssertTrue([object isKindOfClass:[SCValdiMarshallerTestGenericObjectParent class]]);
        XCTAssertTrue([object.genericObject isKindOfClass:[SCValdiMarshallerTestGenericObject class]]);
        NSArray *expectedFirstArray = @[@"Hello", [NSNull null], @"World"];
        XCTAssertEqualObjects(expectedFirstArray, [object.genericObject childObject1]);
        NSArray *expectedSecondArray = @[];
        XCTAssertEqualObjects(expectedSecondArray, [object.genericObject childObject2]);
    }];
}

- (void)testCanMarshallGenericObjectWithBlock
{
    [self _getObjectRegistry:^(SCValdiMarshallableObjectRegistry *registry) {

        SCValdiMarshallerRegisterTestGenericObject(registry);

        const char *identifiers[] = {
            "SCValdiMarshallerTestGenericObject",
            nil
        };

        SCValdiMarshallableObjectFieldDescriptor fields[] = {
            {.name = "genericObject", .selName = nil, .type = "g:'[0]'<f(s, d): s, f(s, d): s>"},
            {.name = nil}
        };

        SCValdiMarshallableObjectBlockSupport blockSupport[] = {
            {.typeEncoding = "ood:o", .invoker = &invokeBlock, .factory = &createBlock },
            { .typeEncoding = nil }
        };

        [registry registerClass:[SCValdiMarshallerTestGenericObjectParent class] objectDescriptor:SCValdiMarshallableObjectDescriptorMake(fields, identifiers, blockSupport, SCValdiMarshallableObjectTypeClass)];

        SCValdiMarshallerTestGenericObjectParent *testObject = [registry makeObjectOfClass:[SCValdiMarshallerTestGenericObjectParent class]];
        SCValdiMarshallerTestGenericObject *genericObject = [registry makeObjectOfClass:[SCValdiMarshallerTestGenericObject class]];
        __block NSString *submittedStr = nil;
        __block NSNumber *submittedI = nil;
        id block = ^NSString *(NSString *str, double i) {
            submittedStr = str;
            submittedI = @(i);

            return @"A Result";
        };
        genericObject.childObject1 = block;
        genericObject.childObject2 = block;
        testObject.genericObject = genericObject;

        NSDictionary *untypedMarshalledResult;
        SCValdiMarshallerScoped(marshaller, {
            NSInteger objectIndex = [registry marshallObject:testObject ofClass:[SCValdiMarshallerTestGenericObjectParent class] toMarshaller:marshaller];
            untypedMarshalledResult = SCValdiMarshallerGetUntyped(marshaller, objectIndex);
        });

        XCTAssertTrue([untypedMarshalledResult isKindOfClass:[NSDictionary class]]);

        id<SCValdiFunction> function = untypedMarshalledResult[@"genericObject"][@"childObject1"];

        XCTAssertTrue([function respondsToSelector:@selector(performWithMarshaller:)]);

        XCTAssertNil(submittedStr);
        XCTAssertNil(submittedI);

        id callResult;

        SCValdiMarshallerScoped(marshaller2, {
            SCValdiMarshallerPushString(marshaller2, @"Hello");
            SCValdiMarshallerPushDouble(marshaller2, 42.0);
            if ([function performWithMarshaller:marshaller2]) {
                callResult = SCValdiMarshallerGetUntyped(marshaller2, -1);
            }
        });

        XCTAssertNotNil(submittedStr);
        XCTAssertNotNil(submittedI);

        XCTAssertEqualObjects(@"Hello", submittedStr);
        XCTAssertEqualObjects(submittedI, @(42));

        XCTAssertNotNil(callResult);
        XCTAssertEqualObjects(@"A Result", callResult);
    }];
}

- (void)testCanUnmarshallGenericObjectWithBlock
{
    [self _getObjectRegistry:^(SCValdiMarshallableObjectRegistry *registry) {

        SCValdiMarshallerRegisterTestGenericObject(registry);

        const char *identifiers[] = {
            "SCValdiMarshallerTestGenericObject",
            nil
        };

        SCValdiMarshallableObjectFieldDescriptor fields[] = {
            {.name = "genericObject", .selName = nil, .type = "g:'[0]'<f(s, d): s, f(s, d): s>"},
            {.name = nil}
        };

        SCValdiMarshallableObjectBlockSupport blockSupport[] = {
            {.typeEncoding = "ood:o", .invoker = &invokeBlock, .factory = &createBlock },
            { .typeEncoding = nil }
        };

        [registry registerClass:[SCValdiMarshallerTestGenericObjectParent class] objectDescriptor:SCValdiMarshallableObjectDescriptorMake(fields, identifiers, blockSupport, SCValdiMarshallableObjectTypeClass)];

        __block NSString *submittedStr = nil;
        __block NSNumber *submittedI = nil;
        id<SCValdiFunction> function1 = [SCValdiFunctionWithBlock functionWithBlock:^BOOL(SCValdiMarshallerRef  _Nonnull parameters) {
            submittedStr = SCValdiMarshallerGetString(parameters, 0);
            submittedI = SCValdiMarshallerGetOptionalDouble(parameters, 1);

            SCValdiMarshallerPushString(parameters, @"First Result");
            return YES;
        }];

        id<SCValdiFunction> function2 = [SCValdiFunctionWithBlock functionWithBlock:^BOOL(SCValdiMarshallerRef  _Nonnull parameters) {
            submittedStr = SCValdiMarshallerGetString(parameters, 0);
            submittedI = SCValdiMarshallerGetOptionalDouble(parameters, 1);

            SCValdiMarshallerPushString(parameters, @"Second Result");
            return YES;
        }];

        SCValdiMarshallerTestGenericObjectParent *object;

        SCValdiMarshallerScoped(marshaller, {
            NSInteger idx = SCValdiMarshallerPushMap(marshaller, 1);
            NSInteger innerIndex = SCValdiMarshallerPushMap(marshaller, 2);

            SCValdiMarshallerPushFunction(marshaller, function1);
            SCValdiMarshallerPutMapPropertyUninterned(marshaller, @"childObject1", innerIndex);

            SCValdiMarshallerPushFunction(marshaller, function2);
            SCValdiMarshallerPutMapPropertyUninterned(marshaller, @"childObject2", innerIndex);

            SCValdiMarshallerPutMapPropertyUninterned(marshaller, @"genericObject" , idx);

            object = [registry unmarshallObjectOfClass:[SCValdiMarshallerTestGenericObjectParent class] fromMarshaller:marshaller atIndex:idx];
        });

        XCTAssertTrue([object isKindOfClass:[SCValdiMarshallerTestGenericObjectParent class]]);
        XCTAssertTrue([object.genericObject isKindOfClass:[SCValdiMarshallerTestGenericObject class]]);

        XCTAssertNotNil([object.genericObject childObject1]);
        XCTAssertNotNil([object.genericObject childObject2]);

        SCValdiMarshallerTestBlock block1 = object.genericObject.childObject1;
        NSString *callResult1 = block1(@"Hello1", 42.0);

        XCTAssertNotNil(submittedStr);
        XCTAssertNotNil(submittedI);
        XCTAssertNotNil(callResult1);

        XCTAssertEqualObjects(@"Hello1", submittedStr);
        XCTAssertEqualObjects(submittedI, @(42));
        XCTAssertEqualObjects(@"First Result", callResult1);

        SCValdiMarshallerTestBlock block2 = object.genericObject.childObject2;
        NSString *callResult2 = block2(@"Hello2", 84.0);

        XCTAssertNotNil(submittedStr);
        XCTAssertNotNil(submittedI);
        XCTAssertNotNil(callResult2);

        XCTAssertEqualObjects(@"Hello2", submittedStr);
        XCTAssertEqualObjects(submittedI, @(84));
        XCTAssertEqualObjects(@"Second Result", callResult2);
    }];
}

- (void)testCanMarshallGenericObjectWithBlockWithBuiltinSupport
{
    [self _getObjectRegistry:^(SCValdiMarshallableObjectRegistry *registry) {

        SCValdiMarshallerRegisterTestGenericObject(registry);

        const char *identifiers[] = {
            "SCValdiMarshallerTestGenericObject",
            nil
        };

        SCValdiMarshallableObjectFieldDescriptor fields[] = {
            {.name = "genericObject", .selName = nil, .type = "g:'[0]'<f(s), f(s)>"},
            {.name = nil}
        };

        [registry registerClass:[SCValdiMarshallerTestGenericObjectParent class] objectDescriptor:SCValdiMarshallableObjectDescriptorMake(fields, identifiers, nil, SCValdiMarshallableObjectTypeClass)];

        SCValdiMarshallerTestGenericObjectParent *testObject = [registry makeObjectOfClass:[SCValdiMarshallerTestGenericObjectParent class]];
        SCValdiMarshallerTestGenericObject *genericObject = [registry makeObjectOfClass:[SCValdiMarshallerTestGenericObject class]];
        __block NSString *submittedStr = nil;
        id block = ^(NSString *str) {
            submittedStr = str;
        };
        genericObject.childObject1 = block;
        genericObject.childObject2 = block;
        testObject.genericObject = genericObject;

        NSDictionary *untypedMarshalledResult;
        SCValdiMarshallerScoped(marshaller, {
            NSInteger objectIndex = [registry marshallObject:testObject ofClass:[SCValdiMarshallerTestGenericObjectParent class] toMarshaller:marshaller];
            untypedMarshalledResult = SCValdiMarshallerGetUntyped(marshaller, objectIndex);
        });

        XCTAssertTrue([untypedMarshalledResult isKindOfClass:[NSDictionary class]]);

        id<SCValdiFunction> function = untypedMarshalledResult[@"genericObject"][@"childObject1"];

        XCTAssertTrue([function respondsToSelector:@selector(performWithMarshaller:)]);

        XCTAssertNil(submittedStr);

        id callResult;

        SCValdiMarshallerScoped(marshaller2, {
            SCValdiMarshallerPushString(marshaller2, @"Hello");
            if ([function performWithMarshaller:marshaller2]) {
                callResult = SCValdiMarshallerGetUntyped(marshaller2, -1);
            }
        });

        XCTAssertEqual(callResult, [SCValdiUndefinedValue undefined]);
        XCTAssertNotNil(submittedStr);

        XCTAssertEqualObjects(@"Hello", submittedStr);
    }];
}

- (void)testCanUnmarshallGenericObjectWithBlockWithBuiltinSupport
{
    [self _getObjectRegistry:^(SCValdiMarshallableObjectRegistry *registry) {

        SCValdiMarshallerRegisterTestGenericObject(registry);

        const char *identifiers[] = {
            "SCValdiMarshallerTestGenericObject",
            nil
        };

        SCValdiMarshallableObjectFieldDescriptor fields[] = {
            {.name = "genericObject", .selName = nil, .type = "g:'[0]'<f(s), f(s)>"},
            {.name = nil}
        };

        [registry registerClass:[SCValdiMarshallerTestGenericObjectParent class] objectDescriptor:SCValdiMarshallableObjectDescriptorMake(fields, identifiers, nil, SCValdiMarshallableObjectTypeClass)];

        __block NSString *submittedStr;

        id<SCValdiFunction> function1 = [SCValdiFunctionWithBlock functionWithBlock:^BOOL(SCValdiMarshallerRef  _Nonnull parameters) {
            submittedStr = SCValdiMarshallerGetString(parameters, 0);
            return YES;
        }];

        id<SCValdiFunction> function2 = [SCValdiFunctionWithBlock functionWithBlock:^BOOL(SCValdiMarshallerRef  _Nonnull parameters) {
            submittedStr = SCValdiMarshallerGetString(parameters, 0);
            return YES;
        }];

        SCValdiMarshallerTestGenericObjectParent *object;

        SCValdiMarshallerScoped(marshaller, {
            NSInteger idx = SCValdiMarshallerPushMap(marshaller, 1);
            NSInteger innerIndex = SCValdiMarshallerPushMap(marshaller, 2);

            SCValdiMarshallerPushFunction(marshaller, function1);
            SCValdiMarshallerPutMapPropertyUninterned(marshaller, @"childObject1", innerIndex);

            SCValdiMarshallerPushFunction(marshaller, function2);
            SCValdiMarshallerPutMapPropertyUninterned(marshaller, @"childObject2", innerIndex);

            SCValdiMarshallerPutMapPropertyUninterned(marshaller, @"genericObject" , idx);

            object = [registry unmarshallObjectOfClass:[SCValdiMarshallerTestGenericObjectParent class] fromMarshaller:marshaller atIndex:idx];
        });

        XCTAssertTrue([object isKindOfClass:[SCValdiMarshallerTestGenericObjectParent class]]);
        XCTAssertTrue([object.genericObject isKindOfClass:[SCValdiMarshallerTestGenericObject class]]);

        XCTAssertNotNil([object.genericObject childObject1]);
        XCTAssertNotNil([object.genericObject childObject2]);

        SCValdiMarshallerBuiltinBlock block1 = object.genericObject.childObject1;
        block1(@"Hello1");

        XCTAssertNotNil(submittedStr);

        XCTAssertEqualObjects(@"Hello1", submittedStr);

        SCValdiMarshallerBuiltinBlock block2 = object.genericObject.childObject2;
        block2(@"Hello2");

        XCTAssertNotNil(submittedStr);

        XCTAssertEqualObjects(@"Hello2", submittedStr);
    }];
}

- (void)testCanMarshallNestedGenericObject
{
    [self _getObjectRegistry:^(SCValdiMarshallableObjectRegistry *registry) {

        SCValdiMarshallerRegisterTestGenericObject(registry);

        SCValdiMarshallableObjectFieldDescriptor fields[] = {
            {.name = "genericObject", .selName = nil, .type = "g:'SCValdiMarshallerTestGenericObject'<s, g:'SCValdiMarshallerTestGenericObject'<g:'SCValdiMarshallerTestGenericObject'<s, s>, s>>"},
            {.name = nil}
        };

        [registry registerClass:[SCValdiMarshallerTestGenericObjectParent class] objectDescriptor:SCValdiMarshallableObjectDescriptorMake(fields, nil, nil, SCValdiMarshallableObjectTypeClass)];

        SCValdiMarshallerTestGenericObject *innerGenericObject = [registry makeObjectOfClass:[SCValdiMarshallerTestGenericObject class]];
        innerGenericObject.childObject1 = @"Hello";
        innerGenericObject.childObject2 = @"World";

        SCValdiMarshallerTestGenericObject *outerGenericObject = [registry makeObjectOfClass:[SCValdiMarshallerTestGenericObject class]];
        outerGenericObject.childObject1 = innerGenericObject;
        outerGenericObject.childObject2 = @"!";

        SCValdiMarshallerTestGenericObject *mainGenericObject = [registry makeObjectOfClass:[SCValdiMarshallerTestGenericObject class]];
        mainGenericObject.childObject1 = @">";
        mainGenericObject.childObject2 = outerGenericObject;

        SCValdiMarshallerTestGenericObjectParent *testObject = [registry makeObjectOfClass:[SCValdiMarshallerTestGenericObjectParent class]];
        testObject.genericObject = mainGenericObject;

        NSString *result;
        SCValdiMarshallerScoped(marshaller, {
            NSInteger objectIndex = [registry marshallObject:testObject ofClass:[SCValdiMarshallerTestGenericObjectParent class] toMarshaller:marshaller];
            result = SCValdiMarshallerToString(marshaller, objectIndex, NO);
        });

        XCTAssertEqualObjects(@"<typedObject SCValdiMarshallerTestGenericObjectParent: { genericObject: <typedObject SCValdiMarshallerTestGenericObject: { childObject1: >, childObject2: <typedObject SCValdiMarshallerTestGenericObject: { childObject1: <typedObject SCValdiMarshallerTestGenericObject: { childObject1: Hello, childObject2: World} >, childObject2: !} >} >} >", result);
    }];
}

SCValdiFieldValue invokeGenericParameterBlock(const void *function, SCValdiFieldValue* fields) {
    id result = ((id(*)(const void *, const void *))function)(SCValdiFieldValueGetPtr(fields[0]), SCValdiFieldValueGetPtr(fields[1]));

    return SCValdiFieldValueMakeObject(result);
}

id createGenericParameterBlock(SCValdiBlockTrampoline trampoline) {
    return ^NSString *(SCValdiMarshallerTestGenericObject *inputObj) {
        return SCValdiFieldValueGetObject(trampoline(0, SCValdiFieldValueMakeUnretainedObject(inputObj)));
    };
}

- (void)testCanMarshallBlockWithGenericObjectParameter
{
    [self _getObjectRegistry:^(SCValdiMarshallableObjectRegistry *registry) {

        SCValdiMarshallerRegisterTestGenericObject(registry);

        const char *identifiers[] = {
            "SCValdiMarshallerTestGenericObject",
            nil
        };

        SCValdiMarshallableObjectFieldDescriptor fields[] = {
            {.name = "genericObjectParameterBlock", .selName = nil, .type = "f(g:'[0]'<s,s>): s"},
            {.name = nil}
        };

        SCValdiMarshallableObjectBlockSupport blockSupport[] = {
            {.typeEncoding = "oo:o", .invoker = &invokeGenericParameterBlock, .factory = &createGenericParameterBlock},
            {.typeEncoding = nil}
        };

        [registry registerClass:[SCValdiMarshallerTestObject class] objectDescriptor:SCValdiMarshallableObjectDescriptorMake(fields, identifiers, blockSupport, SCValdiMarshallableObjectTypeClass)];

        SCValdiMarshallerTestObject *testObject = [registry makeObjectOfClass:[SCValdiMarshallerTestObject class]];

        __block SCValdiMarshallerTestGenericObject *submittedGenericObject = nil;
        testObject.genericObjectParameterBlock = ^NSString *(SCValdiMarshallerTestGenericObject *input) {
            submittedGenericObject = input;

            return @"A Result";
        };

        NSDictionary *result;

        SCValdiMarshallerScoped(marshaller, {
            NSInteger objectIndex = [registry marshallObject:testObject toMarshaller:marshaller];
            result = SCValdiMarshallerGetUntyped(marshaller, objectIndex);
        });

        XCTAssertTrue([result isKindOfClass:[NSDictionary class]]);

        id<SCValdiFunction> function = result[@"genericObjectParameterBlock"];

        XCTAssertTrue([function respondsToSelector:@selector(performWithMarshaller:)]);

        XCTAssertNil(submittedGenericObject);

        id callResult;

        SCValdiMarshallerScoped(marshaller2, {
            NSInteger idx = SCValdiMarshallerPushMap(marshaller2, 1);

            SCValdiMarshallerPushString(marshaller2, @"child1");
            SCValdiMarshallerPutMapPropertyUninterned(marshaller2, @"childObject1", idx);
            SCValdiMarshallerPushString(marshaller2, @"child2");
            SCValdiMarshallerPutMapPropertyUninterned(marshaller2, @"childObject2", idx);

            if ([function performWithMarshaller:marshaller2]) {
                callResult = SCValdiMarshallerGetUntyped(marshaller2, -1);
            }
        });


        XCTAssertNotNil(submittedGenericObject);

        XCTAssertEqualObjects(@"child1", submittedGenericObject.childObject1);
        XCTAssertEqualObjects(@"child2", submittedGenericObject.childObject2);

        XCTAssertNotNil(callResult);
        XCTAssertEqualObjects(@"A Result", callResult);
    }];
}

- (void)testCanUnmarshallBlockWithGenericObjectParameter
{
    [self _getObjectRegistry:^(SCValdiMarshallableObjectRegistry *registry) {

        SCValdiMarshallerRegisterTestGenericObject(registry);

        const char *identifiers[] = {
            "SCValdiMarshallerTestGenericObject",
            nil
        };

        SCValdiMarshallableObjectFieldDescriptor fields[] = {
            {.name = "genericObjectParameterBlock", .selName = nil, .type = "f(g:'[0]'<s,s>): s"},
            {.name = nil}
        };

        SCValdiMarshallableObjectBlockSupport blockSupport[] = {
            {.typeEncoding = "oo:o", .invoker = &invokeGenericParameterBlock, .factory = &createGenericParameterBlock},
            {.typeEncoding = nil}
        };

        [registry registerClass:[SCValdiMarshallerTestObject class] objectDescriptor:SCValdiMarshallableObjectDescriptorMake(fields, identifiers, blockSupport, SCValdiMarshallableObjectTypeClass)];

        __block NSDictionary *submittedObject = nil;
        id<SCValdiFunction> function = [SCValdiFunctionWithBlock functionWithBlock:^BOOL(SCValdiMarshallerRef  _Nonnull parameters) {
            submittedObject = SCValdiMarshallerGetUntyped(parameters, 0);

            SCValdiMarshallerPushString(parameters, @"A Result");
            return YES;
        }];

        SCValdiMarshallerTestObject *object;

        SCValdiMarshallerScoped(marshaller, {
            NSInteger idx = SCValdiMarshallerPushMap(marshaller, 1);

            SCValdiMarshallerPushFunction(marshaller, function);
            SCValdiMarshallerPutMapPropertyUninterned(marshaller, @"genericObjectParameterBlock", idx);

            object = [registry unmarshallObjectOfClass:[SCValdiMarshallerTestObject class] fromMarshaller:marshaller atIndex:idx];
        });

        XCTAssertTrue([object isKindOfClass:[SCValdiMarshallerTestObject class]]);
        XCTAssertNotNil(object.genericObjectParameterBlock);

        SCValdiMarshallerTestGenericObject *inputObject = [registry makeObjectOfClass:[SCValdiMarshallerTestGenericObject class]];
        inputObject.childObject1 = @"child1";
        inputObject.childObject2 = @"child2";

        NSString *callResult = object.genericObjectParameterBlock(inputObject);

        XCTAssertNotNil(submittedObject);
        XCTAssertNotNil(callResult);

        XCTAssertEqualObjects(@"child1", submittedObject[@"childObject1"]);
        XCTAssertEqualObjects(@"child2", submittedObject[@"childObject2"]);

        XCTAssertEqualObjects(@"A Result", callResult);
    }];
}

SCValdiFieldValue invokeGenericReturnValueBlock(const void *function, SCValdiFieldValue* fields) {
    id result = ((id(*)(const void *, const void *, double))function)(SCValdiFieldValueGetPtr(fields[0]), SCValdiFieldValueGetPtr(fields[1]), SCValdiFieldValueGetDouble(fields[2]));

    return SCValdiFieldValueMakeObject(result);
}

id createGenericReturnValueBlock(SCValdiBlockTrampoline trampoline) {
    return ^SCValdiMarshallerTestGenericObject *(NSString *str, double d) {
        return SCValdiFieldValueGetObject(trampoline(0, SCValdiFieldValueMakeUnretainedObject(str), SCValdiFieldValueMakeDouble(d)));
    };
}

- (void)testCanMarshallBlockWithGenericObjectReturnValue
{
    [self _getObjectRegistry:^(SCValdiMarshallableObjectRegistry *registry) {

        SCValdiMarshallerRegisterTestGenericObject(registry);

        SCValdiMarshallableObjectFieldDescriptor fields[] = {
            {.name = "genericObjectReturningBlock", .selName = nil, .type = "f(s,d): g:'[0]'<s, s>"},
            {.name = nil}
        };

        const char *identifiers[] = {
            "SCValdiMarshallerTestGenericObject",
            nil
        };

        SCValdiMarshallableObjectBlockSupport blockSupport[] = {
            {.typeEncoding = "ood:o", .invoker = &invokeGenericReturnValueBlock, .factory = &createGenericReturnValueBlock},
            {.typeEncoding = nil}
        };

        [registry registerClass:[SCValdiMarshallerTestObject class] objectDescriptor:SCValdiMarshallableObjectDescriptorMake(fields, identifiers, blockSupport, SCValdiMarshallableObjectTypeClass)];

        SCValdiMarshallerTestObject *testObject = [registry makeObjectOfClass:[SCValdiMarshallerTestObject class]];

        SCValdiMarshallerTestGenericObject *returnObject = [registry makeObjectOfClass:[SCValdiMarshallerTestGenericObject class]];
        returnObject.childObject1 = @"child1";
        returnObject.childObject2 = @"child2";

        __block NSString *submittedStr = nil;
        __block NSNumber *submittedI = nil;
        testObject.genericObjectReturningBlock = ^SCValdiMarshallerTestGenericObject *(NSString *str, double i) {
            submittedStr = str;
            submittedI = @(i);

            return returnObject;
        };

        NSDictionary *result;

        SCValdiMarshallerScoped(marshaller, {
            NSInteger objectIndex = [registry marshallObject:testObject toMarshaller:marshaller];
            result = SCValdiMarshallerGetUntyped(marshaller, objectIndex);
        });

        XCTAssertTrue([result isKindOfClass:[NSDictionary class]]);

        id<SCValdiFunction> function = result[@"genericObjectReturningBlock"];

        XCTAssertTrue([function respondsToSelector:@selector(performWithMarshaller:)]);

        XCTAssertNil(submittedStr);
        XCTAssertNil(submittedI);

        id callResult;

        SCValdiMarshallerScoped(marshaller2, {
            SCValdiMarshallerPushString(marshaller2, @"Hello");
            SCValdiMarshallerPushDouble(marshaller2, 42.0);
            if ([function performWithMarshaller:marshaller2]) {
                callResult = SCValdiMarshallerGetUntyped(marshaller2, -1);
            }
        });

        XCTAssertNotNil(submittedStr);
        XCTAssertNotNil(submittedI);

        XCTAssertEqualObjects(@"Hello", submittedStr);
        XCTAssertEqualObjects(@(42), submittedI);

        XCTAssertNotNil(callResult);
        XCTAssertEqualObjects(@"child1", callResult[@"childObject1"]);
        XCTAssertEqualObjects(@"child2", callResult[@"childObject2"]);
    }];
}

- (void)testCanUnmarshallBlockWithGenericObjectReturnValue
{
    [self _getObjectRegistry:^(SCValdiMarshallableObjectRegistry *registry) {

        SCValdiMarshallerRegisterTestGenericObject(registry);

        const char *identifiers[] = {
            "SCValdiMarshallerTestGenericObject",
            nil
        };

        SCValdiMarshallableObjectFieldDescriptor fields[] = {
            {.name = "genericObjectReturningBlock", .selName = nil, .type = "f(s,d): g:'[0]'<s, s>"},
            {.name = nil}
        };

        SCValdiMarshallableObjectBlockSupport blockSupport[] = {
            {.typeEncoding = "ood:o", .invoker = &invokeGenericReturnValueBlock, .factory = &createGenericReturnValueBlock},
            {.typeEncoding = nil}
        };

        [registry registerClass:[SCValdiMarshallerTestObject class] objectDescriptor:SCValdiMarshallableObjectDescriptorMake(fields, identifiers, blockSupport, SCValdiMarshallableObjectTypeClass)];

        __block NSString *submittedStr = nil;
        __block NSNumber *submittedI = nil;
        id<SCValdiFunction> function = [SCValdiFunctionWithBlock functionWithBlock:^BOOL(SCValdiMarshallerRef  _Nonnull parameters) {
            submittedStr = SCValdiMarshallerGetString(parameters, 0);
            submittedI = SCValdiMarshallerGetOptionalDouble(parameters, 1);

            NSInteger mapIndex = SCValdiMarshallerPushMap(parameters, 2);

            SCValdiMarshallerPushString(parameters, submittedStr);
            SCValdiMarshallerPutMapPropertyUninterned(parameters, @"childObject1", mapIndex);

            SCValdiMarshallerPushString(parameters, @"child2");
            SCValdiMarshallerPutMapPropertyUninterned(parameters, @"childObject2", mapIndex);
            return YES;
        }];

        SCValdiMarshallerTestObject *object;

        SCValdiMarshallerScoped(marshaller, {
            NSInteger idx = SCValdiMarshallerPushMap(marshaller, 1);

            SCValdiMarshallerPushFunction(marshaller, function);
            SCValdiMarshallerPutMapPropertyUninterned(marshaller, @"genericObjectReturningBlock", idx);

            object = [registry unmarshallObjectOfClass:[SCValdiMarshallerTestObject class] fromMarshaller:marshaller atIndex:idx];
        });

        XCTAssertTrue([object isKindOfClass:[SCValdiMarshallerTestObject class]]);
        XCTAssertNotNil(object.genericObjectReturningBlock);

        SCValdiMarshallerTestGenericObject *callResult = object.genericObjectReturningBlock(@"some value", 100);

        XCTAssertNotNil(submittedStr);
        XCTAssertNotNil(submittedI);
        XCTAssertNotNil(callResult);

        XCTAssertEqualObjects(@"some value", callResult.childObject1);
        XCTAssertEqualObjects(@"child2", callResult.childObject2);
    }];
}

- (void)testCanMarshallGenericArrayWithStrings
{
    [self _getObjectRegistry:^(SCValdiMarshallableObjectRegistry *registry) {

        SCValdiMarshallerRegisterTestGenericArray(registry);
        SCValdiMarshallerRegisterTestBox(registry);

        const char *identifiers[] = {
            "SCValdiMarshallerTestGenericArray",
            nil
        };

        SCValdiMarshallableObjectFieldDescriptor fields[] = {
            {.name = "genericArray", .selName = nil, .type = "g:'[0]'<s, s>"},
            {.name = nil}
        };

        [registry registerClass:[SCValdiMarshallerTestGenericArrayParent class] objectDescriptor:SCValdiMarshallableObjectDescriptorMake(fields, identifiers, nil, SCValdiMarshallableObjectTypeClass)];

        SCValdiMarshallerTestGenericArrayParent *testObject = [registry makeObjectOfClass:[SCValdiMarshallerTestGenericArrayParent class]];
        SCValdiMarshallerTestGenericArray *genericArray = [registry makeObjectOfClass:[SCValdiMarshallerTestGenericArray class]];
        SCValdiMarshallerTestBox *childBox1 = [registry makeObjectOfClass:[SCValdiMarshallerTestBox class]];
        childBox1.value = @"Hello";
        genericArray.childArray1 = @[childBox1];
        SCValdiMarshallerTestBox *childBox2 = [registry makeObjectOfClass:[SCValdiMarshallerTestBox class]];
        childBox2.value = @"World";
        genericArray.childArray2 = @[childBox2];
        testObject.genericArray = genericArray;

        NSString *result;
        SCValdiMarshallerScoped(marshaller, {
            NSInteger objectIndex = [registry marshallObject:testObject ofClass:[SCValdiMarshallerTestGenericArrayParent class] toMarshaller:marshaller];
            result = SCValdiMarshallerToString(marshaller, objectIndex, NO);
        });

        XCTAssertEqualObjects(@"<typedObject SCValdiMarshallerTestGenericArrayParent: { genericArray: <typedObject SCValdiMarshallerTestGenericArray: { childArray1: [ <typedObject SCValdiMarshallerTestBox: { value: Hello} > ], childArray2: [ <typedObject SCValdiMarshallerTestBox: { value: World} > ]} >} >", result);
    }];
}

- (void)testCanUnmarshallGenericArrayWithStrings
{
    [self _getObjectRegistry:^(SCValdiMarshallableObjectRegistry *registry) {

        SCValdiMarshallerRegisterTestGenericArray(registry);
        SCValdiMarshallerRegisterTestBox(registry);

        const char *identifiers[] = {
            "SCValdiMarshallerTestGenericArray",
            nil
        };

        SCValdiMarshallableObjectFieldDescriptor fields[] = {
            {.name = "genericArray", .selName = nil, .type = "g:'[0]'<s, s>"},
            {.name = nil}
        };

        [registry registerClass:[SCValdiMarshallerTestGenericArrayParent class] objectDescriptor:SCValdiMarshallableObjectDescriptorMake(fields, identifiers, nil, SCValdiMarshallableObjectTypeClass)];

        SCValdiMarshallerTestGenericArrayParent *object;

        SCValdiMarshallerScoped(marshaller, {
            NSInteger idx = SCValdiMarshallerPushMap(marshaller, 1);
            NSInteger innerIndex = SCValdiMarshallerPushMap(marshaller, 2);

            NSInteger arrayIndex1 = SCValdiMarshallerPushArray(marshaller, 1);
            NSInteger boxIndex1 = SCValdiMarshallerPushMap(marshaller, 1);
            SCValdiMarshallerPushString(marshaller, @"Hello");
            SCValdiMarshallerPutMapPropertyUninterned(marshaller, @"value", boxIndex1);
            SCValdiMarshallerSetArrayItem(marshaller, arrayIndex1, 0);
            SCValdiMarshallerPutMapPropertyUninterned(marshaller, @"childArray1", innerIndex);

            NSInteger arrayIndex2 = SCValdiMarshallerPushArray(marshaller, 1);
            NSInteger boxIndex2 = SCValdiMarshallerPushMap(marshaller, 1);
            SCValdiMarshallerPushString(marshaller, @"World");
            SCValdiMarshallerPutMapPropertyUninterned(marshaller, @"value", boxIndex2);
            SCValdiMarshallerSetArrayItem(marshaller, arrayIndex2, 0);
            SCValdiMarshallerPutMapPropertyUninterned(marshaller, @"childArray2", innerIndex);

            SCValdiMarshallerPutMapPropertyUninterned(marshaller, @"genericArray", idx);

            object = [registry unmarshallObjectOfClass:[SCValdiMarshallerTestGenericArrayParent class] fromMarshaller:marshaller atIndex:idx];
        });

        XCTAssertTrue([object isKindOfClass:[SCValdiMarshallerTestGenericArrayParent class]]);
        XCTAssertTrue([object.genericArray isKindOfClass:[SCValdiMarshallerTestGenericArray class]]);
        XCTAssertTrue([[object.genericArray.childArray1 objectAtIndex:0] isKindOfClass:[SCValdiMarshallerTestBox class]]);
        XCTAssertTrue([[object.genericArray.childArray2 objectAtIndex:0] isKindOfClass:[SCValdiMarshallerTestBox class]]);

        SCValdiMarshallerTestBox *box1 = [object.genericArray.childArray1 objectAtIndex:0];
        SCValdiMarshallerTestBox *box2 = [object.genericArray.childArray2 objectAtIndex:0];
        XCTAssertEqualObjects(@"Hello", box1.value);
        XCTAssertEqualObjects(@"World", box2.value);
    }];
}

- (void)testCanMarshallGenericObjectWithBoxes
{
    [self _getObjectRegistry:^(SCValdiMarshallableObjectRegistry *registry) {

        SCValdiMarshallerRegisterTestBox(registry);
        SCValdiMarshallerRegisterTestBoxes(registry);

        const char *identifiers[] = {
            "SCValdiMarshallerTestBoxes",
            nil
        };
        SCValdiMarshallableObjectFieldDescriptor fields[] = {
            {.name = "genericBoxes", .selName = nil, .type = "g:'[0]'<s, s>"},
            {.name = nil}
        };

        [registry registerClass:[SCValdiMarshallerTestBoxesParent class] objectDescriptor:SCValdiMarshallableObjectDescriptorMake(fields, identifiers, nil, SCValdiMarshallableObjectTypeClass)];

        SCValdiMarshallerTestBoxesParent *testObject = [registry makeObjectOfClass:[SCValdiMarshallerTestBoxesParent class]];
        SCValdiMarshallerTestBoxes *genericBoxes = [registry makeObjectOfClass:[SCValdiMarshallerTestBoxes class]];
        SCValdiMarshallerTestBox *childBox1 = [registry makeObjectOfClass:[SCValdiMarshallerTestBox class]];
        SCValdiMarshallerTestBox *childBox2 = [registry makeObjectOfClass:[SCValdiMarshallerTestBox class]];
        childBox1.value = @"Hello";
        childBox2.value = @"World";
        genericBoxes.childBox1 = childBox1;
        genericBoxes.childBox2 = childBox2;
        testObject.genericBoxes = genericBoxes;

        NSString *result;
        SCValdiMarshallerScoped(marshaller, {
            NSInteger objectIndex = [registry marshallObject:testObject ofClass:[SCValdiMarshallerTestBoxesParent class] toMarshaller:marshaller];
            result = SCValdiMarshallerToString(marshaller, objectIndex, NO);
        });

        XCTAssertEqualObjects(@"<typedObject SCValdiMarshallerTestBoxesParent: { genericBoxes: <typedObject SCValdiMarshallerTestBoxes: { childBox1: <typedObject SCValdiMarshallerTestBox: { value: Hello} >, childBox2: <typedObject SCValdiMarshallerTestBox: { value: World} >} >} >", result);
    }];
}

- (void)testCanUnmarshallGenericObjectWithBoxes
{
    [self _getObjectRegistry:^(SCValdiMarshallableObjectRegistry *registry) {

        SCValdiMarshallerRegisterTestBox(registry);
        SCValdiMarshallerRegisterTestBoxes(registry);

        const char *identifiers[] = {
            "SCValdiMarshallerTestBoxes",
            nil
        };
        SCValdiMarshallableObjectFieldDescriptor fields[] = {
            {.name = "genericBoxes", .selName = nil, .type = "g:'[0]'<s, s>"},
            {.name = nil}
        };

        [registry registerClass:[SCValdiMarshallerTestBoxesParent class] objectDescriptor:SCValdiMarshallableObjectDescriptorMake(fields, identifiers, nil, SCValdiMarshallableObjectTypeClass)];

        SCValdiMarshallerTestBoxesParent *object;

        SCValdiMarshallerScoped(marshaller, {
            NSInteger idx = SCValdiMarshallerPushMap(marshaller, 1);
            NSInteger innerIndex = SCValdiMarshallerPushMap(marshaller, 2);

            NSInteger box1 = SCValdiMarshallerPushMap(marshaller, 1);
            SCValdiMarshallerPushString(marshaller, @"Hello");
            SCValdiMarshallerPutMapPropertyUninterned(marshaller, @"value", box1);
            SCValdiMarshallerPutMapPropertyUninterned(marshaller, @"childBox1", innerIndex);

            NSInteger box2 = SCValdiMarshallerPushMap(marshaller, 1);
            SCValdiMarshallerPushString(marshaller, @"World");
            SCValdiMarshallerPutMapPropertyUninterned(marshaller, @"value", box2);
            SCValdiMarshallerPutMapPropertyUninterned(marshaller, @"childBox2", innerIndex);

            SCValdiMarshallerPutMapPropertyUninterned(marshaller, @"genericBoxes", idx);

            object = [registry unmarshallObjectOfClass:[SCValdiMarshallerTestBoxesParent class] fromMarshaller:marshaller atIndex:idx];
        });

        XCTAssertTrue([object isKindOfClass:[SCValdiMarshallerTestBoxesParent class]]);
        XCTAssertTrue([object.genericBoxes isKindOfClass:[SCValdiMarshallerTestBoxes class]]);
        XCTAssertEqualObjects(@"Hello", [[object.genericBoxes childBox1] value]);
        XCTAssertEqualObjects(@"World", [[object.genericBoxes childBox2] value]);
    }];
}

- (void)testCanMarshallPromiseAndPropagateSuccess
{
    [self _getObjectRegistry:^(SCValdiMarshallableObjectRegistry *registry) {
        [registry registerClass:[SCValdiMarshallerTestPromise class] objectDescriptor:[SCValdiMarshallerTestPromise valdiMarshallableObjectDescriptor]];

        SCValdiMarshallerTestPromise *testObject = [registry makeObjectOfClass:[SCValdiMarshallerTestPromise class]];

        SCValdiResolvablePromise<NSString *> *promise = [SCValdiResolvablePromise new];
        testObject.promise = promise;

        __block NSString *successResult;
        __block NSError *errorResult;

        SCValdiMarshallerScoped(marshaller, {
            NSInteger objectIndex = [registry marshallObject:testObject ofClass:[SCValdiMarshallerTestPromise class] toMarshaller:marshaller];
            NSInteger promiseIndex = SCValdiMarshallerGetTypedObjectProperty(marshaller, 0, objectIndex);

            SCValdiMarshallerOnPromiseComplete(marshaller, promiseIndex, ^BOOL(SCValdiMarshallerRef  _Nonnull parameters) {
                if (SCValdiMarshallerIsError(parameters, 0)) {
                    errorResult = SCValdiMarshallerGetError(parameters, 0);
                } else {
                    successResult = SCValdiMarshallerGetString(parameters, 0);
                }

                return NO;
            });
        });

        XCTAssertNil(successResult);
        XCTAssertNil(errorResult);

        [promise fulfillWithSuccessValue:@"Hello World"];

        XCTAssertNotNil(successResult);
        XCTAssertNil(errorResult);

        XCTAssertEqualObjects(@"Hello World", successResult);
    }];
}

- (void)testCanMarshallPromiseAndPropagateFailure
{
    [self _getObjectRegistry:^(SCValdiMarshallableObjectRegistry *registry) {
        [registry registerClass:[SCValdiMarshallerTestPromise class] objectDescriptor:[SCValdiMarshallerTestPromise valdiMarshallableObjectDescriptor]];

        SCValdiMarshallerTestPromise *testObject = [registry makeObjectOfClass:[SCValdiMarshallerTestPromise class]];

        SCValdiResolvablePromise<NSString *> *promise = [SCValdiResolvablePromise new];
        testObject.promise = promise;

        __block NSString *successResult;
        __block NSError *errorResult;

        SCValdiMarshallerScoped(marshaller, {
            NSInteger objectIndex = [registry marshallObject:testObject ofClass:[SCValdiMarshallerTestPromise class] toMarshaller:marshaller];
            NSInteger promiseIndex = SCValdiMarshallerGetTypedObjectProperty(marshaller, 0, objectIndex);

            SCValdiMarshallerOnPromiseComplete(marshaller, promiseIndex, ^BOOL(SCValdiMarshallerRef  _Nonnull parameters) {
                if (SCValdiMarshallerIsError(parameters, 0)) {
                    errorResult = SCValdiMarshallerGetError(parameters, 0);
                } else {
                    successResult = SCValdiMarshallerGetString(parameters, 0);
                }

                return NO;
            });
        });

        XCTAssertNil(successResult);
        XCTAssertNil(errorResult);

        [promise fulfillWithError:[NSError errorWithDomain:@"" code:0 userInfo:@{NSLocalizedDescriptionKey: @"This is an error"}]];

        XCTAssertNil(successResult);
        XCTAssertNotNil(errorResult);

        XCTAssertEqualObjects(@"This is an error", [errorResult localizedDescription]);
    }];
}

- (void)testCanUnmarshallPromiseAndReceiveSuccess
{
    [self _getObjectRegistry:^(SCValdiMarshallableObjectRegistry *registry) {
        [registry registerClass:[SCValdiMarshallerTestPromise class] objectDescriptor:[SCValdiMarshallerTestPromise valdiMarshallableObjectDescriptor]];

        SCValdiMarshallerTestPromise *testObject = [registry makeObjectOfClass:[SCValdiMarshallerTestPromise class]];

        SCValdiResolvablePromise<NSString *> *promise = [SCValdiResolvablePromise new];
        testObject.promise = promise;

        SCValdiMarshallerTestPromise *unmarshalledTestObject;

        SCValdiMarshallerScoped(marshaller, {
            // Marshall our object holding the promise first
            NSInteger objectIndex = [registry marshallObject:testObject ofClass:[SCValdiMarshallerTestPromise class] toMarshaller:marshaller];
            unmarshalledTestObject = [registry unmarshallObjectOfClass:[SCValdiMarshallerTestPromise class] fromMarshaller:marshaller atIndex:objectIndex];
        });

        XCTAssertNotEqual(testObject, unmarshalledTestObject);
        XCTAssertNotEqual(testObject.promise, unmarshalledTestObject.promise);
        XCTAssertNotNil(unmarshalledTestObject.promise);
        XCTAssertFalse([unmarshalledTestObject.promise isKindOfClass:[SCValdiResolvablePromise class]]);

        __block NSString *successResult;
        __block NSError *errorResult;

        [unmarshalledTestObject.promise onCompleteWithCallbackBlock:^(id  _Nullable value, NSError * _Nullable error) {
            successResult = value;
            errorResult = error;
        }];

        XCTAssertNil(successResult);
        XCTAssertNil(errorResult);

        [promise fulfillWithSuccessValue:@"Hello World"];

        XCTAssertNotNil(successResult);
        XCTAssertNil(errorResult);

        XCTAssertEqualObjects(@"Hello World", successResult);
    }];
}

- (void)testCanUnmarshallPromiseAndReceiveFailure
{
    [self _getObjectRegistry:^(SCValdiMarshallableObjectRegistry *registry) {
        [registry registerClass:[SCValdiMarshallerTestPromise class] objectDescriptor:[SCValdiMarshallerTestPromise valdiMarshallableObjectDescriptor]];

        SCValdiMarshallerTestPromise *testObject = [registry makeObjectOfClass:[SCValdiMarshallerTestPromise class]];

        SCValdiResolvablePromise<NSString *> *promise = [SCValdiResolvablePromise new];
        testObject.promise = promise;

        SCValdiMarshallerTestPromise *unmarshalledTestObject;

        SCValdiMarshallerScoped(marshaller, {
            // Marshall our object holding the promise first
            NSInteger objectIndex = [registry marshallObject:testObject ofClass:[SCValdiMarshallerTestPromise class] toMarshaller:marshaller];
            unmarshalledTestObject = [registry unmarshallObjectOfClass:[SCValdiMarshallerTestPromise class] fromMarshaller:marshaller atIndex:objectIndex];
        });

        XCTAssertNotEqual(testObject, unmarshalledTestObject);
        XCTAssertNotEqual(testObject.promise, unmarshalledTestObject.promise);
        XCTAssertNotNil(unmarshalledTestObject.promise);
        XCTAssertFalse([unmarshalledTestObject.promise isKindOfClass:[SCValdiResolvablePromise class]]);

        __block NSString *successResult;
        __block NSError *errorResult;

        [unmarshalledTestObject.promise onCompleteWithCallbackBlock:^(id  _Nullable value, NSError * _Nullable error) {
            successResult = value;
            errorResult = error;
        }];

        XCTAssertNil(successResult);
        XCTAssertNil(errorResult);

        [promise fulfillWithError:[NSError errorWithDomain:@"" code:0 userInfo:@{NSLocalizedDescriptionKey: @"This is an error"}]];

        XCTAssertNil(successResult);
        XCTAssertNotNil(errorResult);

        XCTAssertEqualObjects(@"This is an error", [errorResult localizedDescription]);
    }];
}

- (void)testCanForwardPromiseCancel
{
    [self _getObjectRegistry:^(SCValdiMarshallableObjectRegistry *registry) {
        [registry registerClass:[SCValdiMarshallerTestPromise class] objectDescriptor:[SCValdiMarshallerTestPromise valdiMarshallableObjectDescriptor]];

        SCValdiMarshallerTestPromise *testObject = [registry makeObjectOfClass:[SCValdiMarshallerTestPromise class]];

        SCValdiResolvablePromise<NSString *> *promise = [SCValdiResolvablePromise new];
        testObject.promise = promise;

        SCValdiMarshallerTestPromise *unmarshalledTestObject;

        SCValdiMarshallerScoped(marshaller, {
            // Marshall our object holding the promise first
            NSInteger objectIndex = [registry marshallObject:testObject ofClass:[SCValdiMarshallerTestPromise class] toMarshaller:marshaller];
            unmarshalledTestObject = [registry unmarshallObjectOfClass:[SCValdiMarshallerTestPromise class] fromMarshaller:marshaller atIndex:objectIndex];
        });

        XCTAssertNotEqual(testObject.promise, unmarshalledTestObject.promise);

        XCTAssertFalse(promise.isCancelable);
        XCTAssertFalse(unmarshalledTestObject.promise.isCancelable);

        __block BOOL cancelCalled = NO;
        [promise setCancelCallback:^{
            cancelCalled = YES;
        }];
        XCTAssertTrue(promise.isCancelable);
        XCTAssertTrue(unmarshalledTestObject.promise.isCancelable);

        [unmarshalledTestObject.promise cancel];

        XCTAssertTrue(cancelCalled);
    }];
}

- (void)testCanMarshallVoidPromise
{
    [self _getObjectRegistry:^(SCValdiMarshallableObjectRegistry *registry) {
        [registry registerClass:[SCValdiMarshallerTestPromise class] objectDescriptor:[SCValdiMarshallerTestPromise valdiMarshallableObjectDescriptor]];

        SCValdiMarshallerTestPromise *testObject = [registry makeObjectOfClass:[SCValdiMarshallerTestPromise class]];

        SCValdiResolvablePromise<SCValdiUndefinedValue *> *promise = [SCValdiResolvablePromise new];
        testObject.voidPromise = promise;

        __block NSString *onPromiseCompleteResult;

        SCValdiMarshallerScoped(marshaller, {
            NSInteger objectIndex = [registry marshallObject:testObject ofClass:[SCValdiMarshallerTestPromise class] toMarshaller:marshaller];
            NSInteger promiseIndex = SCValdiMarshallerGetTypedObjectProperty(marshaller, 1, objectIndex);

            SCValdiMarshallerOnPromiseComplete(marshaller, promiseIndex, ^BOOL(SCValdiMarshallerRef  _Nonnull parameters) {
                onPromiseCompleteResult = SCValdiMarshallerToString(parameters, 0, NO);

                return NO;
            });
        });

        XCTAssertNil(onPromiseCompleteResult);

        [promise fulfillWithSuccessValue:[SCValdiUndefinedValue undefined]];

        XCTAssertEqualObjects(@"<undefined>", onPromiseCompleteResult);
    }];
}

- (void)testCanUnmarshallVoidPromise
{
    [self _getObjectRegistry:^(SCValdiMarshallableObjectRegistry *registry) {
        [registry registerClass:[SCValdiMarshallerTestPromise class] objectDescriptor:[SCValdiMarshallerTestPromise valdiMarshallableObjectDescriptor]];

        SCValdiMarshallerTestPromise *testObject = [registry makeObjectOfClass:[SCValdiMarshallerTestPromise class]];

        SCValdiResolvablePromise<SCValdiUndefinedValue *> *promise = [SCValdiResolvablePromise new];
        testObject.voidPromise = promise;

        SCValdiMarshallerTestPromise *unmarshalledTestObject;

        SCValdiMarshallerScoped(marshaller, {
            // Marshall our object holding the promise first
            NSInteger objectIndex = [registry marshallObject:testObject ofClass:[SCValdiMarshallerTestPromise class] toMarshaller:marshaller];
            unmarshalledTestObject = [registry unmarshallObjectOfClass:[SCValdiMarshallerTestPromise class] fromMarshaller:marshaller atIndex:objectIndex];
        });

        __block id successResult;
        __block NSError *errorResult;

        [unmarshalledTestObject.voidPromise onCompleteWithCallbackBlock:^(id  _Nullable value, NSError * _Nullable error) {
            successResult = value;
            errorResult = error;
        }];

        XCTAssertNil(successResult);
        XCTAssertNil(errorResult);

        [promise fulfillWithSuccessValue:[SCValdiUndefinedValue undefined]];

        XCTAssertNotNil(successResult);
        XCTAssertNil(errorResult);

        XCTAssertEqual([SCValdiUndefinedValue undefined], successResult);
    }];
}

- (void)testRetainReleaseBridgedValdiFunction
{
    [self _getObjectRegistry:^(SCValdiMarshallableObjectRegistry *registry) {
        id<SCValdiFunction> function = [SCValdiFunctionWithBlock functionWithBlock:^BOOL(SCValdiMarshallerRef  _Nonnull parameters) {
            return NO;
        }];

        XCTAssertEqual(1, CFGetRetainCount((__bridge CFTypeRef)(function)));

        @autoreleasepool {
            SCValdiMarshallerScoped(marshaller, {
                @autoreleasepool {
                    SCValdiMarshallerPushFunction(marshaller, function);
                }
                XCTAssertEqual(2, CFGetRetainCount((__bridge CFTypeRef)(function)));
            });
        }

        XCTAssertEqual(1, CFGetRetainCount((__bridge CFTypeRef)(function)));
    }];
}

@end

#pragma clang diagnostic pop

