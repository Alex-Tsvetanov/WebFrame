//
//  JNIMethodUtils.hpp
//  valdi-android
//
//  Created by Simon Corsin on 7/24/19.
//

#pragma once

#include "valdi_core/cpp/Utils/Error.hpp"
#include <fbjni/fbjni.h>
#include <jni.h>

namespace ValdiAndroid {

void throwJavaValdiException(JNIEnv* env, const char* message);
void throwJavaValdiException(JNIEnv* env, const Valdi::Error& error);

} // namespace ValdiAndroid
