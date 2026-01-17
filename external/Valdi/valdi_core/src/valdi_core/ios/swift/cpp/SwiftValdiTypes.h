//
//  SwiftValdiTypes.h
//  valdi_core
//
//  Created by Edward Lee on 4/30/24.
//

#pragma once

#include <CoreFoundation/CoreFoundation.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef intptr_t SwiftInt;
typedef int32_t SwiftInt32;
typedef int64_t SwiftInt64;
typedef double SwiftDouble;
typedef bool SwiftBool;
typedef CFStringRef SwiftString;
typedef void* /*InternedStringImpl*/ SwiftStringBox;
typedef struct {
    const char* buf;
    size_t size;
} SwiftStringView;

#ifdef __cplusplus
}
#endif
