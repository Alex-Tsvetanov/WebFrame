//
//  SCValdiMarshallableObjectUtils.m
//  valdi-ios
//
//  Created by Simon Corsin on 6/5/20.
//

#import "valdi_core/SCValdiMarshallableObjectUtils.h"
#include "SCValdiFieldValue.h"
#import "valdi_core/SCValdiMarshallableObject.h"
#import "valdi_core/SCValdiError.h"

#import <objc/runtime.h>

typedef void (*SCValdiObjectSetter)(id, SEL, id);
typedef void (*SCValdiDoubleSetter)(id, SEL, double);
typedef void (*SCValdiBoolSetter)(id, SEL, BOOL);
typedef void (*SCValdiIntSetter)(id, SEL, NSInteger);

typedef id (*SCValdiObjectGetter)(id, SEL);
typedef double (*SCValdiDoubleGetter)(id, SEL);
typedef BOOL (*SCValdiBoolGetter)(id, SEL);
typedef int32_t (*SCValdiIntGetter)(id, SEL);
typedef int64_t (*SCValdiLongGetter)(id, SEL);

SEL SCValdiMakeSetterSelector(const char *getterMethodName) {
    unsigned long len = strlen(getterMethodName);

    char *selectorName = malloc(3 + len + 2);
    if (!selectorName) {
        SCValdiErrorThrow(@"Allocation failure");
    }

    char *selectorNamePtr = selectorName;
    (*selectorNamePtr++) = 's';
    (*selectorNamePtr++) = 'e';
    (*selectorNamePtr++) = 't';
    (*selectorNamePtr++) = toupper(getterMethodName[0]);
    for (unsigned long i = 1; i < len; i++) {
        (*selectorNamePtr++) = getterMethodName[i];
    }
    (*selectorNamePtr++) = ':';
    (*selectorNamePtr) = 0;

    SEL sel = sel_registerName(selectorName);
    free(selectorName);
    return sel;
}

extern char SCValdiCTypeCharForFieldType(SCValdiFieldValueType fieldType) {
    switch (fieldType) {
        case SCValdiFieldValueTypeVoid:
            return 'v';
        case SCValdiFieldValueTypePtr:
        case SCValdiFieldValueTypeObject:
            return 'o';
        case SCValdiFieldValueTypeDouble:
            return 'd';
        case SCValdiFieldValueTypeBool:
            return 'b';
        case SCValdiFieldValueTypeInt:
            return 'i';
        case SCValdiFieldValueTypeLong:
            return 'l';
    }
}

extern SCValdiFieldValueType SCValdiFieldTypeFromCTypeChar(char c) {
    switch (c) {
        case 'v':
            return SCValdiFieldValueTypeVoid;
        case 'o':
            return SCValdiFieldValueTypeObject;
        case 'd':
            return SCValdiFieldValueTypeDouble;
        case 'b':
            return SCValdiFieldValueTypeBool;
        case 'i':
            return SCValdiFieldValueTypeInt;
        case 'l':
            return SCValdiFieldValueTypeLong;
        default:
            SCValdiErrorThrow([NSString stringWithFormat:@"Invalid C type '%c'", c]);
    }
}


SCValdiFieldValue *SCValdiGetMarshallableObjectFieldsStorage(__unsafe_unretained SCValdiMarshallableObject *instance)
{
    static dispatch_once_t onceToken;
    static ptrdiff_t storageOffset;
    dispatch_once(&onceToken, ^{
        Ivar ivar = class_getInstanceVariable([SCValdiMarshallableObject class], "_storage");
        storageOffset = ivar_getOffset(ivar);
    });

    return *(SCValdiFieldValue **)((__bridge void *)instance + storageOffset);
}

