//
//  JSFunctionWithCallable.hpp
//  valdi
//
//  Created by Simon Corsin on 5/11/21.
//

#pragma once

#include "valdi/runtime/Interfaces/IJavaScriptContext.hpp"
#include "valdi_core/cpp/Utils/Function.hpp"
#include "valdi_core/cpp/Utils/ReferenceInfo.hpp"

namespace Valdi {

using JSFunctionCallable = Function<JSValueRef(JSFunctionNativeCallContext&)>;

/**
 a JSFunction backed by an anonymous callable, represented as a Valdi::Function.
 */
class JSFunctionWithCallable : public JSFunction {
public:
    JSFunctionWithCallable(const ReferenceInfoBuilder& referenceInfoBuilder, JSFunctionCallable&& callable);
    ~JSFunctionWithCallable() override;

    const ReferenceInfo& getReferenceInfo() const override;
    JSValueRef operator()(JSFunctionNativeCallContext& callContext) noexcept override;

private:
    ReferenceInfo _referenceInfo;
    JSFunctionCallable _callable;
};

} // namespace Valdi
