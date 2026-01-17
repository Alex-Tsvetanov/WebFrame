//
//  JSFunctionWithCallable.cpp
//  valdi
//
//  Created by Simon Corsin on 5/11/21.
//

#include "valdi/runtime/JavaScript/JSFunctionWithCallable.hpp"

namespace Valdi {

JSFunctionWithCallable::JSFunctionWithCallable(const ReferenceInfoBuilder& referenceInfoBuilder,
                                               JSFunctionCallable&& callable)
    : _referenceInfo(referenceInfoBuilder.asFunction().build()), _callable(std::move(callable)) {}

JSFunctionWithCallable::~JSFunctionWithCallable() = default;

const ReferenceInfo& JSFunctionWithCallable::getReferenceInfo() const {
    return _referenceInfo;
}

JSValueRef JSFunctionWithCallable::operator()(JSFunctionNativeCallContext& callContext) noexcept {
    return _callable(callContext);
}

} // namespace Valdi
