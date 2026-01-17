#pragma once
#ifdef __cplusplus
extern "C" {
#endif
#include "SwiftValdiMarshaller.h"
#include <stdint.h>

// This file contains the interface for calls from C into Swift

// Calls the Swift ValdiFunction object with the associated SwiftValdiMarshaller
// Returns true if the function succeded
bool swiftCallWithMarshaller(SwiftValdiFunction swiftFunction, SwiftValdiMarshallerRef marshaller);

// Increments the reference count of the underlying Swift Object
void retainSwiftObject(const void* swiftObject);

// Decrements the reference count of the underlying Swift Object
void releaseSwiftObject(const void* swiftObject);

#ifdef __cplusplus
}
#endif