#pragma once

// You can include this on any platform (android/ios/pc) and it'll replace the
// JNI types with dummy types on non-android.

#ifdef __ANDROID__
#include <jni.h> // for JNINativeMethod; it's a typedef of an anonymous struct so we can't forward declare it

struct _JavaVM;
struct _JNIEnv;
using JavaVM = _JavaVM;
using JNIEnv = _JNIEnv;
#else
struct JNINativeMethod {};

struct JavaVM_;
struct JNIEnv_;
class _jobject {};

using JavaVM = JavaVM_;
using JNIEnv = JNIEnv_;
using jobject = _jobject*;
#endif
