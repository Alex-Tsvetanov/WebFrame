//
//  Promise.hpp
//  valdi-core
//
//  Created by Simon Corsin on 8/07/23.
//

#pragma once

#include "valdi_core/cpp/Utils/Mutex.hpp"
#include "valdi_core/cpp/Utils/Promise.hpp"
#include "valdi_core/cpp/Utils/Result.hpp"
#include "valdi_core/cpp/Utils/SmallVector.hpp"
#include "valdi_core/cpp/Utils/ValueFunction.hpp"

namespace Valdi {

class ResolvablePromise final : public Promise {
public:
    ResolvablePromise();
    ~ResolvablePromise() override;

    void onComplete(const Ref<PromiseCallback>& callback) final;

    void cancel() final;

    bool isCancelable() const final;

    void setCancelCallback(DispatchFunction cancelCallback);

    /**
     Fulfill the promise with the given result.
     */
    void fulfill(Result<Value> result);

    /**
     Fulfill the promise with the result of the given promise
     */
    void fulfillWithPromiseResult(const Ref<Promise>& promise);

private:
    mutable Mutex _mutex;
    Result<Value> _result;
    SmallVector<Ref<PromiseCallback>, 1> _callbacks;
    DispatchFunction _cancelCallback;
    bool _canceled = false;
};

} // namespace Valdi
