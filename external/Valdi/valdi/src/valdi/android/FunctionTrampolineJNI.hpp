//
//  FunctionTrampolineJNI.hpp
//  valdi-android
//
//  Created by Simon Corsin on 2/16/23.
//

#pragma once

#include "valdi_core/cpp/Utils/PlatformValueDelegate.hpp"
#include "valdi_core/cpp/Utils/ValueFunction.hpp"
#include "valdi_core/jni/JNIMethodUtils.hpp"
#include "valdi_core/jni/JavaFunctionTrampolineHelper.hpp"
#include "valdi_core/jni/JavaUtils.hpp"
#include <fbjni/fbjni.h>

namespace fbjni = facebook::jni;

namespace ValdiAndroid {

class FunctionTrampolineJNI : public fbjni::JavaClass<FunctionTrampolineJNI> {
public:
    static constexpr auto kJavaDescriptor = "Lcom/snap/valdi/callable/ValdiFunctionTrampoline;";

    template<typename... T>
    static jobject nativePerformImpl(jlong trampolineHandle, jlong valueFunctionHandle, const T&... args) {
        const size_t ksize = sizeof...(args);
        JavaValue parameters[ksize] = {JavaValue::unsafeMakeObject(args)...};

        return JavaFunctionTrampolineHelper::callNativeFunction(
                   trampolineHandle, valueFunctionHandle, &parameters[0], ksize)
            .releaseObject();
    }

    // NOLINTNEXTLINE
    static jobject nativePerform0(fbjni::alias_ref<fbjni::JClass> /* clazz */,
                                  jlong trampolineHandle,
                                  jlong valueFunctionHandle) {
        return nativePerformImpl(trampolineHandle, valueFunctionHandle);
    }

    // NOLINTNEXTLINE
    static jobject nativePerform1(fbjni::alias_ref<fbjni::JClass> /* clazz */,
                                  jlong trampolineHandle,
                                  jlong valueFunctionHandle,
                                  jobject p0) {
        return nativePerformImpl(trampolineHandle, valueFunctionHandle, p0);
    }

    // NOLINTNEXTLINE
    static jobject nativePerform2(fbjni::alias_ref<fbjni::JClass> /* clazz */,
                                  jlong trampolineHandle,
                                  jlong valueFunctionHandle,
                                  jobject p0,
                                  jobject p1) {
        return nativePerformImpl(trampolineHandle, valueFunctionHandle, p0, p1);
    }

    // NOLINTNEXTLINE
    static jobject nativePerform3(fbjni::alias_ref<fbjni::JClass> /* clazz */,
                                  jlong trampolineHandle,
                                  jlong valueFunctionHandle,
                                  jobject p0,
                                  jobject p1,
                                  jobject p2) {
        return nativePerformImpl(trampolineHandle, valueFunctionHandle, p0, p1, p2);
    }

    // NOLINTNEXTLINE
    static jobject nativePerform4(fbjni::alias_ref<fbjni::JClass> /* clazz */,
                                  jlong trampolineHandle,
                                  jlong valueFunctionHandle,
                                  jobject p0,
                                  jobject p1,
                                  jobject p2,
                                  jobject p3) {
        return nativePerformImpl(trampolineHandle, valueFunctionHandle, p0, p1, p2, p3);
    }

    // NOLINTNEXTLINE
    static jobject nativePerform5(fbjni::alias_ref<fbjni::JClass> /* clazz */,
                                  jlong trampolineHandle,
                                  jlong valueFunctionHandle,
                                  jobject p0,
                                  jobject p1,
                                  jobject p2,
                                  jobject p3,
                                  jobject p4) {
        return nativePerformImpl(trampolineHandle, valueFunctionHandle, p0, p1, p2, p3, p4);
    }

    // NOLINTNEXTLINE
    static jobject nativePerform6(fbjni::alias_ref<fbjni::JClass> /* clazz */,
                                  jlong trampolineHandle,
                                  jlong valueFunctionHandle,
                                  jobject p0,
                                  jobject p1,
                                  jobject p2,
                                  jobject p3,
                                  jobject p4,
                                  jobject p5) {
        return nativePerformImpl(trampolineHandle, valueFunctionHandle, p0, p1, p2, p3, p4, p5);
    }

