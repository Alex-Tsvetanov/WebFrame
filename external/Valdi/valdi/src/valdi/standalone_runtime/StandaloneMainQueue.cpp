//
//  StandaloneMainQueue.cpp
//  valdi-pc
//
//  Created by Simon Corsin on 10/14/19.
//

#include "valdi/standalone_runtime/StandaloneMainQueue.hpp"
#include <future>

namespace Valdi {

class StandaloneMainThreadDispatcher : public IMainThreadDispatcher {
public:
    StandaloneMainThreadDispatcher(Weak<TaskQueue>&& taskQueue) : _taskQueue(std::move(taskQueue)) {}
    ~StandaloneMainThreadDispatcher() override = default;

    void dispatch(DispatchFunction* function, bool sync) final {
        auto taskQueue = _taskQueue.lock();
        if (taskQueue != nullptr) {
            if (sync) {
                std::promise<void> promise;
                auto future = promise.get_future();

                taskQueue->enqueue([&]() {
                    (*function)();
                    promise.set_value();
                });

                future.get();
            } else {
                taskQueue->enqueue(std::move(*function));
            }
        }

        delete function;
    }

private:
    Weak<TaskQueue> _taskQueue;
};

StandaloneMainQueue::StandaloneMainQueue() = default;
StandaloneMainQueue::~StandaloneMainQueue() = default;

int StandaloneMainQueue::runIndefinitely() {
    while (!isDisposed()) {
        runNextTask(std::chrono::steady_clock::now() + std::chrono::seconds(100000));
    }
    return _exitCode;
}

void StandaloneMainQueue::exit(int exitCode) {
    _exitCode = exitCode;
    dispose();
}

Ref<IMainThreadDispatcher> StandaloneMainQueue::createMainThreadDispatcher() {
    return makeShared<StandaloneMainThreadDispatcher>(weakRef(this));
}

} // namespace Valdi