void SCValdiSetFieldsStorageObjectValue(SCValdiFieldValue *fieldsStorage, NSUInteger fieldIndex, BOOL isCopyable, __unsafe_unretained id value)
{
    fieldsStorage += fieldIndex;

    if (fieldsStorage->o) {
        CFRelease(fieldsStorage->o);
    }

    if (value) {
        if (isCopyable) {
            id valueCopy = [value copy];
            fieldsStorage->o = CFBridgingRetain(valueCopy);
        } else {
            fieldsStorage->o = CFBridgingRetain(value);
        }
    } else {
        fieldsStorage->o = nil;
    }
}

void SCValdiSetFieldsStorageValue(SCValdiFieldValue *fieldsStorage, NSUInteger fieldIndex, SCValdiFieldValue value)
{
    fieldsStorage[fieldIndex] = value;
}

const char *SCValdiObjCTypeForFieldType(SCValdiFieldValueType fieldType) {
    switch (fieldType) {
        case SCValdiFieldValueTypeVoid:
            return @encode(void);
        case SCValdiFieldValueTypePtr:
            return @encode(const void *);
        case SCValdiFieldValueTypeObject:
            return @encode(id);
        case SCValdiFieldValueTypeDouble:
            return @encode(double);
        case SCValdiFieldValueTypeBool:
            return @encode(BOOL);
        case SCValdiFieldValueTypeInt:
            return @encode(int32_t);
        case SCValdiFieldValueTypeLong:
            return @encode(int64_t);
    }
}

IMP SCValdiResolveMethodImpl(__unsafe_unretained id instance, SEL selector) {
    // TOOD(simon): Maybe cache this?
    return class_getMethodImplementation([instance class], selector);
}

void SCValdiRegisterObjectMethod(Class objectClass, IMP method, SEL selector, const char *returnType, const char *argument)
{
    char types[16];
    snprintf(types, sizeof(types), "%s@:%s", returnType, argument ? argument : "");

    if (!class_addMethod(objectClass, selector, method, types)) {
        class_replaceMethod(objectClass, selector, method, types);
    }
}

void SCValdiAppendFieldSetter(SCValdiFieldValueDescriptor fieldDescriptor, const char *selectorName, NSUInteger fieldIndex, Class objectClass) {
    SEL setterSelector = SCValdiMakeSetterSelector(selectorName);

    switch (fieldDescriptor.type) {
        case SCValdiFieldValueTypeVoid:
            SCValdiErrorThrow(@"Void properties not supported");
            break;
        case SCValdiFieldValueTypePtr:
            SCValdiErrorThrow(@"Raw ptr properties not supported");
            break;
        case SCValdiFieldValueTypeObject: {
            BOOL isCopyable = fieldDescriptor.isCopyable;
            IMP setter = imp_implementationWithBlock(^void(__unsafe_unretained id receiver, __unsafe_unretained id value) {
                SCValdiSetFieldsStorageObjectValue(SCValdiGetMarshallableObjectFieldsStorage(receiver), fieldIndex, isCopyable, value);
             });
            SCValdiRegisterObjectMethod(objectClass, setter, setterSelector, @encode(void), @encode(id));
            break;
        }
        case SCValdiFieldValueTypeDouble: {
            IMP setter = imp_implementationWithBlock(^void(__unsafe_unretained id receiver, double value) {
                SCValdiSetFieldsStorageValue(SCValdiGetMarshallableObjectFieldsStorage(receiver), fieldIndex, SCValdiFieldValueMakeDouble(value));
             });
            SCValdiRegisterObjectMethod(objectClass, setter, setterSelector, @encode(void), @encode(double));
            break;
        }
        case SCValdiFieldValueTypeBool: {
            IMP setter = imp_implementationWithBlock(^void(__unsafe_unretained id receiver, BOOL value) {
                SCValdiSetFieldsStorageValue(SCValdiGetMarshallableObjectFieldsStorage(receiver), fieldIndex, SCValdiFieldValueMakeBool(value));
             });
            SCValdiRegisterObjectMethod(objectClass, setter, setterSelector, @encode(void), @encode(BOOL));
            break;
        }
        case SCValdiFieldValueTypeInt: {
            IMP setter = imp_implementationWithBlock(^void(__unsafe_unretained id receiver, int32_t value) {
                SCValdiSetFieldsStorageValue(SCValdiGetMarshallableObjectFieldsStorage(receiver), fieldIndex, SCValdiFieldValueMakeInt(value));
            });
            SCValdiRegisterObjectMethod(objectClass, setter, setterSelector, @encode(void), @encode(int32_t));
            break;
        }
        case SCValdiFieldValueTypeLong: {
            IMP setter = imp_implementationWithBlock(^void(__unsafe_unretained id receiver, int64_t value) {
                SCValdiSetFieldsStorageValue(SCValdiGetMarshallableObjectFieldsStorage(receiver), fieldIndex, SCValdiFieldValueMakeLong(value));
            });
            SCValdiRegisterObjectMethod(objectClass, setter, setterSelector, @encode(void), @encode(int64_t));
            break;
        }
    }
}