    // NOLINTNEXTLINE
    static jobject nativePerform7(fbjni::alias_ref<fbjni::JClass> /* clazz */,
                                  jlong trampolineHandle,
                                  jlong valueFunctionHandle,
                                  jobject p0,
                                  jobject p1,
                                  jobject p2,
                                  jobject p3,
                                  jobject p4,
                                  jobject p5,
                                  jobject p6) {
        return nativePerformImpl(trampolineHandle, valueFunctionHandle, p0, p1, p2, p3, p4, p5, p6);
    }

    // NOLINTNEXTLINE
    static jobject nativePerform8(fbjni::alias_ref<fbjni::JClass> /* clazz */,
                                  jlong trampolineHandle,
                                  jlong valueFunctionHandle,
                                  jobject p0,
                                  jobject p1,
                                  jobject p2,
                                  jobject p3,
                                  jobject p4,
                                  jobject p5,
                                  jobject p6,
                                  jobject p7) {
        return nativePerformImpl(trampolineHandle, valueFunctionHandle, p0, p1, p2, p3, p4, p5, p6, p7);
    }

    // NOLINTNEXTLINE
    static jobject nativePerform9(fbjni::alias_ref<fbjni::JClass> /* clazz */,
                                  jlong trampolineHandle,
                                  jlong valueFunctionHandle,
                                  jobject p0,
                                  jobject p1,
                                  jobject p2,
                                  jobject p3,
                                  jobject p4,
                                  jobject p5,
                                  jobject p6,
                                  jobject p7,
                                  jobject p8) {
        return nativePerformImpl(trampolineHandle, valueFunctionHandle, p0, p1, p2, p3, p4, p5, p6, p7, p8);
    }

    // NOLINTNEXTLINE
    static jobject nativePerform10(fbjni::alias_ref<fbjni::JClass> /* clazz */,
                                   jlong trampolineHandle,
                                   jlong valueFunctionHandle,
                                   jobject p0,
                                   jobject p1,
                                   jobject p2,
                                   jobject p3,
                                   jobject p4,
                                   jobject p5,
                                   jobject p6,
                                   jobject p7,
                                   jobject p8,
                                   jobject p9) {
        return nativePerformImpl(trampolineHandle, valueFunctionHandle, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9);
    }

    // NOLINTNEXTLINE
    static jobject nativePerform11(fbjni::alias_ref<fbjni::JClass> /* clazz */,
                                   jlong trampolineHandle,
                                   jlong valueFunctionHandle,
                                   jobject p0,
                                   jobject p1,
                                   jobject p2,
                                   jobject p3,
                                   jobject p4,
                                   jobject p5,
                                   jobject p6,
                                   jobject p7,
                                   jobject p8,
                                   jobject p9,
                                   jobject p10) {
        return nativePerformImpl(trampolineHandle, valueFunctionHandle, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10);
    }

    // NOLINTNEXTLINE
    static jobject nativePerform12(fbjni::alias_ref<fbjni::JClass> /* clazz */,
                                   jlong trampolineHandle,
                                   jlong valueFunctionHandle,
                                   jobject p0,
                                   jobject p1,
                                   jobject p2,
                                   jobject p3,
                                   jobject p4,
                                   jobject p5,
                                   jobject p6,
                                   jobject p7,
                                   jobject p8,
                                   jobject p9,
                                   jobject p10,
                                   jobject p11) {
        return nativePerformImpl(
            trampolineHandle, valueFunctionHandle, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11);
    }

    // NOLINTNEXTLINE
    static jobject nativePerform13(fbjni::alias_ref<fbjni::JClass> /* clazz */,
                                   jlong trampolineHandle,
                                   jlong valueFunctionHandle,
                                   jobject p0,
                                   jobject p1,
                                   jobject p2,
                                   jobject p3,
                                   jobject p4,
                                   jobject p5,
                                   jobject p6,
                                   jobject p7,
                                   jobject p8,
                                   jobject p9,
                                   jobject p10,
                                   jobject p11,
                                   jobject p12) {
        return nativePerformImpl(
            trampolineHandle, valueFunctionHandle, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12);
    }

