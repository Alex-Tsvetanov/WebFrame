#pragma once

#include "valdi_core/cpp/Utils/ValueFunction.hpp"
#include "valdi_core/jni/GlobalRefJavaObject.hpp"

namespace ValdiAndroid {

class ValueFunctionWithJavaFunction : public Valdi::ValueFunction, public GlobalRefJavaObjectBase {
public:
    ValueFunctionWithJavaFunction(JavaEnv env, jobject object);
    ~ValueFunctionWithJavaFunction() override;

    Valdi::Value operator()(const Valdi::ValueFunctionCallContext& callContext) noexcept override;
    std::string_view getFunctionType() const override;
};

} // namespace ValdiAndroid
