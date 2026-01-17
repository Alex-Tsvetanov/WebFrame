#include "valdi_core/jni/ValueFunctionWithJavaFunction.hpp"
#include "valdi_core/cpp/Utils/Marshaller.hpp"
#include "valdi_core/cpp/Utils/Trace.hpp"
#include "valdi_core/jni/JavaCache.hpp"

namespace ValdiAndroid {

ValueFunctionWithJavaFunction::ValueFunctionWithJavaFunction(JavaEnv env, jobject object)
    : GlobalRefJavaObjectBase(env, object, "ValdiFunction") {}

ValueFunctionWithJavaFunction::~ValueFunctionWithJavaFunction() = default;

Valdi::Value ValueFunctionWithJavaFunction::operator()(const Valdi::ValueFunctionCallContext& callContext) noexcept {
    Valdi::Marshaller marshaller(callContext.getExceptionTracker());
    for (size_t i = 0; i < callContext.getParametersSize(); i++) {
        marshaller.push(callContext.getParameters()[i]);
    }

    ValdiFunctionType functionObject(JavaEnv(), get());

    VALDI_TRACE("Valdi.callJavaFunction");
    JavaEnv::getCache().getValdiFunctionNativePerformFromNativeMethod().call(
        JavaEnv::getCache().getValdiFunctionNativeClass().getClass(),
        functionObject,
        reinterpret_cast<int64_t>(&marshaller));

    if (!callContext.getExceptionTracker()) {
        return Valdi::Value::undefined();
    } else {
        return marshaller.getOrUndefined(-1);
    }
}

std::string_view ValueFunctionWithJavaFunction::getFunctionType() const {
    return "Java";
}

} // namespace ValdiAndroid
