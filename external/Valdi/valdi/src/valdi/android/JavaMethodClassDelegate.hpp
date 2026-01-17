//
//  JavaMethodClassDelegate.hpp
//  valdi-android
//
//  Created by Simon Corsin on 2/17/23.
//

#pragma once

#include "valdi/android/JavaClassDelegate.hpp"
#include "valdi_core/jni/IndirectJavaGlobalRef.hpp"
#include "valdi_core/jni/JavaClass.hpp"
#include "valdi_core/jni/JavaMethod.hpp"

namespace ValdiAndroid {

/**
 Implementation of PlatformFunctionClassDelegate for Java Methods.
 */
class JavaMethodClassDelegate : public Valdi::PlatformFunctionClassDelegate<JavaValue> {
public:
    JavaMethodClassDelegate(const Valdi::Ref<Valdi::PlatformFunctionTrampoline<JavaValue>>& trampoline);
    ~JavaMethodClassDelegate() override;

    JavaValue newFunction(const Valdi::Ref<Valdi::ValueFunction>& valueFunction,
                          const Valdi::ReferenceInfoBuilder& referenceInfoBuilder,
                          Valdi::ExceptionTracker& exceptionTracker) final;

    Valdi::Ref<Valdi::ValueFunction> toValueFunction(const JavaValue* receiver,
                                                     const JavaValue& function,
                                                     const Valdi::ReferenceInfoBuilder& referenceInfoBuilder,
                                                     Valdi::ExceptionTracker& exceptionTracker) final;

private:
    Valdi::Ref<Valdi::PlatformFunctionTrampoline<JavaValue>> _trampoline;
};
} // namespace ValdiAndroid
