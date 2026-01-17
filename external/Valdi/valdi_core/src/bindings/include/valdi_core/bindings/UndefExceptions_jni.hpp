#pragma once

#include "djinni/jni/djinni_support.hpp"
// This header will override the try/catch statements through macros
#include "valdi_core/cpp/Utils/Function.hpp"

#define try
#undef JNI_TRANSLATE_EXCEPTIONS_RETURN
#define JNI_TRANSLATE_EXCEPTIONS_RETURN(...)
