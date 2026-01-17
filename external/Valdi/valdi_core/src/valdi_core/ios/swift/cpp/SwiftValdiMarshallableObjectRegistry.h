//
//  SwiftValdiMarshallableObjectRegistry.h
//  valdi_core
//
//  Created by Edward Lee on 4/30/24.
//
#pragma once

#include <stdint.h>
#include <valdi_core/ios/swift/cpp/SwiftValdiMarshallableObjectDescriptor.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void* SwiftValdiMarshallerRef;
typedef void* SwiftValdiMarshallableObjectRegistryRef;

SwiftValdiMarshallableObjectRegistryRef SwiftValdiMarshallableObjectRegistry_Create(void* valueSchemaRegistryPtr);

void SwiftValdiMarshallableObjectRegistry_Destroy(SwiftValdiMarshallableObjectRegistryRef marshallerRegistry);

void SwiftValdiMarshallableObjectRegistry_RegisterClass(
    SwiftValdiMarshallableObjectRegistryRef marshallerRegistry,
    const char* className,
    SwiftValdiMarshallableObjectRegistry_ObjectDescriptor descriptor);

void SwiftValdiMarshallableObjectRegistry_RegisterStringEnum(SwiftValdiMarshallableObjectRegistryRef marshallerRegistry,
                                                             const char* enumName,
                                                             const char* const* enumCases,
                                                             const int32_t numCases);

void SwiftValdiMarshallableObjectRegistry_RegisterIntEnum(SwiftValdiMarshallableObjectRegistryRef marshallerRegistry,
                                                          const char* enumName,
                                                          const int32_t* enumCases,
                                                          const int32_t numCases);

void SwiftValdiMarshallableObjectRegistry_SetActiveSchemeOfClassInMarshaller(
    SwiftValdiMarshallableObjectRegistryRef marshallerRegistry,
    const char* className,
    SwiftValdiMarshallerRef marshaller);

#ifdef __cplusplus
}
#endif
