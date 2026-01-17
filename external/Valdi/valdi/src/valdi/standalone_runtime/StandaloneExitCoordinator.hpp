//
//  StandaloneExitCoordinator.hpp
//  valdi-standalone_runtime
//
//  Created by Simon Corsin on 10/16/19.
//

#pragma once

#include "valdi_core/cpp/Threading/DispatchQueue.hpp"
#include "valdi_core/cpp/Threading/TaskQueue.hpp"

namespace Valdi {

class JavaScriptQueueListener;
class MainQueueListener;

class StandaloneExitCoordinator : public SharedPtrRefCountable {
public:
    StandaloneExitCoordinator(const Ref<DispatchQueue>& jsQueue, const Ref<TaskQueue>& mainQueue);
    ~StandaloneExitCoordinator() override;

    void postInit();

    void flushUpdatesSync();

    void setEnabled(bool enabled);

private:
    bool _enabled = false;
    Ref<DispatchQueue> _jsQueue;
    Ref<TaskQueue> _mainQueue;
    Ref<DispatchQueue> _coordinatorQueue;
    bool _jsQueueEmpty = false;
    bool _mainQueueEmpty = false;

    friend JavaScriptQueueListener;
    void onJsQueueEmpty(bool empty);

    friend MainQueueListener;
    void onMainQueueEmpty(bool empty);

    void exitIfNeeded();
};

} // namespace Valdi
