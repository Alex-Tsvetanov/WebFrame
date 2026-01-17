//
//  SCValdiMarshallableObjectUtils.h
//  valdi-ios
//
//  Created by Simon Corsin on 6/5/20.
//

#import <Foundation/Foundation.h>

#import "valdi_core/SCMacros.h"
#import "valdi_core/SCValdiFieldValue.h"

SC_EXTERN_C_BEGIN

extern SEL SCValdiMakeSetterSelector(const char* getterMethodName);

extern char SCValdiCTypeCharForFieldType(SCValdiFieldValueType fieldType);
extern SCValdiFieldValueType SCValdiFieldTypeFromCTypeChar(char c);
const char* SCValdiObjCTypeForFieldType(SCValdiFieldValueType fieldType);

@class SCValdiMarshallableObject;
extern SCValdiFieldValue* SCValdiGetMarshallableObjectFieldsStorage(
    __unsafe_unretained SCValdiMarshallableObject* instance);
extern void SCValdiSetFieldsStorageObjectValue(SCValdiFieldValue* fieldsStorage,
                                               NSUInteger fieldIndex,
                                               BOOL isCopyable,
                                               __unsafe_unretained id value);
extern void SCValdiSetFieldsStorageValue(SCValdiFieldValue* fieldsStorage,
                                         NSUInteger fieldIndex,
                                         SCValdiFieldValue value);

extern IMP SCValdiResolveMethodImpl(__unsafe_unretained id instance, SEL selector);

extern void SCValdiRegisterObjectMethod(
    Class objectClass, IMP method, SEL selector, const char* returnType, const char* argument);
extern void SCValdiAppendFieldSetter(SCValdiFieldValueDescriptor fieldDescriptor,
                                     const char* selectorName,
                                     NSUInteger fieldIndex,
                                     Class objectClass);
extern void SCValdiAppendFieldGetter(SCValdiFieldValueDescriptor fieldDescriptor,
                                     NSUInteger fieldIndex,
                                     Class objectClass);

extern SCValdiFieldValue SCValdiGetFieldValueFromBridgedObject(__unsafe_unretained id objectInstance,
                                                               SCValdiFieldValueDescriptor fieldDescriptor);

extern SCValdiFieldValue* SCValdiAllocateFieldsStorage(NSUInteger fieldsCount);

extern void SCValdiDeallocateFieldsStorage(SCValdiFieldValue* fieldsStorage,
                                           NSUInteger fieldsCount,
                                           const SCValdiFieldValueDescriptor* fieldsDescriptors);

extern void SCValdiSetFieldsStorageWithValues(SCValdiFieldValue* fieldsStorage,
                                              NSUInteger fieldsCount,
                                              const SCValdiFieldValueDescriptor* fieldsDescriptors,
                                              const SCValdiFieldValue* values);

extern void SCValdiSetFieldsStorageWithValuesList(SCValdiFieldValue* fieldsStorage,
                                                  NSUInteger fieldsCount,
                                                  const SCValdiFieldValueDescriptor* fieldsDescriptors,
                                                  va_list list);

extern BOOL SCValdiMarshallableObjectsEqual(__unsafe_unretained SCValdiMarshallableObject* left,
                                            __unsafe_unretained SCValdiMarshallableObject* right,
                                            NSUInteger fieldsCount,
                                            const SCValdiFieldValueDescriptor* fieldsDescriptors);

SC_EXTERN_C_END
