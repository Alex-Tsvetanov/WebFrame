//
//  AsyncGroup.hpp
//  valdi-ios
//
//  Created by Simon Corsin on 8/27/19.
//

#pragma once

#include "valdi_core/cpp/Utils/Function.hpp"
#include "valdi_core/cpp/Utils/Mutex.hpp"
#include "valdi_core/cpp/Utils/Shared.hpp"
#include <chrono>
#include <mutex>
#include <type_traits>
#include <utility>
#include <vector>

#include "utils/base/NonCopyable.hpp"

namespace Valdi {

// workaround to prevent compilation errors due to a two-phase lookup bug in libc++
static_assert(std::is_copy_constructible_v<snap::CopyableFunction<void()>>);
static_assert(!std::is_copy_constructible_v<snap::Function<void()>>);

class AsyncGroup : public SimpleRefCountable, public snap::NonCopyable {
public:
    AsyncGroup();
    ~AsyncGroup() override;

    void enter();
    void leave();

    bool leaveIfNotCompleted();

    void notify(DispatchFunction function);

    bool isCompleted() const;

    void blockingWait();
    /**
     Wait up to the given duration until all enter calls have been
     balanced by leave calls.
     Returns true if calls were balanced between now and the max time,
     returns false if it timed out.
     */
    bool blockingWaitWithTimeout(const std::chrono::steady_clock::duration& maxTime);

private:
    mutable Mutex _mutex;
    std::vector<std::pair<int, DispatchFunction>> _callbacks;
    int _enterCount = 0;
    int _callbackSequence = 0;

    int lockFreeEnqueueFunction(DispatchFunction function);
    void lockFreeRemoveFunction(int id);
    void doLeave(std::unique_lock<Mutex>& guard);
};

} // namespace Valdi
