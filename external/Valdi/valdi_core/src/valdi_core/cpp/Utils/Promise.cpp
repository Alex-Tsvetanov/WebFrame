//
//  Error.cpp
//  valdi-ios
//
//  Created by Simon Corsin on 8/30/19.
//

#include "valdi_core/cpp/Utils/Promise.hpp"

namespace Valdi {

class PromiseCallbackAsFunctionBridge : public PromiseCallback {
public:
    explicit PromiseCallbackAsFunctionBridge(PromiseCallbackAsFunction&& callback) : _callback(std::move(callback)) {}

    ~PromiseCallbackAsFunctionBridge() override = default;

    void onSuccess(const Value& value) final {
        _callback(value);
    }

    void onFailure(const Error& error) final {
        _callback(error);
    }

private:
    PromiseCallbackAsFunction _callback;
};

Promise::Promise() = default;
Promise::~Promise() = default;

void Promise::onComplete(PromiseCallbackAsFunction callback) {
    onComplete(makeShared<PromiseCallbackAsFunctionBridge>(std::move(callback)));
}

VALDI_CLASS_IMPL(Promise)

} // namespace Valdi
