//
//  Animator.cpp
//  valdi-ios
//
//  Created by Simon Corsin on 4/30/19.
//

#include "valdi/runtime/Attributes/Animator.hpp"

#include "utils/debugging/Assert.hpp"

namespace Valdi {

Animator::Animator(std::shared_ptr<snap::valdi_core::Animator> nativeAnimator)
    : _nativeAnimator(std::move(nativeAnimator)) {}

Animator::~Animator() = default;

void Animator::appendCompletion(ValueFunctionCallable completion) {
    appendCompletion(makeShared<ValueFunctionWithCallable>(std::move(completion)));
}

void Animator::appendCompletion(Ref<ValueFunction> completion) {
    _completions.emplace_back(std::move(completion));
}

Value Animator::makeCompletionFunction() {
    return Value(makeShared<ValueFunctionWithCallable>(
        [completions = std::move(_completions)](const ValueFunctionCallContext& callContext) -> Value {
            for (const auto& completion : completions) {
                (*completion)(callContext);
            }
            return Valdi::Value::undefined();
        }));
}

const NativeAnimator& Animator::getNativeAnimator() const {
    return _nativeAnimator;
}

} // namespace Valdi
