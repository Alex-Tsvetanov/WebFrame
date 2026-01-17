//
//  JavaMethodClassDelegate.cpp
//  valdi-android
//
//  Created by Simon Corsin on 2/17/23.
//

#include "valdi/android/JavaMethodClassDelegate.hpp"
#include "valdi/android/JavaFunctionClassDelegate.hpp"
#include "valdi_core/cpp/Utils/Error.hpp"
#include "valdi_core/cpp/Utils/PlatformFunctionTrampolineUtils.hpp"
#include "valdi_core/cpp/Utils/ReferenceInfo.hpp"
#include "valdi_core/cpp/Utils/SmallVector.hpp"
#include "valdi_core/jni/JavaFunctionTrampolineHelper.hpp"
#include "valdi_core/jni/JavaUtils.hpp"

namespace ValdiAndroid {

static IndirectJavaGlobalRef makeGlobalWeakRef(jobject value, const char* tag) {
    auto weakRef = newWeakReference(JavaEnv(), value);
    return IndirectJavaGlobalRef::make(weakRef.get(), tag);
}

class JavaMethodValueFunction : public Valdi::ValueFunction {
public:
    JavaMethodValueFunction(const Valdi::Ref<Valdi::PlatformFunctionTrampoline<JavaValue>>& trampoline,
                            const JavaValue& receiver,
                            const AnyJavaMethod& method)
        : _trampoline(trampoline),
          _receiver(makeGlobalWeakRef(receiver.getObject(), "JavaProxyReceiver")),
          _method(method) {}

    ~JavaMethodValueFunction() override = default;

    Valdi::Value operator()(const Valdi::ValueFunctionCallContext& callContext) noexcept final {
        return handleBridgeCall(_trampoline->getCallQueue(),
                                _trampoline->isPromiseReturnType(),
                                this,
                                callContext,
                                [](auto* self, const auto& callContext) { return self->doCall(callContext); });
    }

    std::string_view getFunctionType() const final {
        return "Java Method";
    }

private:
    Valdi::Ref<Valdi::PlatformFunctionTrampoline<JavaValue>> _trampoline;
    IndirectJavaGlobalRef _receiver;
    AnyJavaMethod _method;

    Valdi::Value doCall(const Valdi::ValueFunctionCallContext& callContext) {
        auto receiver = getWeakReferenceValue(JavaEnv(), _receiver.get().get());

        if (receiver == nullptr) {
            callContext.getExceptionTracker().onError(
                Valdi::Error("Cannot call proxy method: Java receiver got garbage collected"));
            return Valdi::Value::undefined();
        }

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

        auto result = _method.call(receiver.get(), parameters.size(), parameters.data());

        return _trampoline->marshallReturnValue(
            result, Valdi::ReferenceInfoBuilder(), callContext.getExceptionTracker());
    }
};

JavaMethodClassDelegate::JavaMethodClassDelegate(
    const Valdi::Ref<Valdi::PlatformFunctionTrampoline<JavaValue>>& trampoline)
    : _trampoline(trampoline) {}

JavaMethodClassDelegate::~JavaMethodClassDelegate() = default;

JavaValue JavaMethodClassDelegate::newFunction(const Valdi::Ref<Valdi::ValueFunction>& valueFunction,
                                               const Valdi::ReferenceInfoBuilder& /*referenceInfoBuilder*/,
                                               Valdi::ExceptionTracker& exceptionTracker) {
    return JavaFunctionClassDelegate::javaFunctionFromValueFunction(_trampoline, valueFunction);
}

Valdi::Ref<Valdi::ValueFunction> JavaMethodClassDelegate::toValueFunction(
    const JavaValue* receiver,
    const JavaValue& function,
    const Valdi::ReferenceInfoBuilder& /*referenceInfoBuilder*/,
    Valdi::ExceptionTracker& exceptionTracker) {
    SC_ASSERT(receiver != nullptr);
    return Valdi::makeShared<JavaMethodValueFunction>(
        _trampoline, *receiver, *reinterpret_cast<AnyJavaMethod*>(function.getPointer()));
}

} // namespace ValdiAndroid
