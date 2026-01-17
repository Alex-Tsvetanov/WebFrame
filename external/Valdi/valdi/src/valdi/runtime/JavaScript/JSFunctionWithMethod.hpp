//
//  JSFunctionWithMethod.hpp
//  valdi
//
//  Created by Simon Corsin on 5/11/21.
//

#pragma once

#include "valdi/runtime/Interfaces/IJavaScriptContext.hpp"
#include "valdi_core/cpp/Utils/ReferenceInfo.hpp"
#include "valdi_core/cpp/Utils/ValueFunction.hpp"

namespace Valdi {

/**
 A JSFunction backed by a method pointer to a C++ object instance.
 */
template<typename T>
class JSFunctionWithMethod : public JSFunction {
public:
    JSFunctionWithMethod(T& self,
                         JSValueRef (T::*function)(JSFunctionNativeCallContext&),
                         const ReferenceInfo& referenceInfo)
        : _self(self), _function(function), _referenceInfo(referenceInfo) {}
    ~JSFunctionWithMethod() override = default;

    const ReferenceInfo& getReferenceInfo() const override {
        return _referenceInfo;
    }

    JSValueRef operator()(JSFunctionNativeCallContext& callContext) noexcept override {
        return (_self.*_function)(callContext);
    }

private:
    T& _self;
    JSValueRef (T::*_function)(JSFunctionNativeCallContext&);
    ReferenceInfo _referenceInfo;
};

} // namespace Valdi
