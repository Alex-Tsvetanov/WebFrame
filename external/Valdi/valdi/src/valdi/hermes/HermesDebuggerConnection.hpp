//
//  HermesDebuggerConnection.hpp
//  valdi-hermes
//
//  Created by Edward Lee on 4/8/24.
//

#pragma once

#include "valdi/hermes/HermesDebuggerRegistry.hpp"
#include "valdi_core/cpp/Utils/ByteBuffer.hpp"
#include "valdi_core/cpp/Utils/FlatMap.hpp"
#include "valdi_core/cpp/Utils/Mutex.hpp"
#include "valdi_core/cpp/Utils/Shared.hpp"
#include "valdi_core/cpp/Utils/Value.hpp"

#include <vector>

namespace Valdi::Hermes {

class HermesDebuggerConnection : public HermesRuntimeMessageListener, public SharedPtrRefCountable {
public:
    HermesDebuggerConnection(const Ref<HermesDebuggerRegistry>& registry, HermesRuntimeId runtimeId, ILogger& logger);
    ~HermesDebuggerConnection() override;

    void handleCDPMessage(const Value& message);
    void handleHermesMessage(const Value& message) final;
    void handleHermesWorkerMessage(const Value& message, HermesRuntimeId workerId) final;
    void onWorkerAttached(HermesRuntimeId workerId) final;
    HermesRuntimeId getConnectedRuntimeId() final;

    using CDPMessageListener = std::function<void(const Value&)>;
    void setCDPMessageListener(CDPMessageListener listener);

    static Value createEvent(const StringBox& method);
    static Value createEvent(const StringBox& method, int id);

private:
    Ref<HermesDebuggerRegistry> _registry;
    HermesRuntimeId _connectedRuntimeId;
    [[maybe_unused]] ILogger& _logger;
    Mutex _mutex;

    bool _workersEnabled;
    std::vector<HermesRuntimeId> _attachedWorkers;

    CDPMessageListener _cdpMessageListener = nullptr;
    using DebuggerMethodHandler = void (HermesDebuggerConnection::*)(const Value&);
    FlatMap<StringBox, DebuggerMethodHandler> _hermesResponseHandlers;
    FlatMap<StringBox, DebuggerMethodHandler> _cdpMethodHandlers;
    FlatMap<StringBox, DebuggerMethodHandler> _hermesMethodHandlers;
    FlatMap<int, DebuggerMethodHandler> _hermesResponseHandlerForRequest;

    bool _runIfWaitingForDebuggerCalled = false;
    FlatMap<StringBox, Value> _deferredScriptParsedMessages;
    FlatMap<StringBox, StringBox> _scriptIdsByUrl;
    FlatSet<StringBox> _latestScriptIds;

    bool tryHandleHermesMessageInternally(const Value& message);
    void submitMessageToCDP(const Value& value);

    bool tryHandleCdpMessageInternally(const Value& message);
    void submitMessageToHermes(const Value& message, HermesRuntimeId runtimeId = kRuntimeIdUndefined);

    void bindCdpMethodHandler(const StringBox& method, DebuggerMethodHandler handler);
    void bindHermesResponseHandler(const StringBox& method, DebuggerMethodHandler handler);
    void bindHermesMethodHandler(const StringBox& method, DebuggerMethodHandler handler);

    void handleGetDebugTargets(const Value& message);
    void handleSendMessageToWorker(const Value& message);
    void handleWorkerEnabled(const Value& message);
    void handleWorkerDisable(const Value& message);
    void handleWorkerDetach(const Value& message);
    void handleCallFunctionOn(const Value& message);
    void handlePaused(const Value& message);
    void handleScriptParsed(const Value& message);
    void handleRunIfWaitingForDebugger(const Value& message);
    void handleSetBreakpointResponse(const Value& message);

    void handleProfilerStop(const Value& message);
    void handleGetProperties(const Value& message);
    void handleResponse(const Value& message);

    static Value createNodeWorkerAttachedEvent(HermesRuntimeId workerId);
    static Value createNodeWorkerDetachedEvent(HermesRuntimeId workerId);
    static Value createReceivedMessageFromWorkerEvent(HermesRuntimeId workerId, const Value& messageFromWorker);

    static Value emptyResponse(const Value& message);
    static Value emptyErrorResponse(const Value& message);
};

} // namespace Valdi::Hermes
