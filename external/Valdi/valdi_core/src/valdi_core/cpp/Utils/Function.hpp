//
//  Function.hpp
//  valdi-pc
//
//  Created by Simon Corsin on 2/24/20.
//

#pragma once

#include "utils/base/Function.hpp"

namespace Valdi {

template<typename Signature>
using Function = snap::CopyableFunction<Signature>;

struct DispatchFunction : public Valdi::Function<void()> {
    using Valdi::Function<void()>::Function;
};

} // namespace Valdi
