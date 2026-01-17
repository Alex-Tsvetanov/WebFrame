//
//  HermesDebuggerServer.cpp
//  valdi-hermes
//
//  Created by Simon Corsin on 1/8/24.
//

#include "valdi/hermes/HermesDebuggerServer.hpp"
#include "valdi/hermes/WebSocketServer/WebSocketServer.hpp"
#include "valdi_core/cpp/Utils/LoggerUtils.hpp"
#include "valdi_core/cpp/Utils/ValueArrayBuilder.hpp"
#include "valdi_core/cpp/Utils/ValueUtils.hpp"

namespace Valdi::Hermes {

class HermesDebuggerWebSocketConnection : public IWebSocketConnectionDataListener {
public:
    HermesDebuggerWebSocketConnection(IWebSocketConnection& websocketConnection,
                                      const Ref<HermesDebuggerConnection>& debuggerConnection)
        : _websocketConnection(websocketConnection), _debuggerConnection(debuggerConnection) {
        _debuggerConnection->setCDPMessageListener([this](const Value& message) { onMessageToCDP(message); });
    }
    ~HermesDebuggerWebSocketConnection() override {
        _debuggerConnection->setCDPMessageListener(nullptr);
    };

    void onDataReceived(const Ref<IWebSocketConnection>& connection, const BytesView& bytes) final {
        auto message = jsonToValue(bytes.data(), bytes.size());
        if (!message.success()) {
            return;
        }
        _debuggerConnection->handleCDPMessage(message.value());
    }

    void onMessageToCDP(const Value& message) {
        _websocketConnection.submitData(valueToJson(message)->toBytesView());
    };

private:
    IWebSocketConnection& _websocketConnection;
    Ref<HermesDebuggerConnection> _debuggerConnection;
};

HermesDebuggerServer::HermesDebuggerServer(ILogger& logger)
    : _logger(&logger), _registry(makeShared<HermesDebuggerRegistry>()) {}

HermesDebuggerServer::~HermesDebuggerServer() = default;

void HermesDebuggerServer::start() {
    std::lock_guard<Mutex> lock(_mutex);
    if (_webSocketServer != nullptr) {
        return;
    }
    auto server = WebSocketServer::create(0, this);
    if (server == nullptr) {
        VALDI_ERROR(*_logger, "[HermesDebugger] Failed to create JS debugger server");
        return;
    }

    auto result = server->start();
    if (!result) {
        VALDI_ERROR(*_logger, "[HermesDebugger] Failed to start JS debugger {}", result.error());
        return;
    }
    VALDI_INFO(*_logger, "[HermesDebugger] Started JS debugger on {}", server->getBoundPort());
    _webSocketServer = server;
}

void HermesDebuggerServer::stop() {
    std::lock_guard<Mutex> lock(_mutex);
    if (_webSocketServer != nullptr) {
        _webSocketServer->stop();
        _webSocketServer = nullptr;
        VALDI_INFO(*_logger, "Stopped Hermes JS Valdi debugger service");
    }
}

uint16_t HermesDebuggerServer::getPort() {
    std::lock_guard<Mutex> lock(_mutex);
    if (_webSocketServer == nullptr) {
        return 0;
    }
    return _webSocketServer->getBoundPort();
}

std::vector<StringBox> HermesDebuggerServer::getWebsocketTargets() {
    return _registry->getRuntimeIds();
}

std::string HermesDebuggerServer::getJsonList() {
    auto arrayBuilder = ValueArrayBuilder();
    for (auto runtimeId : _registry->getRuntimeIds()) {
        auto webSocketUrl = "ws://localhost:" + std::to_string(getPort()) + "/" + std::string(runtimeId.toStringView());
        auto device = Valdi::Value()
                          .setMapValue("description", Value("Valdi Hermes"))
                          .setMapValue("faviconUrl", Value("https://nodejs.org/static/images/favicons/favicon.ico"))
                          .setMapValue("id", Value(runtimeId))
                          .setMapValue("title", Value("Valdi Hermes"))
                          .setMapValue("type", Value("node"))
                          .setMapValue("webSocketDebuggerUrl", Value(webSocketUrl));
        arrayBuilder.append(device);
    }
    return Valdi::valueToJsonString(Value(arrayBuilder.build()));
}

void HermesDebuggerServer::onHttp(const Ref<IWebSocketServerHttpResponse>& response) {
    auto resource = response->getResource();
    if (resource == "/json/list" || resource == "/json") {
        std::string response_body = getJsonList();
        response->setBody(response_body);
        response->setStatus(IWebSocketServerHttpResponse::Status::OK);
        response->setHeader("Content-Type", "application/json");
    } else {
        response->setStatus(IWebSocketServerHttpResponse::Status::NOT_FOUND);
    }
}

void HermesDebuggerServer::onClientConnected(const Ref<IWebSocketConnection>& client) {
    auto resource = client->getResourceAddress();
    auto runtimeIdFromResource = resource.substr(resource.find_last_of("/") + 1);
    const HermesRuntimeId requestedRuntimeId = StringCache::getGlobal().makeString(runtimeIdFromResource);
    auto runtimeIds = _registry->getRuntimeIds();

    auto connectedRuntimeId = kRuntimeIdUndefined;
    for (auto runtimeId : runtimeIds) {
        if (runtimeId == requestedRuntimeId) {
            connectedRuntimeId = requestedRuntimeId;
            break;
        }
    }

    // If can't find the requested runtime, connect to the first available runtime.
    if (connectedRuntimeId == kRuntimeIdUndefined && runtimeIds.size() > 0) {
        connectedRuntimeId = runtimeIds[0];
    }
    client->setDataListener(
        makeShared<HermesDebuggerWebSocketConnection>(*client, getDebuggerConnection(connectedRuntimeId)));
    VALDI_INFO(*_logger, "[HermesDebuggerServer] New debugger connection to {}", client->getResourceAddress());
}

void HermesDebuggerServer::onClientDisconnected(const Ref<IWebSocketConnection>& client, const Error& error) {
    auto debuggerConnection = castOrNull<HermesDebuggerWebSocketConnection>(client->getDataListener());
    if (debuggerConnection != nullptr) {
        VALDI_INFO(*_logger, "[HermesDebuggerServer] Disconnected from {}: {}", client->getResourceAddress(), error);
    }
}

Ref<HermesDebuggerConnection> HermesDebuggerServer::getDebuggerConnection(HermesRuntimeId runtimeId) {
    return makeShared<HermesDebuggerConnection>(_registry, runtimeId, *_logger);
}

Ref<HermesDebuggerRegistry> HermesDebuggerServer::getRegistry() {
    return _registry;
}

HermesDebuggerServer* HermesDebuggerServer::getInstance(ILogger& logger) {
#if defined(HERMES_ENABLE_DEBUGGER) && defined(HERMES_API)
    static auto* kInstance = new HermesDebuggerServer(logger);
    return kInstance;
#else
    return nullptr;
#endif
}

} // namespace Valdi::Hermes
