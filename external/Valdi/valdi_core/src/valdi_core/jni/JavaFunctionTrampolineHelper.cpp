//
//  JavaFunctionTrampolineHelper.cpp
//  valdi-android
//
//  Created by Simon Corsin on 2/16/23.
//

#include "valdi_core/jni/JavaFunctionTrampolineHelper.hpp"

#include "valdi_core/cpp/Schema/ValueSchema.hpp"
#include "valdi_core/cpp/Utils/InlineContainerAllocator.hpp"
#include "valdi_core/cpp/Utils/PlatformValueDelegate.hpp"
#include "valdi_core/cpp/Utils/ReferenceInfo.hpp"
#include "valdi_core/cpp/Utils/ValueFunction.hpp"

#include "valdi_core/jni/JNIMethodUtils.hpp"
#include "valdi_core/jni/JavaUtils.hpp"
#include <fmt/format.h>

namespace ValdiAndroid {

JavaFunctionTrampolineHelper::JavaFunctionTrampolineHelper(JavaClass&& cls, size_t entriesCount)
    : _cls(std::move(cls)), _entriesCount(entriesCount) {
    Valdi::InlineContainerAllocator<JavaFunctionTrampolineHelper, JavaFunctionTrampolineHelperEntry> allocator;
    allocator.constructContainerEntries(this, entriesCount);
}

JavaFunctionTrampolineHelper::~JavaFunctionTrampolineHelper() {
    Valdi::InlineContainerAllocator<JavaFunctionTrampolineHelper, JavaFunctionTrampolineHelperEntry> allocator;
    allocator.deallocate(this, _entriesCount);
}

JavaFunctionTrampolineHelperEntry& JavaFunctionTrampolineHelper::getEntry(size_t parametersSize) {
    if (parametersSize >= _entriesCount) {
        handleFatalError(fmt::format(
            "Java calls up to {} parameters are supported (requested {})", _entriesCount - 1, parametersSize));
    }

    Valdi::InlineContainerAllocator<JavaFunctionTrampolineHelper, JavaFunctionTrampolineHelperEntry> allocator;

    return allocator.getContainerStartPtr(this)[parametersSize];
}

JavaValue JavaFunctionTrampolineHelper::createJavaFunction(
    const Valdi::Ref<Valdi::PlatformFunctionTrampoline<JavaValue>>& trampoline,
    const Valdi::Ref<Valdi::ValueFunction>& valueFunction) {
    auto& entry = getEntry(trampoline->getParametersSize());

    auto object = entry.bridgeFunctionClass.newObject(entry.bridgeFunctionConstructor,
                                                      static_cast<int64_t>(bridgeRetain(trampoline.get())),
                                                      static_cast<int64_t>(bridgeRetain(valueFunction.get())));

    return JavaValue::makeObject(std::move(object));
}

JavaValue JavaFunctionTrampolineHelper::callJavaFunction(jobject javaFunction,
                                                         const JavaValue* parameters,
                                                         size_t parametersSize) {
    return getEntry(parametersSize).invoker.call(javaFunction, parametersSize, parameters);
}

JavaValue JavaFunctionTrampolineHelper::callNativeFunction(jlong trampolineHandle,
                                                           jlong valueFunctionHandle,
                                                           const JavaValue* parameters,
                                                           size_t parametersSize) {
    auto trampoline = bridge<Valdi::PlatformFunctionTrampoline<JavaValue>>(trampolineHandle);
    auto valueFunction = bridge<Valdi::ValueFunction>(valueFunctionHandle);

    if (trampoline == nullptr || valueFunction == nullptr) {
        throwJavaValdiException(JavaEnv::getUnsafeEnv(), Valdi::Error("Function was disposed"));
        return JavaValue::unsafeMakeObject(nullptr);
    }

    Valdi::SimpleExceptionTracker exceptionTracker;
    auto result = trampoline->forwardCall(
        valueFunction.get(), parameters, parametersSize, Valdi::ReferenceInfoBuilder(), nullptr, exceptionTracker);
    if (!exceptionTracker) {
        throwJavaValdiException(JavaEnv::getUnsafeEnv(), exceptionTracker.extractError());
        return JavaValue::unsafeMakeObject(nullptr);
    }

    return std::move(result.value());
}

Valdi::StringBox JavaFunctionTrampolineHelper::getFunctionClassNameForArity(size_t parametersSize) {
    return getEntry(parametersSize).functionClassName;
}

JavaFunctionTrampolineHelper& JavaFunctionTrampolineHelper::get() {
    static auto* kInstance = make();
    return *kInstance;
}

JavaFunctionTrampolineHelper* JavaFunctionTrampolineHelper::make() {
    auto cls = JavaClass::resolveOrAbort(JavaEnv(), "com/snap/valdi/callable/ValdiFunctionTrampoline");

    auto getFunctionClassesMethod = cls.getStaticMethod(
        "getFunctionClasses", Valdi::ValueSchema::array(Valdi::ValueSchema::untyped()), nullptr, 0, false);

    auto functionClasses = getFunctionClassesMethod.call(reinterpret_cast<jobject>(cls.getClass()), 0, nullptr);

    JavaObjectArray array(functionClasses.getObjectArray());

    auto size = array.size();
    SC_ASSERT(size % 5 == 0);

    Valdi::InlineContainerAllocator<JavaFunctionTrampolineHelper, JavaFunctionTrampolineHelperEntry> allocator;

    auto entriesCount = size / 5;
    auto* trampolineHelper = allocator.allocateUnmanaged(entriesCount, std::move(cls), entriesCount);

    size_t entryIndex = 0;

    for (size_t i = 0; i < size;) {
        auto functionClassName = array.getObject(i++);
        auto functionClass = array.getObject(i++);
        auto bridgeFunctionClass = array.getObject(i++);
        auto invokeMethod = array.getObject(i++);
        auto constructorMethod = array.getObject(i++);

        auto invoker = AnyJavaMethod::fromReflectedMethod(invokeMethod.get(), false, JavaValueType::Object);
        auto constructor = AnyJavaMethod::fromReflectedMethod(constructorMethod.get(), false, JavaValueType::Object);

        auto& entry = allocator.getContainerStartPtr(trampolineHelper)[entryIndex];
        entry.functionClassName = toInternedString(JavaEnv(), reinterpret_cast<jstring>(functionClassName.get()));
        entry.functionClass = JavaClass(JavaEnv(), reinterpret_cast<jclass>(functionClass.get()));
        entry.bridgeFunctionClass = JavaClass(JavaEnv(), reinterpret_cast<jclass>(bridgeFunctionClass.get()));
        entry.invoker = invoker;
        entry.bridgeFunctionConstructor.setMethodId(constructor.getMethodId());

        entryIndex++;
    }

    return trampolineHelper;
}

} // namespace ValdiAndroid
