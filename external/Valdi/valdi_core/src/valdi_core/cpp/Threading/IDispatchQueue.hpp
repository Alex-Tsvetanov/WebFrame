// Copyright © 2023 Snap, Inc. All rights reserved.

#pragma once

#include "valdi_core/cpp/Threading/TaskId.hpp"
#include "valdi_core/cpp/Utils/Function.hpp"
#include "valdi_core/cpp/Utils/Shared.hpp"
#include <chrono>

namespace Valdi {

class IDispatchQueue : public SharedPtrRefCountable {
public:
    virtual void sync(const DispatchFunction& function) = 0;
    virtual void async(DispatchFunction function) = 0;
    virtual task_id_t asyncAfter(DispatchFunction function, std::chrono::steady_clock::duration delay) = 0;
    virtual void cancel(task_id_t taskId) = 0;
};

} // namespace Valdi
