//
//  JavaFunctionTrampolineHelper.hpp
//  valdi-android
//
//  Created by Simon Corsin on 2/16/23.
//

#pragma once

#include "valdi_core/jni/JavaClass.hpp"
#include "valdi_core/jni/JavaMethod.hpp"

namespace Valdi {
template<typename ValueType>
class PlatformFunctionTrampoline;

template<typename T, typename ValueType>
struct InlineContainerAllocator;

class ValueFunction;
} // namespace Valdi

namespace ValdiAndroid {

struct JavaFunctionTrampolineHelperEntry {
    Valdi::StringBox functionClassName;
    JavaClass functionClass;
    JavaClass bridgeFunctionClass;
    AnyJavaMethod invoker;
    JavaMethod<ConstructorType, int64_t, int64_t> bridgeFunctionConstructor;
};

/**
 Helper singleton to allow native code to create and call Java functions.
 Uses helpers from ValdiFunctionTrampoline.kt.
 */
class JavaFunctionTrampolineHelper : public Valdi::SimpleRefCountable {
public:
    ~JavaFunctionTrampolineHelper() override;

    /**
     Create a Java function that will retain the given trampoline and value function.
     When the function is called from Java, it will call callNativeFunction() and pass it
     the parameters that were passed from Java.
     */
    JavaValue createJavaFunction(const Valdi::Ref<Valdi::PlatformFunctionTrampoline<JavaValue>>& trampoline,
                                 const Valdi::Ref<Valdi::ValueFunction>& valueFunction);

    /**
     Call a Java function with the given parameters. The implementation will use the resolved Java Method
     for the given parametersSize which was returned by ValdiFunctionTrampoline.getFunctionClasses().
     */
    JavaValue callJavaFunction(jobject javaFunction, const JavaValue* parameters, size_t parametersSize);

    /**
     Return the Java Function class type name for the given parametersSize
     */
    Valdi::StringBox getFunctionClassNameForArity(size_t parametersSize);

    /**
     Call the native ValueFunction with the given trampoline and the given Java parameters.
     The parameters will be passed to the trampoline which will convert them, call the ValueFunction,
     convert the return value to Java, and return the value as a JavaValue.
     */
    static JavaValue callNativeFunction(jlong trampolineHandle,
                                        jlong valueFunctionHandle,
                                        const JavaValue* parameters,
                                        size_t parametersSize);

    static JavaFunctionTrampolineHelper& get();

private:
    JavaClass _cls;
    size_t _entriesCount;

    JavaFunctionTrampolineHelper(JavaClass&& cls, size_t entriesCount);

    friend Valdi::InlineContainerAllocator<JavaFunctionTrampolineHelper, JavaFunctionTrampolineHelperEntry>;

    static JavaFunctionTrampolineHelper* make();

    JavaFunctionTrampolineHelperEntry& getEntry(size_t parametersSize);
};

} // namespace ValdiAndroid
