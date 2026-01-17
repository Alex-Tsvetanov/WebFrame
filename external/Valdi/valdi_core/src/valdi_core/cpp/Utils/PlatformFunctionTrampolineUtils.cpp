// Copyright © 2023 Snap, Inc. All rights reserved.

#include "valdi_core/cpp/Utils/PlatformFunctionTrampolineUtils.hpp"

namespace Valdi {

CapturedValueFunctionCallContext::CapturedValueFunctionCallContext(const ValueFunctionCallContext& callContext)
    : _capturedParameters(callContext.getParameters(), callContext.getParameters() + callContext.getParametersSize()) {
    if (callContext.getFunctionIdentifier() != nullptr) {
        _functionIdentifier = *callContext.getFunctionIdentifier();
    }
}

CapturedValueFunctionCallContext::~CapturedValueFunctionCallContext() = default;

ValueFunctionCallContext CapturedValueFunctionCallContext::toCallContext(ExceptionTracker& exceptionTracker) const {
    return ValueFunctionCallContext(
        ValueFunctionFlagsNone, _capturedParameters.data(), _capturedParameters.size(), exceptionTracker);
}

void handleBridgeCallPromiseResult(const ValueFunctionCallContext& callContext,
                                   const Ref<ResolvablePromise>& resolvablePromise,
                                   const Value& result) {
    if (callContext.getExceptionTracker()) {
        auto nestedPromise = result.getTypedRef<Promise>();
        if (nestedPromise != nullptr) {
            // Flatten one level of promise, since we already returned a promise to the caller
            resolvablePromise->fulfillWithPromiseResult(nestedPromise);
        } else {
            resolvablePromise->fulfill(result);
        }
    } else {
        resolvablePromise->fulfill(callContext.getExceptionTracker().extractError());
    }
}

} // namespace Valdi
