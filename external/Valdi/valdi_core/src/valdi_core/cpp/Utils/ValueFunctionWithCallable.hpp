//
//  ValueFunctionWithCallable.hpp
//  valdi_core-pc
//
//  Created by Simon Corsin on 5/10/21.
//

#pragma once

#include "valdi_core/cpp/Utils/Function.hpp"
#include "valdi_core/cpp/Utils/ValueFunction.hpp"

namespace Valdi {

using ValueFunctionCallable = Function<Value(const ValueFunctionCallContext& callContext)>;

/**
 a ValueFunction backed by an anonymous callable, represented as a Valdi::Function.
 */
class ValueFunctionWithCallable : public ValueFunction {
public:
    explicit ValueFunctionWithCallable(ValueFunctionCallable&& callable);
    ~ValueFunctionWithCallable() override;

    Value operator()(const ValueFunctionCallContext& callContext) noexcept override;

    std::string_view getFunctionType() const override;

    template<typename T, typename Ret>
    static Ref<ValueFunction> makeFromMethod(T* object, Ret (T::*method)()) {
        auto ref = Valdi::strongSmallRef(object);

        return makeShared<ValueFunctionWithCallable>([=](const ValueFunctionCallContext& /*callContext*/) -> Value {
            auto result = ((*ref).*method)();

            return Value(result);
        });
    }

    template<typename T>
    static Ref<ValueFunction> makeFromMethod(T* object,
                                             Value (T::*method)(const ValueFunctionCallContext& callContext)) {
        auto ref = Valdi::strongSmallRef(object);

        return makeShared<ValueFunctionWithCallable>(
            [=](const ValueFunctionCallContext& callContext) -> Value { return ((*ref).*method)(callContext); });
    }

private:
    ValueFunctionCallable _callable;
};

} // namespace Valdi
