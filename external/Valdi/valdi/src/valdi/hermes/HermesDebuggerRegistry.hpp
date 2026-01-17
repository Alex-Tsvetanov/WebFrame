//
//  HermesDebuggerRegistry.hpp
//  valdi-hermes
//
//  Created by Edward Lee on 4/8/24.
//

#pragma once

#include "valdi_core/cpp/Utils/Mutex.hpp"
#include "valdi_core/cpp/Utils/Shared.hpp"
#include "valdi_core/cpp/Utils/StringCache.hpp"
#include "valdi_core/cpp/Utils/Value.hpp"

#include <string>
#include <vector>

namespace hermes::vm {
class Runtime;
class RuntimeConfig;
} // namespace hermes::vm

namespace facebook::hermes {
class HermesRuntime;
namespace inspector_modern::chrome {
class CDPHandler;
}
} // namespace facebook::hermes

namespace Valdi {
class JavaScriptTaskScheduler;
class ILogger;
} // namespace Valdi

namespace Valdi::Hermes {

using HermesRuntimeId = StringBox;
static const HermesRuntimeId kRuntimeIdUndefined = STRING_LITERAL("<undefined>");

class HermesRuntimeMessageListener {
public:
    virtual ~HermesRuntimeMessageListener() = default;

    virtual void handleHermesMessage(const Value& message) = 0;
    virtual void handleHermesWorkerMessage(const Value& workerMessage, HermesRuntimeId workerId) = 0;
    virtual HermesRuntimeId getConnectedRuntimeId() = 0;
    virtual void onWorkerAttached(HermesRuntimeId workerId) = 0;
};

class HermesDebuggerRegistry : public SimpleRefCountable {
public:
    HermesDebuggerRegistry();
    ~HermesDebuggerRegistry() override;

    HermesRuntimeId append(const std::shared_ptr<facebook::hermes::HermesRuntime>& jsiRuntime,
                           const hermes::vm::RuntimeConfig& runtimeConfig,
                           JavaScriptTaskScheduler* taskScheduler,
                           HermesRuntimeId parentRuntimeId = kRuntimeIdUndefined);
    void remove(const HermesRuntimeId& runtimeId);

    void registerListener(HermesRuntimeMessageListener* listener);
    void unregisterListener(HermesRuntimeMessageListener* listener);

    void submitMessageToHermes(const Value& message, HermesRuntimeId runtimeId);

    std::vector<HermesRuntimeId> getRuntimeIds();
    std::vector<HermesRuntimeId> getWorkers(HermesRuntimeId runtimeId);

private:
    struct Entry {
        HermesRuntimeId runtimeId;
        HermesRuntimeId parentRuntimeId;
        std::shared_ptr<facebook::hermes::inspector_modern::chrome::CDPHandler> hermesHandler;
        bool isWorker() const {
            return parentRuntimeId != kRuntimeIdUndefined;
        }
    };
    Mutex _mutex;
    std::vector<Entry> _entries;
    [[maybe_unused]] size_t _runtimeIdSequence = 0;
    std::vector<HermesRuntimeMessageListener*> _listeners;

    void onHermesMessage(const std::string& message, HermesRuntimeId runtimeId);
};

} // namespace Valdi::Hermes