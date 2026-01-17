#pragma once

#import <Foundation/Foundation.h>
#import <djinni/objc/DJIError.h>

#if !defined(__cpp_exceptions)

#define try
#undef DJINNI_TRANSLATE_EXCEPTIONS
#define DJINNI_TRANSLATE_EXCEPTIONS()

#endif
