// Copyright © 2023 Snap, Inc. All rights reserved.

#pragma once

#include "valdi_core/cpp/Threading/IDispatchQueue.hpp"
#include "valdi_core/cpp/Utils/ResolvablePromise.hpp"
#include "valdi_core/cpp/Utils/SmallVector.hpp"
#include "valdi_core/cpp/Utils/Trace.hpp"
#include "valdi_core/cpp/Utils/Value.hpp"
#include "valdi_core/cpp/Utils/ValueFunction.hpp"

namespace Valdi {

class CapturedValueFunctionCallContext {
public:
    CapturedValueFunctionCallContext(const ValueFunctionCallContext& callContext);
    ~CapturedValueFunctionCallContext();

    ValueFunctionCallContext toCallContext(ExceptionTracker& exceptionTracker) const;

    constexpr const StringBox& getFunctionIdentifier() const {
        return _functionIdentifier;
    }

private:
    SmallVector<Value, 2> _capturedParameters;
    StringBox _functionIdentifier;
};

void handleBridgeCallPromiseResult(const ValueFunctionCallContext& callContext,
                                   const Ref<ResolvablePromise>& resolvablePromise,
                                   const Value& result);

template<typename T, typename F>
Value handleBridgeCall(const Ref<IDispatchQueue>& callQueue,
                       bool isPromiseReturnType,
                       T* self,
                       const ValueFunctionCallContext& callContext,
                       F&& doCall) {
    if (callQueue == nullptr) {
        return doCall(self, callContext);
    } else {
        if (isPromiseReturnType) {
            auto promise = makeShared<ResolvablePromise>();

            callQueue->async([strongSelf = strongSmallRef(self),
                              promise,
                              capturedCallContext = CapturedValueFunctionCallContext(callContext),
                              doCall = std::move(doCall)]() {
                VALDI_TRACE_META("Valdi.workerCall", capturedCallContext.getFunctionIdentifier());
                SimpleExceptionTracker exceptionTracker;
                auto callContext = capturedCallContext.toCallContext(exceptionTracker);
                auto result = doCall(strongSelf.get(), callContext);

                handleBridgeCallPromiseResult(callContext, promise, result);
            });

            return Value(promise);
        } else {
            callQueue->async([strongSelf = strongSmallRef(self),
                              capturedCallContext = CapturedValueFunctionCallContext(callContext),
                              doCall = std::move(doCall)]() {
                VALDI_TRACE_META("Valdi.workerCall", capturedCallContext.getFunctionIdentifier());
                SimpleExceptionTracker exceptionTracker;
                auto callContext = capturedCallContext.toCallContext(exceptionTracker);
                doCall(strongSelf.get(), callContext);
            });

            return Value::undefined();
        }
    }
}

} // namespace Valdi
