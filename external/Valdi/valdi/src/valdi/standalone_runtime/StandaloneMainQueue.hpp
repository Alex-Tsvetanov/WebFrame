//
//  StandaloneMainQueue.hpp
//  valdi-pc
//
//  Created by Simon Corsin on 10/14/19.
//

#pragma once

#include "valdi_core/cpp/Interfaces/IMainThreadDispatcher.hpp"
#include "valdi_core/cpp/Threading/TaskQueue.hpp"
#include <atomic>

namespace Valdi {

class StandaloneMainQueue : public TaskQueue {
public:
    StandaloneMainQueue();
    ~StandaloneMainQueue() override;

    int runIndefinitely();

    Ref<IMainThreadDispatcher> createMainThreadDispatcher();

    void exit(int exitCode);

private:
    std::atomic_int _exitCode = {0};
};

} // namespace Valdi
