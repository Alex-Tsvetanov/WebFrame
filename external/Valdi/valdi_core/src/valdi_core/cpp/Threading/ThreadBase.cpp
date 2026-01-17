//
//  ThreadBase.cpp
//  valdi_core-ios
//
//  Created by Simon Corsin on 5/26/21.
//

#include "valdi_core/cpp/Threading/ThreadBase.hpp"
#include <atomic>

namespace Valdi {

static std::atomic<ThreadId> kThreadIdCounter = kThreadIdNull;

ThreadId getCurrentThreadId() {
    static thread_local ThreadId kThreadId = ++kThreadIdCounter;
    return kThreadId;
}

} // namespace Valdi
