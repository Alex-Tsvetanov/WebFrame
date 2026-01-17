//
//  ValueFunction.cpp
//  valdi-ios
//
//  Created by Simon Corsin on 2/4/19.
//

#include "valdi_core/cpp/Utils/ValueFunction.hpp"
#include "valdi_core/cpp/Utils/ExceptionTracker.hpp"
#include "valdi_core/cpp/Utils/Format.hpp"
#include "valdi_core/cpp/Utils/Promise.hpp"
#include "valdi_core/cpp/Utils/StringCache.hpp"
#include "valdi_core/cpp/Utils/Value.hpp"

namespace Valdi {

const Value& ValueFunctionCallContext::getParameter(size_t index) const {
    return index < _parametersSize ? _parameters[index] : Value::undefinedRef();
}

bool ValueFunctionCallContext::getParameterAsBool(size_t index) const {
    return getParameter(index).checkedTo<bool>(_exceptionTracker);
}

StringBox ValueFunctionCallContext::getParameterAsString(size_t index) const {
    return getParameter(index).checkedTo<StringBox>(_exceptionTracker);
}

int32_t ValueFunctionCallContext::getParameterAsInt(size_t index) const {
    return getParameter(index).checkedTo<int32_t>(_exceptionTracker);
}

int64_t ValueFunctionCallContext::getParameterAsLong(size_t index) const {
    return getParameter(index).checkedTo<int64_t>(_exceptionTracker);
}

double ValueFunctionCallContext::getParameterAsDouble(size_t index) const {
    return getParameter(index).checkedTo<double>(_exceptionTracker);
}

Ref<ValueArray> ValueFunctionCallContext::getParameterAsArray(size_t index) const {
    return getParameter(index).checkedTo<Ref<ValueArray>>(_exceptionTracker);
}

Ref<ValueMap> ValueFunctionCallContext::getParameterAsMap(size_t index) const {
    return getParameter(index).checkedTo<Ref<ValueMap>>(_exceptionTracker);
}

Ref<ValueFunction> ValueFunctionCallContext::getParameterAsFunction(size_t index) const {
    return getParameter(index).checkedTo<Ref<ValueFunction>>(_exceptionTracker);
}

Ref<ValueArray> ValueFunctionCallContext::getParametersAsArray() const {
    return ValueArray::make(_parameters, _parameters + _parametersSize);
}

Result<Value> ValueFunction::operator()() noexcept {
    return (*this)(nullptr, 0);
}

Result<Value> ValueFunction::operator()(const Value* parameters, size_t size) noexcept {
    return (*this)(ValueFunctionFlagsNone, parameters, size);
}

Result<Value> ValueFunction::operator()(ValueFunctionFlags flags, const Value* parameters, size_t size) noexcept {
    return this->call(flags, parameters, size);
}

Result<Value> ValueFunction::call(ValueFunctionFlags flags, const Value* parameters, size_t size) noexcept {
    SimpleExceptionTracker exceptionTracker;
    auto value = (*this)(ValueFunctionCallContext(flags, parameters, size, exceptionTracker));
    if (!exceptionTracker) {
        return exceptionTracker.extractError();
    }
    return value;
}

StringBox ValueFunction::getDebugDescription() const {
    return STRING_FORMAT("{}@{}", getFunctionType(), reinterpret_cast<const void*>(this));
}

bool ValueFunction::propagatesOwnerContextOnCall() const {
    return false;
}

bool ValueFunction::prefersSyncCalls() const {
    return false;
}

} // namespace Valdi
