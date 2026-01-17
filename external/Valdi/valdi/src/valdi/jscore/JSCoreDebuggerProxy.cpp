//
//  JSCoreDebuggerProxy.cpp
//  valdi-ios
//
//  Created by Simon Corsin on 6/17/20.
//

#include "JSCoreDebuggerProxy.hpp"

#include "valdi/runtime/Debugger/TCPServer.hpp"
#include "valdi_core/cpp/Utils/ByteBuffer.hpp"
#include "valdi_core/cpp/Utils/LoggerUtils.hpp"
#include "valdi_core/cpp/Utils/StringCache.hpp"
#include "valdi_core/cpp/Utils/ValueUtils.hpp"

#include "utils/platform/BuildOptions.hpp"
#include <arpa/inet.h>

namespace ValdiJSCore {

constexpr uint32_t kJsDebuggerPort = 13593;
constexpr uint32_t kDebuggerProxyPort = 13594;
constexpr bool kDebugRequests = false;

#if TARGET_OS_IPHONE

DebuggerProxy* DebuggerProxy::start(Valdi::ILogger& logger) {
    return nullptr;
}

#else

DebuggerProxy* DebuggerProxy::start(Valdi::ILogger& logger) {
    if constexpr (snap::kIsDevBuild) {
        static auto* kProxy = new DebuggerProxy(logger);
        return kProxy;
    }

    return nullptr;
}

#endif

DebuggerConnection::DebuggerConnection(Valdi::ITCPConnection& client, Valdi::ILogger& logger)
    : _client(client), _logger(logger) {}

void DebuggerConnection::onDataReceived(const Valdi::Ref<Valdi::ITCPConnection>& connection,
                                        const Valdi::BytesView& bytes) {
    if constexpr (kDebugRequests) {
        VALDI_INFO(_logger, "[JSDebugger] Received debugger data of size {}", bytes.size());
    }

    std::lock_guard<Valdi::Mutex> lock(_mutex);
    _buffer.insert(_buffer.end(), bytes.begin(), bytes.end());

    while (consumeBuffer()) {
    }
}

void DebuggerConnection::submitMessage(const Valdi::Value& value) {
    auto json = Valdi::valueToJson(value);
    submitJson(std::string_view(reinterpret_cast<const char*>(json->data()), json->size()));
}

void DebuggerConnection::submitJson(const std::string_view& json) {
    if constexpr (kDebugRequests) {
        VALDI_INFO(_logger, "[JSDebugger] Submitting JSON: {}", json);
    }

    auto packet = Valdi::makeShared<Valdi::ByteBuffer>();

    auto size = static_cast<uint32_t>(json.size());
    uint32_t nboSize = htonl(size);

    packet->reserve(sizeof(nboSize) + json.size());

    packet->append(reinterpret_cast<const Valdi::Byte*>(&nboSize),
                   reinterpret_cast<const Valdi::Byte*>(&nboSize) + sizeof(nboSize));
    packet->append(json.data(), json.data() + json.size());

    _client.submitData(packet->toBytesView());
}

bool DebuggerConnection::consumeBuffer() {
    if (_buffer.size() < sizeof(uint32_t)) {
        return false;
    }

    uint32_t size;
    std::memcpy(&size, _buffer.data(), sizeof(uint32_t));
    size = ntohl(size);

    if (_buffer.size() - sizeof(uint32_t) < size) {
        return false;
    }

    auto rawMessage = Valdi::StringCache::getGlobal().makeString(
        reinterpret_cast<const char*>(_buffer.data() + sizeof(uint32_t)), static_cast<size_t>(size));

    _buffer.erase(_buffer.begin(), _buffer.begin() + sizeof(uint32_t) + static_cast<size_t>(size));

    processMessage(rawMessage);

    return true;
}

Valdi::ILogger& DebuggerConnection::getLogger() const {
    return _logger;
}

JSDebuggerConnection::JSDebuggerConnection(Valdi::ITCPServer& externHost,
                                           Valdi::ITCPConnection& client,
                                           Valdi::ILogger& logger)
    : DebuggerConnection(client, logger), _externHost(externHost) {}

JSDebuggerConnection::~JSDebuggerConnection() = default;

void JSDebuggerConnection::processMessage(const Valdi::StringBox& message) {
    if constexpr (kDebugRequests) {
        VALDI_INFO(getLogger(), "[JSDebugger] Received JS message: {}", message);
    }

    auto result = Valdi::jsonToValue(message.toStringView());
    if (!result) {
        VALDI_ERROR(getLogger(), "[JSDebugger] Failed to parse JS message: {}", result.error());
        return;
    }
    auto jsMessage = result.moveValue();
    auto event = jsMessage.getMapValue("event").toStringBox();
    auto innerMessage = jsMessage.getMapValue("message").toStringBox();

    static auto kSetTargetList = STRING_LITERAL("SetTargetList");
    static auto kSendMessageToFrontend = STRING_LITERAL("SendMessageToFrontend");

    if (event == kSetTargetList) {
        setTargetList(innerMessage);
    } else if (event == kSendMessageToFrontend) {
        sendMessageToFrontend(innerMessage);
    }
}

void JSDebuggerConnection::sendMessageToFrontend(const Valdi::StringBox& message) {
    for (const auto& client : _externHost.getConnectedClients()) {
        auto debuggerConnection = std::dynamic_pointer_cast<DebuggerConnection>(client->getDataListener());
        if (debuggerConnection != nullptr) {
            debuggerConnection->submitJson(message.toStringView());
        }
    }
}

void JSDebuggerConnection::setTargetList(const Valdi::StringBox& message) {
    auto convertedValue = Valdi::jsonToValue(message.toStringView());

    if (!convertedValue) {
        VALDI_ERROR(getLogger(), "[JSDebugger] Failed to parse message: {}", convertedValue.error());
        return;
    }

    auto* items = convertedValue.value().getArray();
    if (items == nullptr || items->empty()) {
        _targetId = 0;
        return;
    }

    // TODO(simon): This is hardcoded to expose just a single target.
    // If we wanted to support multiple targets, we would have to make
    // the external proxy protocol aware of target ids.
    _targetId = (*items)[0].getMapValue("targetID").toInt();
}

int JSDebuggerConnection::getTargetId() const {
    return _targetId;
}

ExternalDebuggerConnection::ExternalDebuggerConnection(Valdi::ITCPServer& jsHost,
                                                       Valdi::ITCPConnection& client,
                                                       Valdi::ILogger& logger)
    : DebuggerConnection(client, logger), _jsHost(jsHost) {}

ExternalDebuggerConnection::~ExternalDebuggerConnection() = default;

void ExternalDebuggerConnection::processMessage(const Valdi::StringBox& message) {
    if constexpr (kDebugRequests) {
        VALDI_INFO(getLogger(), "[JSDebugger] Received External message: {}", message);
    }

    static auto kSendMessageToTarget = STRING_LITERAL("SendMessageToTarget");

    submitMessageToJs(kSendMessageToTarget, message);
}

void ExternalDebuggerConnection::submitMessageToJs(const Valdi::StringBox& event, const Valdi::StringBox& message) {
    for (const auto& client : _jsHost.getConnectedClients()) {
        auto jsConnection = std::dynamic_pointer_cast<JSDebuggerConnection>(client->getDataListener());
        if (jsConnection != nullptr) {
            auto targetId = jsConnection->getTargetId();

            Valdi::Value outMessage;
            outMessage.setMapValue("event", Valdi::Value(event));
            outMessage.setMapValue("message", Valdi::Value(message));
            if (targetId != 0) {
                outMessage.setMapValue("targetID", Valdi::Value(targetId));
            }

            jsConnection->submitMessage(outMessage);
        }
    }
}

DebuggerProxy::DebuggerProxy(Valdi::ILogger& logger) : _logger(logger) {
    _jsHost = Valdi::TCPServer::create(kJsDebuggerPort, this);

    auto result = _jsHost->start();
    if (result) {
        VALDI_INFO(_logger, "[JSDebugger] Started JS debugger host on {}", kJsDebuggerPort);
    } else {
        VALDI_ERROR(_logger, "[JSDebugger] Failed to start JS debugger host: {}", result.error());
    }

    _externHost = Valdi::TCPServer::create(kDebuggerProxyPort, this);
    result = _externHost->start();
    if (result) {
        VALDI_INFO(_logger, "[JSDebugger] Started Debugger proxy on {}", kDebuggerProxyPort);
    } else {
        VALDI_ERROR(_logger, "[JSDebugger] Failed to start Debugger proxy: {}", result.error());
    }
}

void DebuggerProxy::onClientConnected(const Valdi::Ref<Valdi::ITCPConnection>& client) {
    if (_jsHost->ownsClient(client)) {
        VALDI_INFO(_logger, "[JSDebugger] JSCore client connected from {}", client->getAddress());
        client->setDataListener(Valdi::makeShared<JSDebuggerConnection>(*_externHost, *client, _logger).toShared());
    } else {
        VALDI_INFO(_logger, "[JSDebugger] External client connected from {}", client->getAddress());
        auto connection = Valdi::makeShared<ExternalDebuggerConnection>(*_jsHost, *client, _logger);
        client->setDataListener(connection.toShared());

        connection->submitMessageToJs(STRING_LITERAL("Setup"), STRING_LITERAL("{}"));
    }
}

void DebuggerProxy::onClientDisconnected(const Valdi::Ref<Valdi::ITCPConnection>& client, const Valdi::Error& error) {
    if (_jsHost->ownsClient(client)) {
        VALDI_INFO(_logger, "[JSDebugger] JSCore client disconnecte from {}: {}", client->getAddress(), error);
    } else {
        VALDI_INFO(_logger, "[JSDebugger] External client disconnected from {}: {}", client->getAddress(), error);

        auto connection = std::dynamic_pointer_cast<ExternalDebuggerConnection>(client->getDataListener());
        connection->submitMessageToJs(STRING_LITERAL("FrontendDidClose"), STRING_LITERAL("{}"));
    }
}

uint16_t DebuggerProxy::getInternalPort() const {
    return static_cast<uint16_t>(kJsDebuggerPort);
}

uint16_t DebuggerProxy::getExternalPort() const {
    return static_cast<uint16_t>(kDebuggerProxyPort);
}

} // namespace ValdiJSCore
