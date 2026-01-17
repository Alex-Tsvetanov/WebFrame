//
//  Promise.hpp
//  valdi-core
//
//  Created by Simon Corsin on 8/07/23.
//

#pragma once

#include "valdi_core/cpp/Utils/Function.hpp"
#include "valdi_core/cpp/Utils/Result.hpp"
#include "valdi_core/cpp/Utils/ValdiObject.hpp"
#include "valdi_core/cpp/Utils/Value.hpp"

namespace Valdi {

using PromiseCallbackAsFunction = Function<void(const Result<Value>&)>;

class PromiseCallback : public SimpleRefCountable {
public:
    virtual void onSuccess(const Value& value) = 0;
    virtual void onFailure(const Error& error) = 0;
};

class Promise : public ValdiObject {
public:
    Promise();
    ~Promise() override;

    virtual void onComplete(const Ref<PromiseCallback>& callback) = 0;
    void onComplete(PromiseCallbackAsFunction callback);

    virtual void cancel() = 0;

    virtual bool isCancelable() const = 0;

    VALDI_CLASS_HEADER(Promise);
};

} // namespace Valdi
