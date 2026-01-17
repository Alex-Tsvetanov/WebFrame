//
//  ValueFunctionWithCallable.cpp
//  valdi_core-pc
//
//  Created by Simon Corsin on 5/10/21.
//

#include "valdi_core/cpp/Utils/ValueFunctionWithCallable.hpp"

namespace Valdi {

ValueFunctionWithCallable::ValueFunctionWithCallable(ValueFunctionCallable&& callable)
    : _callable(std::move(callable)) {}
ValueFunctionWithCallable::~ValueFunctionWithCallable() = default;

Value ValueFunctionWithCallable::operator()(const ValueFunctionCallContext& callContext) noexcept {
    return _callable(callContext);
}

std::string_view ValueFunctionWithCallable::getFunctionType() const {
    return "lambda";
}

} // namespace Valdi
