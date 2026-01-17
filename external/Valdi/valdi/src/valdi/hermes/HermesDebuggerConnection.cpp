//
//  HermesDebuggerConnection.cpp
//  valdi-hermes
//
//  Created by Edward Lee on 4/8/24.
//

#include "valdi/hermes/HermesDebuggerConnection.hpp"
#include "valdi_core/cpp/Utils/LoggerUtils.hpp"
#include "valdi_core/cpp/Utils/ValueUtils.hpp"
#include <unordered_set>

namespace Valdi::Hermes {

HermesDebuggerConnection::HermesDebuggerConnection(const Ref<HermesDebuggerRegistry>& registry,
                                                   HermesRuntimeId runtimeId,
                                                   ILogger& logger)
    : _registry(registry), _connectedRuntimeId(runtimeId), _logger(logger) {
    _registry->registerListener(this);

    bindCdpMethodHandler(STRING_LITERAL("NodeWorker.enable"), &HermesDebuggerConnection::handleWorkerEnabled);
    bindCdpMethodHandler(STRING_LITERAL("NodeWorker.disable"), &HermesDebuggerConnection::handleWorkerDisable);
    bindCdpMethodHandler(STRING_LITERAL("NodeWorker.detach"), &HermesDebuggerConnection::handleWorkerDetach);
    bindCdpMethodHandler(STRING_LITERAL("NodeWorker.sendMessageToWorker"),
                         &HermesDebuggerConnection::handleSendMessageToWorker);

    /* Compatability workarounds for vscode-js-debug:
     * 1. Runtime.callFunctionOn causes Herems to crash. This handler returns an undefined value as the response.
     * 2. Debugger.scriptParsed is deferred until Runtime.runIfWaitingForDebugger is called, otherwise it won't show up
     * in the UI.
     * 3. Profiler.stop is used to fix source mapping in the profiler.
     * 4. Runtime.getProperties is used to remove duplicate variables so they appear properly in the debugger.
     * 5. Debugger.setBreakpointByUrl is used to fix multiple breakpoint locations returned after hotreloading.
     * 6. Debugger.paused is used to clean up function names in the callFrames.
     */
    bindCdpMethodHandler(STRING_LITERAL("Runtime.callFunctionOn"), &HermesDebuggerConnection::handleCallFunctionOn);
    bindHermesMethodHandler(STRING_LITERAL("Debugger.paused"), &HermesDebuggerConnection::handlePaused);
    bindHermesMethodHandler(STRING_LITERAL("Debugger.scriptParsed"), &HermesDebuggerConnection::handleScriptParsed);
    bindHermesResponseHandler(STRING_LITERAL("Debugger.setBreakpointByUrl"),
                              &HermesDebuggerConnection::handleSetBreakpointResponse);
    bindCdpMethodHandler(STRING_LITERAL("Runtime.runIfWaitingForDebugger"),
                         &HermesDebuggerConnection::handleRunIfWaitingForDebugger);

    bindHermesResponseHandler(STRING_LITERAL("Profiler.stop"), &HermesDebuggerConnection::handleProfilerStop);
    bindHermesResponseHandler(STRING_LITERAL("Runtime.getProperties"), &HermesDebuggerConnection::handleGetProperties);
}

HermesDebuggerConnection::~HermesDebuggerConnection() {
    submitMessageToHermes(createEvent(STRING_LITERAL("Debugger.disable"), 0));
    _registry->unregisterListener(this);
}

void HermesDebuggerConnection::handleHermesMessage(const Value& message) {
    bool handledInternally = tryHandleHermesMessageInternally(message);
    if (!handledInternally) {
        submitMessageToCDP(message);
    }
}

void HermesDebuggerConnection::handleHermesWorkerMessage(const Value& workerMessage, HermesRuntimeId workerId) {
    if (!_workersEnabled) {
        return;
    }
    auto message = createReceivedMessageFromWorkerEvent(workerId, workerMessage);
    // TODO: Do we need special handling for worker messages too?
    submitMessageToCDP(message);
}

bool HermesDebuggerConnection::tryHandleHermesMessageInternally(const Value& message) {
    DebuggerMethodHandler methodHandler = nullptr;
    auto method = message.getMapValue("method");
    auto requestId = message.getMapValue("id");
    // Methods from Hermes have a "method" key, while responses have a "id" key which corresponds to the matching
    // request's id.
    if (!method.isNullOrUndefined()) {
        auto methodHandlerItr = _hermesMethodHandlers.find(method.toStringBox());
        if (methodHandlerItr != _hermesMethodHandlers.end()) {
            methodHandler = methodHandlerItr->second;
        }
    } else if (!requestId.isNullOrUndefined()) {
        auto methodHandlerItr = _hermesResponseHandlerForRequest.find(requestId.toInt());
        if (methodHandlerItr != _hermesResponseHandlerForRequest.end()) {
            methodHandler = methodHandlerItr->second;
            _hermesResponseHandlerForRequest.erase(methodHandlerItr);
        }
    }

    if (methodHandler) {
        (this->*methodHandler)(message);
        return true;
    }
    return false;
}

void HermesDebuggerConnection::setCDPMessageListener(CDPMessageListener listener) {
    _cdpMessageListener = listener;
}

void HermesDebuggerConnection::submitMessageToCDP(const Value& value) {
    if (_cdpMessageListener) {
        _cdpMessageListener(value);
    }
}

void HermesDebuggerConnection::handleCDPMessage(const Value& message) {
    bool handledInternally = tryHandleCdpMessageInternally(message);
    if (!handledInternally) {
        submitMessageToHermes(message);
    }
}

bool HermesDebuggerConnection::tryHandleCdpMessageInternally(const Value& message) {
    auto method = message.getMapValue("method").toStringBox();

    // If there is a response handler for this method, store the method for the requestId to be called later.
    auto responseMethodHandler = _hermesResponseHandlers.find(method);
    if (responseMethodHandler != _hermesResponseHandlers.end()) {
        _hermesResponseHandlerForRequest[message.getMapValue("id").toInt()] = _hermesResponseHandlers[method];
    }

    auto methodHandler = _cdpMethodHandlers.find(method);
    if (methodHandler == _cdpMethodHandlers.end()) {
        return false;
    }

    (this->*methodHandler->second)(message);
    return true;
}

void HermesDebuggerConnection::submitMessageToHermes(const Value& message, HermesRuntimeId runtimeId) {
    if (runtimeId == kRuntimeIdUndefined) {
        runtimeId = getConnectedRuntimeId();
    }
    _registry->submitMessageToHermes(message, runtimeId);
}

void HermesDebuggerConnection::handleSendMessageToWorker(const Value& message) {
    auto workerId = message.getMapValue("params").getMapValue("sessionId").toStringBox();
    auto messageForWorker = message.getMapValue("params").getMapValue("message");
    submitMessageToHermes(messageForWorker, workerId);
    submitMessageToCDP(emptyResponse(message));
    return;
}

void HermesDebuggerConnection::handleWorkerEnabled(const Value& message) {
    _workersEnabled = true;
    for (auto workerRuntimeId : _attachedWorkers) {
        submitMessageToCDP(createNodeWorkerAttachedEvent(workerRuntimeId));
    }
    submitMessageToCDP(emptyResponse(message));
    return;
}

void HermesDebuggerConnection::handleCallFunctionOn(const Value& message) {
    auto result = Value().setMapValue("result", Value().setMapValue("type", Value("undefined")));
    auto response = emptyResponse(message).setMapValue("result", result);
    submitMessageToCDP(response);
    return;
}

void HermesDebuggerConnection::handleWorkerDisable(const Value& message) {
    _workersEnabled = false;
    submitMessageToCDP(emptyResponse(message));
    return;
}

void HermesDebuggerConnection::handleWorkerDetach(const Value& message) {
    auto detachWorkerId = message.getMapValue("params").getMapValue("sessionId").toStringBox();
    auto it = std::find(_attachedWorkers.begin(), _attachedWorkers.end(), detachWorkerId);
    if (it != _attachedWorkers.end()) {
        _attachedWorkers.erase(it);
        submitMessageToCDP(createNodeWorkerDetachedEvent(detachWorkerId));
    }
    submitMessageToCDP(emptyResponse(message));
    return;
}

static StringBox cleanFunctionName(const StringBox& functionName) {
    if (functionName.length() > 2 && functionName.hasPrefix("#") && functionName.hasSuffix("#")) {
        return functionName.substring(1, functionName.length() - 1);
    } else {
        return functionName;
    }
}

// Compatability workaround for proper Source mapping in Hermes Profiler
// Replace scriptIds in the profile nodes with the actual scriptIds based on the URL
void HermesDebuggerConnection::handleProfilerStop(const Value& message) {
    auto nodeArray = message.getMapValue("result").getMapValue("profile").getMapValue("nodes").getArrayRef();
    if (nodeArray != nullptr) {
        for (size_t i = 0; i < nodeArray->size(); i++) {
            auto callFrame = (*nodeArray)[i].getMapValue("callFrame");
            auto functionName = callFrame.getMapValue("functionName").toStringBox();
            callFrame.setMapValue("functionName", Value(cleanFunctionName(functionName)));
            auto url = callFrame.getMapValue("url").toStringBox();
            if (_scriptIdsByUrl.find(url) != _scriptIdsByUrl.end()) {
                auto scriptId = _scriptIdsByUrl[url];
                callFrame.setMapValue("scriptId", Value(scriptId));
            }
        }
    }
    submitMessageToCDP(message);
}

// Compatability workaround for debugging Hermes with Block Scoping enabled
// Issue outlined here: https://github.com/facebook/hermes/issues/1355. Should be fixed in a future release of Static
// Hermes. This workaround removes duplicate variables from the getProperties response so they display properly in the
// UI
void HermesDebuggerConnection::handleGetProperties(const Value& message) {
    auto properties = message.getMapValue("result").getMapValue("result").getArrayRef();
    if (properties != nullptr) {
        struct ValueCompare {
            bool operator()(const Value& lhs, const Value& rhs) const {
                return lhs.getMapValue("name") == rhs.getMapValue("name");
            }
        };
        std::unordered_set<Value, std::hash<Value>, ValueCompare> uniqueValues;
        for (const auto& property : *properties) {
            uniqueValues.insert(property);
        }
        std::vector<Value> uniqueProperties(uniqueValues.begin(), uniqueValues.end());
        message.getMapValue("result").setMapValue("result", Value(ValueArray::make(std::move(uniqueProperties))));
    }
    submitMessageToCDP(message);
}

void HermesDebuggerConnection::handlePaused(const Value& message) {
    auto callFrames = message.getMapValue("params").getMapValue("callFrames").getArrayRef();
    if (callFrames != nullptr) {
        for (size_t i = 0; i < callFrames->size(); i++) {
            auto functionName = (*callFrames)[i].getMapValue("functionName").toStringBox();
            (*callFrames)[i].setMapValue("functionName", Value(cleanFunctionName(functionName)));
        }
    }
    submitMessageToCDP(message);
}

// When setting a breakpoint after hotreloading, it may return multiple locations.
// This can confuse the debugger since the old script was unloaded and it won't display
// properly in the UI. This handler will make sure only the most recently loaded script
// (stored in _latestScriptIds) is returned as the breakpoint location.
void HermesDebuggerConnection::handleSetBreakpointResponse(const Value& message) {
    auto locations = message.getMapValue("result").getMapValue("locations").getArrayRef();
    bool breakpointAdjusted = false;
    if (locations != nullptr && locations->size() > 1) {
        for (const auto& location : *locations) {
            auto scriptId = location.getMapValue("scriptId").toStringBox();

            // Adjust the message to only include the location for the most recently loaded script
            if (_latestScriptIds.find(scriptId) != _latestScriptIds.end()) {
                message.getMapValue("result").setMapValue("locations", Value(ValueArray::make({location})));
                breakpointAdjusted = true;
                break;
            }
        }

        // TODO: Known Issue: If the script is reverted back to a previously loaded version (A->B->A), the locations
        // array will not include the latest scriptId, and breakpoints for the file cannot be properly set.
        // This is a bug in the Hermes codebase and will be fixed in the future.
        if (!breakpointAdjusted) {
            VALDI_WARN(_logger,
                       "Debugger breakpoint may not behave properly due to reloading a previous version of script.");
        }
    }
    submitMessageToCDP(message);
}

void HermesDebuggerConnection::handleScriptParsed(const Value& message) {
    auto url = message.getMapValue("params").getMapValue("url").toStringBox();
    auto scriptId = message.getMapValue("params").getMapValue("scriptId").toStringBox();

    // Remove stale scriptIds from the latestScriptIds set
    if (_scriptIdsByUrl.find(url) != _scriptIdsByUrl.end()) {
        auto staleScriptId = _scriptIdsByUrl[url];
        _latestScriptIds.erase(staleScriptId);
    }

    _scriptIdsByUrl[url] = scriptId;
    _latestScriptIds.insert(scriptId);

    if (_runIfWaitingForDebuggerCalled) {
        submitMessageToCDP(message);
    } else {
        // Make sure only the latest scriptParsed message is sent to the debugger for a given URL
        _deferredScriptParsedMessages[url] = message;
    }
    return;
}

void HermesDebuggerConnection::handleRunIfWaitingForDebugger(const Value& message) {
    if (!_runIfWaitingForDebuggerCalled) {
        _runIfWaitingForDebuggerCalled = true;
        auto deferredMessages = std::move(_deferredScriptParsedMessages);
        for (const auto& [_, message] : deferredMessages) {
            submitMessageToCDP(message);
        }
    }
    submitMessageToHermes(message);
}

void HermesDebuggerConnection::onWorkerAttached(HermesRuntimeId workerId) {
    std::lock_guard<Mutex> lock(_mutex);
    _attachedWorkers.push_back(workerId);
    if (_workersEnabled) {
        submitMessageToCDP(createNodeWorkerAttachedEvent(workerId));
    }
}

Value HermesDebuggerConnection::createNodeWorkerAttachedEvent(HermesRuntimeId workerId) {
    return createEvent(STRING_LITERAL("NodeWorker.attachedToWorker"))
        .setMapValue("params",
                     Value()
                         .setMapValue("sessionId", Value(workerId))
                         .setMapValue("waitingForDebugger", Value(false))
                         .setMapValue("workerInfo",
                                      Value()
                                          .setMapValue("workerId", Value(workerId))
                                          .setMapValue("type", Value("node"))
                                          .setMapValue("title", Value("ValdiWorker"))
                                          .setMapValue("url", Value(""))));
}

Value HermesDebuggerConnection::createNodeWorkerDetachedEvent(HermesRuntimeId workerId) {
    return createEvent(STRING_LITERAL("NodeWorker.detachedFromWorker"))
        .setMapValue("params", Value().setMapValue("sessionId", Value(workerId)));
}

Value HermesDebuggerConnection::createReceivedMessageFromWorkerEvent(HermesRuntimeId workerId,
                                                                     const Value& messageFromWorker) {
    return createEvent(STRING_LITERAL("NodeWorker.receivedMessageFromWorker"))
        .setMapValue("params",
                     Value().setMapValue("sessionId", Value(workerId)).setMapValue("message", messageFromWorker));
}

Value HermesDebuggerConnection::createEvent(const StringBox& method) {
    return Value().setMapValue("method", Value(method)).setMapValue("params", Value(makeShared<ValueMap>()));
}

Value HermesDebuggerConnection::createEvent(const StringBox& method, int id) {
    return createEvent(method).setMapValue("id", Value(id));
}

Value HermesDebuggerConnection::emptyResponse(const Value& message) {
    return Value()
        .setMapValue("id", Value(message.getMapValue("id").toInt()))
        .setMapValue("result", Value(makeShared<ValueMap>()));
}

Value HermesDebuggerConnection::emptyErrorResponse(const Value& message) {
    return Value()
        .setMapValue("id", Value(message.getMapValue("id").toInt()))
        .setMapValue("error", Value(makeShared<ValueMap>()));
}

HermesRuntimeId HermesDebuggerConnection::getConnectedRuntimeId() {
    return _connectedRuntimeId;
}

void HermesDebuggerConnection::bindCdpMethodHandler(const StringBox& method, DebuggerMethodHandler handler) {
    _cdpMethodHandlers[method] = handler;
}

void HermesDebuggerConnection::bindHermesMethodHandler(const StringBox& method, DebuggerMethodHandler handler) {
    _hermesMethodHandlers[method] = handler;
}

void HermesDebuggerConnection::bindHermesResponseHandler(const StringBox& method, DebuggerMethodHandler handler) {
    _hermesResponseHandlers[method] = handler;
}

} // namespace Valdi::Hermes