//
//  ValueFunctionWithMethod.cpp
//  valdi_core-pc
//
//  Created by Simon Corsin on 5/16/24.
//

#include "valdi_core/cpp/Utils/ValueFunctionWithMethod.hpp"

namespace Valdi {

void bindValueFunctionToValue(Value& value, const char* methodName, const Ref<ValueFunction>& valueFunction) {
    value.setMapValue(methodName, Value(valueFunction));
}

} // namespace Valdi
