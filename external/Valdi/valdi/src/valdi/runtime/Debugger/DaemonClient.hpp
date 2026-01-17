//
//  DaemonClient.hpp
//  valdi-ios
//
//  Created by Simon Corsin on 2/16/19.
//

#pragma once

#include "valdi/runtime/Resources/Resource.hpp"
#include "valdi/runtime/Utils/SharedContainers.hpp"
#include "valdi/valdi.pb.h"
#include "valdi_core/Platform.hpp"
#include "valdi_core/cpp/Interfaces/ILogger.hpp"
#include "valdi_core/cpp/Resources/ValdiPacket.hpp"
#include "valdi_core/cpp/Utils/Bytes.hpp"
#include "valdi_core/cpp/Utils/Error.hpp"
#include "valdi_core/cpp/Utils/FlatMap.hpp"
#include "valdi_core/cpp/Utils/Function.hpp"
#include "valdi_core/cpp/Utils/LoggerUtils.hpp"
#include "valdi_core/cpp/Utils/Mutex.hpp"
#include "valdi_core/cpp/Utils/Shared.hpp"

#include "valdi/runtime/Debugger/IDaemonClient.hpp"

#include <memory>

namespace Valdi {

class ILogger;
class MainThreadManager;
class DaemonClient;

class DaemonClientListener {
public:
    DaemonClientListener() = default;
    virtual ~DaemonClientListener() = default;

    virtual void didReceiveUpdatedResources(const SharedVector<Shared<Resource>>& resources) = 0;
    virtual void didReceiveClientPayload(DaemonClient* daemonClient, int senderClientId, const BytesView& payload) = 0;

    virtual void daemonClientDidDisconnect(DaemonClient* daemonClient, const Error& error) = 0;
};

class DaemonClientDataSender {
public:
    DaemonClientDataSender() = default;
    virtual ~DaemonClientDataSender() = default;

    virtual void submitData(const BytesView& data) = 0;

    /**
     Whether submit data should be called with the packet already
     encoded.
     */
    virtual bool requiresPacketEncoding() const = 0;
};

struct DaemonClientPendingResponse {
    Function<void(const Value&)> handler;

    explicit DaemonClientPendingResponse(Function<void(const Value&)> handler);
};

class DaemonClient : public SharedPtrRefCountable, public IDaemonClient {
public:
    DaemonClient(DaemonClientListener* listener, int connectionId, const Ref<ILogger>& logger);
    ~DaemonClient() override;

    int getConnectionId() const override;

    void setDataSender(const Shared<DaemonClientDataSender>& dataSender);
    Shared<DaemonClientDataSender> getDataSender() const;

    void sendConfiguration(const StringBox& applicationId, snap::valdi_core::Platform platform, bool disableHotReload);

    void closeClient(const Error& error);

    void sendLog(LogType logType, const std::string& message);

    void onDataReceived(const BytesView& bytes);

    void submitRequest(const Value& payload, const Ref<ValueFunction>& callback) override;

    void sendJsDebuggerInfo(uint16_t port, const std::vector<StringBox>& websocketTargets) override;

private:
    DaemonClientListener* _listener;
    int _connectionId;
    [[maybe_unused]] Ref<ILogger> _logger;
    Shared<DaemonClientDataSender> _dataSender;
    mutable Mutex _mutex;
    FlatMap<StringBox, Shared<DaemonClientPendingResponse>> _pendingResponses;
    int64_t _requestIdSequence = 0;
    ValdiPacketStream _packetStream;

    bool consumeBufferedData();

    void processPayload(const Value& payload);
    void processRequest(const Value& request);
    void processEvent(const Value& event);
    void processResponse(const Value& response);

    Shared<DaemonClientPendingResponse> getPendingResponse(const StringBox& requestId);

    void submitPayload(const Value& payload) const;

    void submitRequest(Value request, Function<void(const Value&)> completionHandler);
};

} // namespace Valdi
