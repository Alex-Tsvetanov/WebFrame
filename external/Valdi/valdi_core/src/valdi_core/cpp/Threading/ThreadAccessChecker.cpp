//
//  ThreadAccessChecker.cpp
//  valdi-core
//
//  Created by Simon Corsin on 4/12/2024
//

#include "valdi_core/cpp/Threading/ThreadAccessChecker.hpp"
#include "utils/debugging/Assert.hpp"

namespace Valdi {

ThreadAccessChecker::ThreadAccessChecker() = default;

ThreadAccessCheckerGuard ThreadAccessChecker::guard() {
    auto currentThreadId = getCurrentThreadId();
    auto previousThreadId = _threadIdWithCurrentAccess.exchange(currentThreadId);
    SC_ABORT_UNLESS(previousThreadId == 0 || previousThreadId == currentThreadId,
                    "Unsafe multi-threaded access detected");

    return ThreadAccessCheckerGuard(*this, previousThreadId);
}

ThreadAccessCheckerGuard::ThreadAccessCheckerGuard(ThreadAccessChecker& checker, ThreadId previousThreadId)
    : _checker(checker), _previousThreadId(previousThreadId) {}

ThreadAccessCheckerGuard::~ThreadAccessCheckerGuard() {
    _checker._threadIdWithCurrentAccess = _previousThreadId;
}

} // namespace Valdi
