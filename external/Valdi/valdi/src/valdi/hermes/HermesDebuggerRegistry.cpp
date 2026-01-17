//
//  HermesDebuggerRegistry.cpp
//  valdi-hermes
//
//  Created by Edward Lee on 4/8/24.
//

#include "valdi/hermes/HermesDebuggerRegistry.hpp"
#include "valdi/hermes/Hermes.hpp"

#include "valdi/runtime/JavaScript/JavaScriptFunctionCallContext.hpp"
#include "valdi/runtime/JavaScript/JavaScriptTaskScheduler.hpp"
#include "valdi_core/cpp/Utils/LoggerUtils.hpp"
#include "valdi_core/cpp/Utils/ValueUtils.hpp"

#include <hermes/hermes.h>
#include <hermes/inspector/chrome/CDPHandler.h>

namespace Valdi::Hermes {

class RuntimeAdapterImpl : public facebook::hermes::inspector_modern::RuntimeAdapter {
public:
    RuntimeAdapterImpl(const std::shared_ptr<facebook::hermes::HermesRuntime>& jsiRuntime,
                       JavaScriptTaskScheduler* taskScheduler)
        : _jsiRuntime(jsiRuntime), _taskScheduler(taskScheduler) {}
    ~RuntimeAdapterImpl() override = default;

    facebook::hermes::HermesRuntime& getRuntime() final {
        return *_jsiRuntime;
    }

    void tickleJs() final {
        _taskScheduler->dispatchOnJsThreadAsync(nullptr, [](JavaScriptEntryParameters& entryParameters) {
            auto globalObject = entryParameters.jsContext.getGlobalObject(entryParameters.exceptionTracker);
            JSFunctionCallContext callContext(entryParameters.jsContext, nullptr, 0, entryParameters.exceptionTracker);
            entryParameters.jsContext.callObjectProperty(globalObject.get(), "__tickleJs", callContext);
        });
    }

private:
    std::shared_ptr<facebook::hermes::HermesRuntime> _jsiRuntime;
    Ref<JavaScriptTaskScheduler> _taskScheduler;
};

HermesDebuggerRegistry::HermesDebuggerRegistry() = default;
HermesDebuggerRegistry::~HermesDebuggerRegistry() = default;

HermesRuntimeId HermesDebuggerRegistry::append(const std::shared_ptr<facebook::hermes::HermesRuntime>& jsiRuntime,
                                               const hermes::vm::RuntimeConfig& runtimeConfig,
                                               JavaScriptTaskScheduler* taskScheduler,
                                               HermesRuntimeId parentRuntimeId) {
#if defined(HERMES_API)
    jsiRuntime->registerForProfiling();
    auto hermesHandler = facebook::hermes::inspector_modern::chrome::CDPHandler::create(
        std::make_unique<RuntimeAdapterImpl>(jsiRuntime, taskScheduler));
    std::lock_guard<Mutex> lock(_mutex);
    HermesRuntimeId runtimeId = StringCache::getGlobal().makeString(std::to_string(++_runtimeIdSequence));
    auto& entry = _entries.emplace_back();
    entry.runtimeId = runtimeId;
    entry.hermesHandler = hermesHandler;
    entry.parentRuntimeId = parentRuntimeId;

    hermesHandler->registerCallbacks(
        [this, runtimeId](const auto& message) { this->onHermesMessage(message, runtimeId); },
        /* no-op for the unregister handler */ []() {});

    if (parentRuntimeId != kRuntimeIdUndefined) {
        for (auto* listener : _listeners) {
            if (listener->getConnectedRuntimeId() == parentRuntimeId) {
                listener->onWorkerAttached(runtimeId);
            }
        }
    }
    return entry.runtimeId;
#else
    return kRuntimeIdUndefined;
#endif
}

void HermesDebuggerRegistry::remove(const HermesRuntimeId& runtimeId) {
    HermesDebuggerRegistry::Entry entry;
    {
        std::lock_guard<Mutex> lock(_mutex);
        auto it = _entries.begin();
        while (it != _entries.end()) {
            if (it->runtimeId == runtimeId) {
                entry = std::move(*it);
                it = _entries.erase(it);
            } else {
                it++;
            }
        }
    }
}

void HermesDebuggerRegistry::registerListener(HermesRuntimeMessageListener* listener) {
    std::lock_guard<Mutex> lock(_mutex);
    _listeners.emplace_back(listener);
}

void HermesDebuggerRegistry::unregisterListener(HermesRuntimeMessageListener* listener) {
    std::lock_guard<Mutex> lock(_mutex);
    auto it = _listeners.begin();
    while (it != _listeners.end()) {
        if (*it == listener) {
            it = _listeners.erase(it);
        } else {
            it++;
        }
    }
}

void HermesDebuggerRegistry::onHermesMessage(const std::string& messageStr, HermesRuntimeId runtimeId) {
    auto messageJson = jsonToValue(messageStr);
    if (!messageJson.success()) {
        return;
    }
    auto message = messageJson.value();

    std::lock_guard<Mutex> lock(_mutex);
    bool isWorker = false;
    HermesRuntimeId workerId = kRuntimeIdUndefined;
    for (auto& entry : _entries) {
        if (entry.runtimeId == runtimeId) {
            if (entry.isWorker()) {
                runtimeId = entry.parentRuntimeId;
                workerId = entry.runtimeId;
                isWorker = true;
            }
            break;
        }
    }

    for (auto* listener : _listeners) {
        if (listener->getConnectedRuntimeId() == runtimeId) {
            if (isWorker) {
                listener->handleHermesWorkerMessage(message, workerId);
            } else {
                listener->handleHermesMessage(message);
            }
        }
    }
}

std::vector<HermesRuntimeId> HermesDebuggerRegistry::getRuntimeIds() {
    std::lock_guard<Mutex> lock(_mutex);
    std::vector<HermesRuntimeId> runtimeIds;
    for (auto entry : _entries) {
        if (!entry.isWorker()) {
            runtimeIds.push_back(entry.runtimeId);
        }
    }
    return runtimeIds;
}

std::vector<HermesRuntimeId> HermesDebuggerRegistry::getWorkers(HermesRuntimeId runtimeId) {
    std::lock_guard<Mutex> lock(_mutex);
    std::vector<HermesRuntimeId> runtimeIds;
    for (auto entry : _entries) {
        if (entry.parentRuntimeId == runtimeId) {
            runtimeIds.push_back(entry.runtimeId);
        }
    }
    return runtimeIds;
}

void HermesDebuggerRegistry::submitMessageToHermes(const Value& message, HermesRuntimeId runtimeId) {
    std::shared_ptr<facebook::hermes::inspector_modern::chrome::CDPHandler> hermesHandler = nullptr;
    {
        std::lock_guard<Mutex> lock(_mutex);
        for (auto entry : _entries) {
            if (entry.runtimeId == runtimeId) {
                hermesHandler = entry.hermesHandler;
            }
        }
    }
    if (hermesHandler)
        hermesHandler->handle(valueToJsonString(message));
    return;
}

} // namespace Valdi::Hermes