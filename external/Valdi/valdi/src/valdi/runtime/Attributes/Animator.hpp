//
//  Animator.hpp
//  valdi-ios
//
//  Created by Simon Corsin on 4/30/19.
//

#pragma once

#include "valdi_core/Animator.hpp"
#include "valdi_core/cpp/Utils/Shared.hpp"
#include "valdi_core/cpp/Utils/Value.hpp"
#include "valdi_core/cpp/Utils/ValueFunctionWithCallable.hpp"
#include <vector>

namespace Valdi {

using NativeAnimator = std::shared_ptr<snap::valdi_core::Animator>;

class Animator : public SharedPtrRefCountable {
public:
    explicit Animator(NativeAnimator nativeAnimator);
    ~Animator() override;

    void appendCompletion(ValueFunctionCallable completion);
    void appendCompletion(Ref<ValueFunction> completion);
    Value makeCompletionFunction();

    const NativeAnimator& getNativeAnimator() const;

private:
    NativeAnimator _nativeAnimator;
    std::vector<Ref<ValueFunction>> _completions;
};

using SharedAnimator = Ref<Animator>;

} // namespace Valdi