    // NOLINTNEXTLINE
    static jobject nativePerform14(fbjni::alias_ref<fbjni::JClass> /* clazz */,
                                   jlong trampolineHandle,
                                   jlong valueFunctionHandle,
                                   jobject p0,
                                   jobject p1,
                                   jobject p2,
                                   jobject p3,
                                   jobject p4,
                                   jobject p5,
                                   jobject p6,
                                   jobject p7,
                                   jobject p8,
                                   jobject p9,
                                   jobject p10,
                                   jobject p11,
                                   jobject p12,
                                   jobject p13) {
        return nativePerformImpl(
            trampolineHandle, valueFunctionHandle, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13);
    }

    // NOLINTNEXTLINE
    static jobject nativePerform15(fbjni::alias_ref<fbjni::JClass> /* clazz */,
                                   jlong trampolineHandle,
                                   jlong valueFunctionHandle,
                                   jobject p0,
                                   jobject p1,
                                   jobject p2,
                                   jobject p3,
                                   jobject p4,
                                   jobject p5,
                                   jobject p6,
                                   jobject p7,
                                   jobject p8,
                                   jobject p9,
                                   jobject p10,
                                   jobject p11,
                                   jobject p12,
                                   jobject p13,
                                   jobject p14) {
        return nativePerformImpl(
            trampolineHandle, valueFunctionHandle, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14);
    }

    // NOLINTNEXTLINE
    static jobject nativePerform16(fbjni::alias_ref<fbjni::JClass> /* clazz */,
                                   jlong trampolineHandle,
                                   jlong valueFunctionHandle,
                                   jobject p0,
                                   jobject p1,
                                   jobject p2,
                                   jobject p3,
                                   jobject p4,
                                   jobject p5,
                                   jobject p6,
                                   jobject p7,
                                   jobject p8,
                                   jobject p9,
                                   jobject p10,
                                   jobject p11,
                                   jobject p12,
                                   jobject p13,
                                   jobject p14,
                                   jobject p15) {
        return nativePerformImpl(trampolineHandle,
                                 valueFunctionHandle,
                                 p0,
                                 p1,
                                 p2,
                                 p3,
                                 p4,
                                 p5,
                                 p6,
                                 p7,
                                 p8,
                                 p9,
                                 p10,
                                 p11,
                                 p12,
                                 p13,
                                 p14,
                                 p15);
    }

    static void registerNatives() {
        javaClassStatic()->registerNatives({
            makeNativeMethod("nativePerform0", FunctionTrampolineJNI::nativePerform0),
            makeNativeMethod("nativePerform1", FunctionTrampolineJNI::nativePerform1),
            makeNativeMethod("nativePerform2", FunctionTrampolineJNI::nativePerform2),
            makeNativeMethod("nativePerform3", FunctionTrampolineJNI::nativePerform3),
            makeNativeMethod("nativePerform4", FunctionTrampolineJNI::nativePerform4),
            makeNativeMethod("nativePerform5", FunctionTrampolineJNI::nativePerform5),
            makeNativeMethod("nativePerform6", FunctionTrampolineJNI::nativePerform6),
            makeNativeMethod("nativePerform7", FunctionTrampolineJNI::nativePerform7),
            makeNativeMethod("nativePerform8", FunctionTrampolineJNI::nativePerform8),
            makeNativeMethod("nativePerform9", FunctionTrampolineJNI::nativePerform9),
            makeNativeMethod("nativePerform10", FunctionTrampolineJNI::nativePerform10),
            makeNativeMethod("nativePerform11", FunctionTrampolineJNI::nativePerform11),
            makeNativeMethod("nativePerform12", FunctionTrampolineJNI::nativePerform12),
            makeNativeMethod("nativePerform13", FunctionTrampolineJNI::nativePerform13),
            makeNativeMethod("nativePerform14", FunctionTrampolineJNI::nativePerform14),
            makeNativeMethod("nativePerform15", FunctionTrampolineJNI::nativePerform15),
            makeNativeMethod("nativePerform16", FunctionTrampolineJNI::nativePerform16),
        });
    }
};

} // namespace ValdiAndroid
