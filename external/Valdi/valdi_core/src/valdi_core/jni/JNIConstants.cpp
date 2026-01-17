//
//  JNIConstants.cpp
//  ValdiAndroidNative
//
//  Created by Simon Corsin on 6/12/18.
//  Copyright © 2018 Snap Inc. All rights reserved.
//

#include "valdi_core/jni/JNIConstants.hpp"

namespace ValdiAndroid {

VoidType::VoidType() = default;

const char* const kJniSigObject = JAVA_CLASS("java/lang/Object");
const char* const kJniSigClass = JAVA_CLASS("java/lang/Class");

const char* const kJniSigString = JAVA_CLASS("java/lang/String");
const char* const kJniSigView = JAVA_CLASS("android.view.View");
const char* const kJniSigObjectArray = JAVA_ARRAY(JAVA_CLASS("java/lang/Object"));
const char* const kJniSigByteArray = JAVA_ARRAY("B");
const char* const kJniSigVoid = "V";
const char* const kJniSigInt = "I";
const char* const kJniSigLong = "J";
const char* const kJniSigDouble = "D";
const char* const kJniSigFloat = "F";
const char* const kJniSigBool = "Z";
const char* const kJniSigConstructor = "<init>";
const char* const kJniSigConstructorRetType = "V";

} // namespace ValdiAndroid