void SCValdiAppendFieldGetter(SCValdiFieldValueDescriptor fieldDescriptor, NSUInteger fieldIndex, Class objectClass) {
    SEL selector = fieldDescriptor.selector;
    switch (fieldDescriptor.type) {
        case SCValdiFieldValueTypeVoid:
            SCValdiErrorThrow(@"Void properties not supported");
            break;
        case SCValdiFieldValueTypePtr:
            SCValdiErrorThrow(@"Raw ptr properties not supported");
            break;
        case SCValdiFieldValueTypeObject: {
            IMP getter = imp_implementationWithBlock(^id(__unsafe_unretained id receiver) {
                return SCValdiFieldValueGetObject(SCValdiGetMarshallableObjectFieldsStorage(receiver)[fieldIndex]);
            });
            SCValdiRegisterObjectMethod(objectClass, getter, selector, @encode(id), nil);
            break;
        }
        case SCValdiFieldValueTypeDouble: {
            IMP getter = imp_implementationWithBlock(^double(__unsafe_unretained id receiver) {
                return SCValdiFieldValueGetDouble(SCValdiGetMarshallableObjectFieldsStorage(receiver)[fieldIndex]);
            });
            SCValdiRegisterObjectMethod(objectClass, getter, selector, @encode(double), nil);
            break;
        }
        case SCValdiFieldValueTypeBool: {
            IMP getter = imp_implementationWithBlock(^BOOL(__unsafe_unretained id receiver) {
                return SCValdiFieldValueGetBool(SCValdiGetMarshallableObjectFieldsStorage(receiver)[fieldIndex]);
            });
            SCValdiRegisterObjectMethod(objectClass, getter, selector, @encode(BOOL), nil);
            break;
        }
        case SCValdiFieldValueTypeInt: {
            IMP getter = imp_implementationWithBlock(^int32_t(__unsafe_unretained id receiver) {
                return SCValdiFieldValueGetInt(SCValdiGetMarshallableObjectFieldsStorage(receiver)[fieldIndex]);
            });

            SCValdiRegisterObjectMethod(objectClass, getter, selector, @encode(int32_t), nil);
            break;
        }
        case SCValdiFieldValueTypeLong: {
            IMP getter = imp_implementationWithBlock(^int64_t(__unsafe_unretained id receiver) {
                return SCValdiFieldValueGetLong(SCValdiGetMarshallableObjectFieldsStorage(receiver)[fieldIndex]);
            });

            SCValdiRegisterObjectMethod(objectClass, getter, selector, @encode(int64_t), nil);
            break;
        }
    }
}

