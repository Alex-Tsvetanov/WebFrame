//
//  DaemonClient.cpp
//  valdi-ios
//
//  Created by Simon Corsin on 2/16/19.
//

#include "valdi/runtime/Debugger/DaemonClient.hpp"

#include "valdi_core/cpp/Resources/ValdiPacket.hpp"
#include "valdi_core/cpp/Utils/ByteBuffer.hpp"
#include "valdi_core/cpp/Utils/LoggerUtils.hpp"
#include "valdi_core/cpp/Utils/StringCache.hpp"
#include "valdi_core/cpp/Utils/ValueArrayBuilder.hpp"
#include "valdi_core/cpp/Utils/ValueFunctionWithCallable.hpp"
#include "valdi_core/cpp/Utils/ValueTypedArray.hpp"
#include "valdi_core/cpp/Utils/ValueUtils.hpp"

#include "utils/encoding/Base64Utils.hpp"

namespace Valdi {

DaemonClient::DaemonClient(DaemonClientListener* listener, int connectionId, const Ref<ILogger>& logger)
    : _listener(listener), _connectionId(connectionId), _logger(logger) {}

DaemonClient::~DaemonClient() = default;

int DaemonClient::getConnectionId() const {
    return _connectionId;
}

void DaemonClient::closeClient(const Error& error) {
    _listener->daemonClientDidDisconnect(this, error);
}

void DaemonClient::sendConfiguration(const StringBox& applicationId,
                                     snap::valdi_core::Platform platform,
                                     bool disableHotReload) {
    Value configure;
    configure.setMapValue("application_id", Value(applicationId));
    switch (platform) {
        case snap::valdi_core::Platform::Ios:
            configure.setMapValue("platform", Value(STRING_LITERAL("ios")));
            break;
        case snap::valdi_core::Platform::Android:
            configure.setMapValue("platform", Value(STRING_LITERAL("android")));
            break;
        case snap::valdi_core::Platform::Skia:
            break;
    }
    configure.setMapValue("disable_hot_reload", Value(disableHotReload));
    auto callback = [&](const Value& response) -> bool {
        VALDI_DEBUG(*_logger, "Submitted Daemon configuration");
        return false;
    };
    Value request;
    request.setMapValue("configure", configure);
    submitRequest(request, callback);
}

void DaemonClient::sendLog(LogType logType, const std::string& message) {
    Value log;
    switch (logType) {
        case LogTypeDebug:
            log.setMapValue("severity", Value(STRING_LITERAL("debug")));
            break;
        case LogTypeInfo:
            log.setMapValue("severity", Value(STRING_LITERAL("info")));
            break;
        case LogTypeWarn:
            log.setMapValue("severity", Value(STRING_LITERAL("warn")));
            break;
        case LogTypeError:
            log.setMapValue("severity", Value(STRING_LITERAL("error")));
            break;
        case LogTypeCount:
            break;
    }
    log.setMapValue("log", Value(message));
    auto logs = ValueArray::make(1);
    logs->emplace(0, log);
    Value newLogs;
    newLogs.setMapValue("new_logs", Value(logs));
    Value json;
    json.setMapValue("event", newLogs);
    submitPayload(json);
}

void DaemonClient::submitRequest(const Value& payload, const Ref<ValueFunction>& callback) {
    submitRequest(payload, [=](const Value& response) {
        if (callback == nullptr) {
            return;
        }

        (*callback)({response});
    });
}

void DaemonClient::setDataSender(const Shared<DaemonClientDataSender>& dataSender) {
    std::lock_guard<Mutex> lock(_mutex);
    _dataSender = dataSender;
}

void DaemonClient::processRequest(const Value& request) {
    auto sendLogs = request.getMapValue("send_logs");
    if (sendLogs.isNullOrUndefined()) {
        VALDI_ERROR(*_logger, "Invalid daemon client payload, request not set");
    }
}

static BytesView getDataBytesForKey(const Value& value, std::string_view stringKey, std::string_view base64Key) {
    auto stringValue = value.getMapValue(stringKey);
    if (stringValue.isString()) {
        auto str = stringValue.toStringBox();
        return BytesView(str.getInternedString(), reinterpret_cast<const Byte*>(str.getCStr()), str.length());
    }

    auto base64String = value.getMapValue(base64Key).toStringBox();
    auto data = makeShared<Bytes>();
    snap::utils::encoding::base64ToBinary(base64String.toStringView(), *data);
    return BytesView(data);
}

void DaemonClient::processEvent(const Value& event) {
    auto updatedResources = event.getMapValue("updated_resources");
    if (!updatedResources.isNullOrUndefined()) {
        auto resources = Valdi::makeShared<std::vector<Shared<Resource>>>();
        for (const auto& resource : *updatedResources.getMapValue("resources").getArray()) {
            auto bundleName = resource.getMapValue("bundle_name").toStringBox();
            auto path = resource.getMapValue("file_path_within_bundle").toStringBox();

            auto dataBytesView = getDataBytesForKey(resource, "data_string", "data_base64");

            auto outResource =
                Valdi::makeShared<Resource>(ResourceId(std::move(bundleName), std::move(path)), dataBytesView);
            resources->emplace_back(std::move(outResource));
        }

        _listener->didReceiveUpdatedResources(resources);
        return;
    }

    auto payloadFromClient = event.getMapValue("payload_from_client");
    if (!payloadFromClient.isNullOrUndefined()) {
        auto dataBytesView = getDataBytesForKey(payloadFromClient, "payload_string", "payload_base64");

        _listener->didReceiveClientPayload(
            this, payloadFromClient.getMapValue("sender_client_id").toInt(), dataBytesView);
        return;
    }

    // If we make it this far, something has gone wrong.
    VALDI_ERROR(*_logger, "Invalid daemon client payload, event not set");
}

Shared<DaemonClientDataSender> DaemonClient::getDataSender() const {
    std::lock_guard<Mutex> lock(_mutex);
    return _dataSender;
}

void DaemonClient::submitPayload(const Value& payload) const {
    auto data = valueToJson(payload);

    auto dataSender = getDataSender();
    if (dataSender == nullptr) {
        return;
    }

    auto bytes = makeShared<ByteBuffer>();
    if (dataSender->requiresPacketEncoding()) {
        ValdiPacket::write(data->data(), data->size(), *bytes);
    } else {
        bytes = data;
    }
    dataSender->submitData(bytes->toBytesView());
}

void DaemonClient::submitRequest(Value request, Function<void(const Value&)> completionHandler) {
    auto pendingResponse = Valdi::makeShared<DaemonClientPendingResponse>(completionHandler);
    {
        std::lock_guard<Mutex> lock(_mutex);
        auto requestId = StringCache::getGlobal().makeString(std::to_string(++_requestIdSequence));

        _pendingResponses[requestId] = pendingResponse;
        request.setMapValue("request_id", Value(requestId));
    }
    Value payload;
    payload.setMapValue("request", request);
    submitPayload(payload);
}

Shared<DaemonClientPendingResponse> DaemonClient::getPendingResponse(const StringBox& requestId) {
    Shared<DaemonClientPendingResponse> pendingResponse;

    std::lock_guard<Mutex> lock(_mutex);
    const auto it = _pendingResponses.find(requestId);
    if (it != _pendingResponses.end()) {
        pendingResponse = it->second;
        _pendingResponses.erase(it);
    }
    return pendingResponse;
}

void DaemonClient::processResponse(const Value& response) {
    auto requestId = response.getMapValue("request_id");
    if (requestId.isNullOrUndefined()) {
        VALDI_INFO(*_logger, "Missing request id for object: {}", response.toString());
        return;
    }
    auto pendingResponse = getPendingResponse(requestId.toStringBox());
    if (pendingResponse == nullptr) {
        VALDI_ERROR(*_logger, "Unable to find pending response for requestId {}", response.getMapValue("request_id"));
        return;
    }

    pendingResponse->handler(response);
}

void DaemonClient::processPayload(const Value& payload) {
    auto request = payload.getMapValue("request");
    if (!request.isNullOrUndefined()) {
        processRequest(request);
        return;
    }

    auto response = payload.getMapValue("response");
    if (!response.isNullOrUndefined()) {
        processResponse(response);
        return;
    }

    auto event = payload.getMapValue("event");
    if (!event.isNullOrUndefined()) {
        processEvent(event);
        return;
    }

    VALDI_ERROR(*_logger, "Invalid daemon client payload, content not set");
}

void DaemonClient::onDataReceived(const BytesView& bytes) {
    // onDataReceived is called on a raw TCP connection, we need to maintain a buffer
    {
        std::lock_guard<Mutex> lock(_mutex);
        _packetStream.write(bytes);
    }

    while (consumeBufferedData()) {
    }
}

bool DaemonClient::consumeBufferedData() {
    Result<BytesView> result;
    {
        std::lock_guard<Mutex> lock(_mutex);
        result = _packetStream.read();
    }

    if (!result) {
        if (result.error() == ValdiPacket::incompletePacketError()) {
            return false;
        }

        // Something bad happened, closing connection
        VALDI_ERROR(*_logger, "Received invalid packet: {}, disconnecting daemon client...", result.error());
        closeClient(result.error());
        return false;
    }

    auto payloadData = result.moveValue();

    auto payload = jsonToValue(payloadData.data(), payloadData.size());
    if (payload) {
        processPayload(payload.value());
    } else {
        VALDI_ERROR(*_logger, "Failed to parse packet: {}", payload.error());
    }

    return true;
}

void DaemonClient::sendJsDebuggerInfo(uint16_t port, const std::vector<StringBox>& websocketTargets) {
    auto targets = ValueArray::make(websocketTargets.size());
    for (size_t i = 0; i < websocketTargets.size(); i++) {
        targets->emplace(i, Value(websocketTargets[i]));
    }
    Value jsDebugInfo = Value().setMapValue("port", Value(port)).setMapValue("websocket_targets", Value(targets));
    Value json = Value().setMapValue("event", Value().setMapValue("js_debugger_info", jsDebugInfo));
    submitPayload(json);
}

DaemonClientPendingResponse::DaemonClientPendingResponse(Function<void(const Value&)> handler)
    : handler(std::move(handler)) {}

} // namespace Valdi
