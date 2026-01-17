//
//  IMainThreadDispatcher.hpp
//  ValdiRuntime
//
//  Created by Simon Corsin on 5/29/18.
//  Copyright © 2018 Snap Inc. All rights reserved.
//

#pragma once

#include "valdi_core/cpp/Utils/Function.hpp"
#include "valdi_core/cpp/Utils/Shared.hpp"
#include <chrono>

namespace Valdi {

class IMainThreadDispatcher : public SimpleRefCountable {
public:
    IMainThreadDispatcher() = default;
    virtual ~IMainThreadDispatcher() = default;

    // The function should be deleted after it has been executed.
    virtual void dispatch(DispatchFunction* function, bool sync) = 0;
};

} // namespace Valdi
