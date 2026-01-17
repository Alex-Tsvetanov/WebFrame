//
//  JSFunctionValueFunction.hpp
//  valdi
//
//  Created by Simon Corsin on 5/11/21.
//

#pragma once

#include "valdi/runtime/Context/ContextAttachedValdiObject.hpp"
#include "valdi/runtime/Interfaces/IJavaScriptContext.hpp"
#include "valdi_core/cpp/Utils/ReferenceInfo.hpp"
#include "valdi_core/cpp/Utils/ValueFunction.hpp"

namespace Valdi {

/**
 a JSFunction backed by a ValueFunction reference.
 JSFunctionWithValueFunction will handle conversions in and out to Valdi C++ Value objects.
 */
class JSFunctionWithValueFunction : public JSFunction, public ContextAttachedObject {
public:
    JSFunctionWithValueFunction(Ref<ValueFunction>&& function, bool isSingleCall, const ReferenceInfo& referenceInfo);
    ~JSFunctionWithValueFunction() override;

    const ReferenceInfo& getReferenceInfo() const final;

    JSValueRef operator()(JSFunctionNativeCallContext& callContext) noexcept override;

    Ref<ValueFunction> getFunction(IJavaScriptContext& jsContext,
                                   JSExceptionTracker& exceptionTracker,
                                   bool shouldClear);

protected:
    virtual JSValueRef doCall(const Ref<ValueFunction>& function,
                              const StringBox* precomputedFunctionIdentifier,
                              JSFunctionNativeCallContext& callContext) const = 0;

private:
    ReferenceInfo _referenceInfo;
    StringBox _functionName;
    bool _isSingleCall;

    const StringBox& getFunctionName();

    JSValueRef callWithOwnerContextAsCurrent(const Ref<ValueFunction>& function,
                                             JSFunctionNativeCallContext& callContext) const;
};

class JSFunctionWithUntypedValueFunction : public JSFunctionWithValueFunction {
public:
    JSFunctionWithUntypedValueFunction(Ref<ValueFunction>&& function, const ReferenceInfo& referenceInfo);

protected:
    JSValueRef doCall(const Ref<ValueFunction>& function,
                      const StringBox* precomputedFunctionIdentifier,
                      JSFunctionNativeCallContext& callContext) const final;
};

} // namespace Valdi
