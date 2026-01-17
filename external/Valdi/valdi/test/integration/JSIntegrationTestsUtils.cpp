#include "JSIntegrationTestsUtils.hpp"

using namespace Valdi;

namespace ValdiTest {

Ref<ValueFunction> toFunction(const Value& value) {
    if (value.getFunctionRef() == nullptr) {
        throw Exception(STRING_FORMAT("Value {} is not a function", value.toString()));
    }

    return value.getFunctionRef();
}

Valdi::Result<Valdi::Value> callFunction(const Ref<ValueFunction>& function, const std::vector<Value>& params) {
    return function->call(ValueFunctionFlagsCallSync, params.data(), params.size());
}

Valdi::Result<Valdi::Value> callFunction(const Ref<ValueFunction>& function) {
    return callFunction(function, {});
}

} // namespace ValdiTest
