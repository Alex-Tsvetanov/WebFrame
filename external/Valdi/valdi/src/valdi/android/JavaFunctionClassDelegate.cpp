//
//  JavaFunctionClassDelegate.cpp
//  valdi-android
//
//  Created by Simon Corsin on 2/16/23.
//

#include "valdi/android/JavaFunctionClassDelegate.hpp"
#include "valdi_core/cpp/Utils/InlineContainerAllocator.hpp"
#include "valdi_core/cpp/Utils/PlatformFunctionTrampolineUtils.hpp"
#include "valdi_core/cpp/Utils/ReferenceInfo.hpp"
#include "valdi_core/cpp/Utils/SmallVector.hpp"
#include "valdi_core/jni/JavaException.hpp"
#include "valdi_core/jni/JavaFunctionTrampolineHelper.hpp"
#include "valdi_core/jni/JavaUtils.hpp"

#include "valdi_core/cpp/Utils/ConsoleLogger.hpp"

namespace ValdiAndroid {

class JavaFunctionJNIExceptionHandler final : public JNIExceptionHandler {
public:
    explicit JavaFunctionJNIExceptionHandler(const Valdi::ReferenceInfo& referenceInfo)
        : _previousExceptionHandler(JavaEnv::getExceptionHandlerForCurrentThread()), _referenceInfo(referenceInfo) {
        JavaEnv::setExceptionHandlerForCurrentThread(this);
    }

    ~JavaFunctionJNIExceptionHandler() final {
        JavaEnv::setExceptionHandlerForCurrentThread(_previousExceptionHandler);
    }

    void onException(JavaException exception) final {
        exception.handleFatal(
            JavaEnv::getUnsafeEnv(),
            fmt::format("Failed to call Java function with reference '{}'", _referenceInfo.toString()));
    }

private:
    JNIExceptionHandler* _previousExceptionHandler = nullptr;
    [[maybe_unused]] const Valdi::ReferenceInfo& _referenceInfo;
};

class JavaValueFunction : public Valdi::ValueFunction {
public:
    JavaValueFunction(const Valdi::Ref<Valdi::PlatformFunctionTrampoline<JavaValue>>& trampoline,
                      const JavaValue& function,
                      // NOLINTNEXTLINE(performance-unnecessary-value-param)
                      Valdi::ReferenceInfo referenceInfo)
        : _trampoline(trampoline),
          _function(IndirectJavaGlobalRef::make(function.getObject(), "JavaFunction")),
          _referenceInfo(std::move(referenceInfo)) {}

    ~JavaValueFunction() override = default;

    Valdi::Value operator()(const Valdi::ValueFunctionCallContext& callContext) noexcept final {
        return handleBridgeCall(_trampoline->getCallQueue(),
                                _trampoline->isPromiseReturnType(),
                                this,
                                callContext,
                                [](auto* self, const auto& callContext) { return self->doCall(callContext); });
    }

    djinni::LocalRef<jobject> getJavaFunction() const {
        return _function.get();
    }

    std::string_view getFunctionType() const final {
        return "Java Function";
    }

    const Valdi::Ref<Valdi::PlatformFunctionTrampoline<JavaValue>>& getTrampoline() const {
        return _trampoline;
    }

private:
    Valdi::Ref<Valdi::PlatformFunctionTrampoline<JavaValue>> _trampoline;
    IndirectJavaGlobalRef _function;
    Valdi::ReferenceInfo _referenceInfo;

    Valdi::Value doCall(const Valdi::ValueFunctionCallContext& callContext) {
        JavaFunctionJNIExceptionHandler jniExceptionHandler(_referenceInfo);
        auto javaFunction = getJavaFunction();
        auto parametersSize = _trampoline->getParametersSize();

        Valdi::SmallVector<JavaValue, 8> parameters;
        for (size_t i = 0; i < parametersSize; i++) {
            parameters.emplace_back(JavaValue::unsafeMakeObject(nullptr));
        }

        if (!_trampoline->unmarshallParameters(callContext.getParameters(),
                                               callContext.getParametersSize(),
                                               parameters.data(),
                                               parameters.size(),
                                               Valdi::ReferenceInfoBuilder(),
                                               callContext.getExceptionTracker())) {
            return Valdi::Value::undefined();
        }

        auto& trampolineHelper = JavaFunctionTrampolineHelper::get();

        auto result = trampolineHelper.callJavaFunction(javaFunction.get(), parameters.data(), parameters.size());

        return _trampoline->marshallReturnValue(
            result, Valdi::ReferenceInfoBuilder(), callContext.getExceptionTracker());
    }
};

JavaFunctionClassDelegate::JavaFunctionClassDelegate(
    const Valdi::Ref<Valdi::PlatformFunctionTrampoline<JavaValue>>& trampoline)
    : _trampoline(trampoline) {}

JavaFunctionClassDelegate::~JavaFunctionClassDelegate() = default;

JavaValue JavaFunctionClassDelegate::newFunction(const Valdi::Ref<Valdi::ValueFunction>& valueFunction,
                                                 const Valdi::ReferenceInfoBuilder& /*referenceInfoBuilder*/,
                                                 Valdi::ExceptionTracker& exceptionTracker) {
    return javaFunctionFromValueFunction(_trampoline, valueFunction);
}

Valdi::Ref<Valdi::ValueFunction> JavaFunctionClassDelegate::toValueFunction(
    const JavaValue* /*receiver*/,
    const JavaValue& function,
    const Valdi::ReferenceInfoBuilder& referenceInfoBuilder,
    Valdi::ExceptionTracker& exceptionTracker) {
    return Valdi::makeShared<JavaValueFunction>(_trampoline, function, referenceInfoBuilder.build());
}

JavaValue JavaFunctionClassDelegate::javaFunctionFromValueFunction(
    const Valdi::Ref<Valdi::PlatformFunctionTrampoline<JavaValue>>& trampoline,
    const Valdi::Ref<Valdi::ValueFunction>& valueFunction) {
    auto javaFunction = Valdi::castOrNull<JavaValueFunction>(valueFunction);
    if (javaFunction != nullptr && javaFunction->getTrampoline() == trampoline) {
        // Re-use underlying Java function if it has the same function trampoline
        return JavaValue::makeObject(javaFunction->getJavaFunction());
    }

    return JavaFunctionTrampolineHelper::get().createJavaFunction(trampoline, valueFunction);
}

} // namespace ValdiAndroid
