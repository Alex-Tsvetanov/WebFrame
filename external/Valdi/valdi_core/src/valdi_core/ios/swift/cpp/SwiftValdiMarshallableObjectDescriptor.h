//
//  SwiftValdiMarshallableObjectDescriptor.h
//  valdi_core
//
//  Created by Edward Lee on 4/30/24.
//

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    SwiftValdiMarshallableObjectRegistry_ObjectType_Class,
    SwiftValdiMarshallableObjectRegistry_ObjectType_Protocol,
    SwiftValdiMarshallableObjectRegistry_ObjectType_Function,
    SwiftValdiMarshallableObjectRegistry_ObjectType_Untyped,
} SwiftValdiMarshallableObjectRegistry_ObjectType;

typedef struct {
    const char* name;
    const char* type;
} SwiftValdiMarshallableObjectRegistry_FieldDescriptor;

typedef struct {
    const SwiftValdiMarshallableObjectRegistry_FieldDescriptor* fields;
    const char* const* identifiers;
    SwiftValdiMarshallableObjectRegistry_ObjectType objectType;
} SwiftValdiMarshallableObjectRegistry_ObjectDescriptor;

#ifdef __cplusplus
}
#endif
