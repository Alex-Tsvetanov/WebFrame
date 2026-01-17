//
//  ValueFunctionWithMethod.hpp
//  valdi_core-pc
//
//  Created by Simon Corsin on 5/16/24.
//

#pragma once

#include "valdi_core/cpp/Utils/ValueFunction.hpp"

namespace Valdi {

/**
 A ValueFunction backed by a method pointer to a C++ object instance.
 */
template<typename T>
class ValueFunctionWithMethod : public ValueFunction {
public:
    using Method = Value (T::*)(const ValueFunctionCallContext& callContext);

    ValueFunctionWithMethod(T* self, Method method) : _self(self), _method(method) {}
    ~ValueFunctionWithMethod() override = default;

    Value operator()(const ValueFunctionCallContext& callContext) noexcept final {
        return (*_self.*_method)(callContext);
    }

    std::string_view getFunctionType() const final {
        return "C++ Method";
    }

private:
    Ref<T> _self;
    Method _method;
};

void bindValueFunctionToValue(Value& value, const char* methodName, const Ref<ValueFunction>& valueFunction);

template<typename T, typename V = ValueFunctionWithMethod<T>>
class ValueFunctionMethodBinder {
public:
    ValueFunctionMethodBinder(T* self, Value& output) : _self(self), _output(output) {}
    ~ValueFunctionMethodBinder() = default;

    void bind(const char* methodName, typename V::Method method) {
        auto fn = makeShared<V>(_self, method);
        bindValueFunctionToValue(_output, methodName, fn);
    }

private:
    T* _self;
    Value& _output;
};

} // namespace Valdi
