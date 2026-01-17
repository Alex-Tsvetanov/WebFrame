//
//  JavaFunctionClassDelegate.hpp
//  valdi-android
//
//  Created by Simon Corsin on 2/16/23.
//

#pragma once

#include "valdi/android/JavaClassDelegate.hpp"
#include "valdi_core/jni/IndirectJavaGlobalRef.hpp"
#include "valdi_core/jni/JavaClass.hpp"
#include "valdi_core/jni/JavaMethod.hpp"

namespace ValdiAndroid {

/**
 Implementation of PlatformFunctionClassDelegate for Java Functions. Will use JavaFunctionTrampolineHelper
 to create Java functions from ValueFunction objects.
 */
class JavaFunctionClassDelegate : public Valdi::PlatformFunctionClassDelegate<JavaValue> {
public:
    JavaFunctionClassDelegate(const Valdi::Ref<Valdi::PlatformFunctionTrampoline<JavaValue>>& trampoline);
    ~JavaFunctionClassDelegate() override;

    JavaValue newFunction(const Valdi::Ref<Valdi::ValueFunction>& valueFunction,
                          const Valdi::ReferenceInfoBuilder& referenceInfoBuilder,
                          Valdi::ExceptionTracker& exceptionTracker) final;

    Valdi::Ref<Valdi::ValueFunction> toValueFunction(const JavaValue* receiver,
                                                     const JavaValue& function,
                                                     const Valdi::ReferenceInfoBuilder& referenceInfoBuilder,
                                                     Valdi::ExceptionTracker& exceptionTracker) final;

    static JavaValue javaFunctionFromValueFunction(
        const Valdi::Ref<Valdi::PlatformFunctionTrampoline<JavaValue>>& trampoline,
        const Valdi::Ref<Valdi::ValueFunction>& valueFunction);

private:
    Valdi::Ref<Valdi::PlatformFunctionTrampoline<JavaValue>> _trampoline;
};
} // namespace ValdiAndroid