SCValdiFieldValue SCValdiGetFieldValueFromBridgedObject(__unsafe_unretained id objectInstance,
                                                              SCValdiFieldValueDescriptor fieldDescriptor) {
    SEL selector = fieldDescriptor.selector;

    switch (fieldDescriptor.type) {
        case SCValdiFieldValueTypeVoid:
            SCValdiErrorThrow(@"Void properties not supported");
            break;
        case SCValdiFieldValueTypePtr:
            SCValdiErrorThrow(@"Raw ptr properties not supported");
            break;
        case SCValdiFieldValueTypeObject: {
            if (!objectInstance) {
                return SCValdiFieldValueMakeNull();
            }

            if (fieldDescriptor.isOptional && ![objectInstance respondsToSelector:selector]) {
                return SCValdiFieldValueMakeNull();
            }

            IMP getter = SCValdiResolveMethodImpl(objectInstance, selector);
            id value = ((SCValdiObjectGetter)getter)(objectInstance, selector);

            return SCValdiFieldValueMakeObject(value);
        }
        case SCValdiFieldValueTypeDouble: {
            if (!objectInstance) {
                return SCValdiFieldValueMakeDouble(0);
            }

            IMP getter = SCValdiResolveMethodImpl(objectInstance, selector);;
            double value = ((SCValdiDoubleGetter)getter)(objectInstance, selector);

            return SCValdiFieldValueMakeDouble(value);
        }
        case SCValdiFieldValueTypeBool: {
            if (!objectInstance) {
                return SCValdiFieldValueMakeBool(NO);
            }

            IMP getter = SCValdiResolveMethodImpl(objectInstance, selector);
            BOOL value = ((SCValdiBoolGetter)getter)(objectInstance, selector);

            return SCValdiFieldValueMakeBool(value);
        }
        case SCValdiFieldValueTypeInt: {
            if (!objectInstance) {
                return SCValdiFieldValueMakeInt(0);
            }

            IMP getter = SCValdiResolveMethodImpl(objectInstance, selector);
            int32_t value = ((SCValdiIntGetter)getter)(objectInstance, selector);

            return SCValdiFieldValueMakeInt(value);
        }
        case SCValdiFieldValueTypeLong: {
            if (!objectInstance) {
                return SCValdiFieldValueMakeLong(0);
            }

            IMP getter = SCValdiResolveMethodImpl(objectInstance, selector);
            int64_t value = ((SCValdiLongGetter)getter)(objectInstance, selector);

            return SCValdiFieldValueMakeLong(value);
        }
    }
}

SCValdiFieldValue *SCValdiAllocateFieldsStorage(NSUInteger fieldsCount) {
    NSUInteger allocSize = sizeof(SCValdiFieldValue) * fieldsCount;
    void *storage = malloc(allocSize);
    memset(storage, 0, allocSize);
    return (SCValdiFieldValue *)storage;
}

void SCValdiDeallocateFieldsStorage(SCValdiFieldValue *fieldsStorage,
                                       NSUInteger fieldsCount,
                                       const SCValdiFieldValueDescriptor *fieldsDescriptors) {
    for (NSUInteger i = 0; i < fieldsCount; i++) {
        if (fieldsDescriptors[i].type == SCValdiFieldValueTypeObject) {
            if (fieldsStorage[i].o) {
                CFRelease(fieldsStorage[i].o);
            }
        }
    }

    free(fieldsStorage);
}

void SCValdiSetFieldsStorageWithValues(SCValdiFieldValue *fieldsStorage,
                                          NSUInteger fieldsCount,
                                          const SCValdiFieldValueDescriptor *fieldsDescriptors,
                                          const SCValdiFieldValue *values) {
    for (NSUInteger i = 0; i < fieldsCount; i++) {
        if (fieldsDescriptors[i].type == SCValdiFieldValueTypeObject) {
            SCValdiSetFieldsStorageObjectValue(fieldsStorage, i, fieldsDescriptors[i].isCopyable, SCValdiFieldValueGetObject(values[i]));
        } else {
            SCValdiSetFieldsStorageValue(fieldsStorage, i, values[i]);
        }
    }
}

void SCValdiSetFieldsStorageWithValuesList(SCValdiFieldValue *fieldsStorage,
                                                     NSUInteger fieldsCount,
                                                     const SCValdiFieldValueDescriptor *fieldsDescriptors,
                                                     va_list list) {
    for (size_t i = 0; i < fieldsCount; i++) {
        switch (fieldsDescriptors[i].type) {
            case SCValdiFieldValueTypeVoid:
                SCValdiErrorThrow(@"Void properties not supported");
                break;
            case SCValdiFieldValueTypePtr:
                SCValdiErrorThrow(@"Raw ptr properties not supported");
                break;
            case SCValdiFieldValueTypeObject:
                SCValdiSetFieldsStorageObjectValue(fieldsStorage, i, fieldsDescriptors[i].isCopyable, va_arg(list, id));
                break;
            case SCValdiFieldValueTypeDouble:
                SCValdiSetFieldsStorageValue(fieldsStorage, i, SCValdiFieldValueMakeDouble(va_arg(list, double)));
                break;
            case SCValdiFieldValueTypeBool:
                SCValdiSetFieldsStorageValue(fieldsStorage, i, SCValdiFieldValueMakeBool(va_arg(list, int))); // BOOL gets promoted to int in C
                break;
            case SCValdiFieldValueTypeInt:
                SCValdiSetFieldsStorageValue(fieldsStorage, i, SCValdiFieldValueMakeInt(va_arg(list, int32_t)));
                break;
            case SCValdiFieldValueTypeLong:
                SCValdiSetFieldsStorageValue(fieldsStorage, i, SCValdiFieldValueMakeLong(va_arg(list, int64_t)));
                break;
        }
    }
}

static BOOL SCValdiObjectsEqual(id left, id right) {
    if (!left) {
        return right == nil;
    }
    if (!right) {
        return left == nil;
    }

    return [left isEqual:right];
}

extern BOOL SCValdiMarshallableObjectsEqual(__unsafe_unretained SCValdiMarshallableObject* left,
                                               __unsafe_unretained SCValdiMarshallableObject* right,
                                               NSUInteger fieldsCount,
                                               const SCValdiFieldValueDescriptor* fieldsDescriptors) {
    SCValdiFieldValue *leftStorage = SCValdiGetMarshallableObjectFieldsStorage(left);
    SCValdiFieldValue *rightStorage = SCValdiGetMarshallableObjectFieldsStorage(right);

    for (size_t i = 0; i < fieldsCount; i++) {
        SCValdiFieldValue leftValue = leftStorage[i];
        SCValdiFieldValue rightValue = rightStorage[i];

        switch (fieldsDescriptors[i].type) {
            case SCValdiFieldValueTypeVoid:
                break;
            case SCValdiFieldValueTypePtr:
                if (SCValdiFieldValueGetPtr(leftValue) != SCValdiFieldValueGetPtr(rightValue)) {
                    return NO;
                }
                break;
            case SCValdiFieldValueTypeObject:
                if (!SCValdiObjectsEqual(SCValdiFieldValueGetObject(leftValue), SCValdiFieldValueGetObject(rightValue))) {
                    return NO;
                }
                break;
            case SCValdiFieldValueTypeDouble:
                if (SCValdiFieldValueGetDouble(leftValue) != SCValdiFieldValueGetDouble(rightValue)) {
                    return NO;
                }
                break;
            case SCValdiFieldValueTypeBool:
                if (SCValdiFieldValueGetBool(leftValue) != SCValdiFieldValueGetBool(rightValue)) {
                    return NO;
                }
                break;
            case SCValdiFieldValueTypeInt:
                if (SCValdiFieldValueGetInt(leftValue) != SCValdiFieldValueGetInt(rightValue)) {
                    return NO;
                }
                break;
            case SCValdiFieldValueTypeLong:
                if (SCValdiFieldValueGetDouble(leftValue) != SCValdiFieldValueGetDouble(rightValue)) {
                    return NO;
                }
                break;
        }
    }

    return YES;
}
