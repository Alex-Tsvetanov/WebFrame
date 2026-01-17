//
//  HermesDebuggerServer.hpp
//  valdi-hermes
//
//  Created by Simon Corsin on 1/8/24.
//

#pragma once

#include "valdi/hermes/HermesDebuggerConnection.hpp"
#include "valdi/hermes/HermesDebuggerRegistry.hpp"
#include "valdi/hermes/WebSocketServer/IWebSocketServer.hpp"
#include "valdi_core/cpp/Utils/Mutex.hpp"
#include "valdi_core/cpp/Utils/Shared.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace Valdi::Hermes {

/**
 The HermesDebuggerServer hosts an TCP server implementing the Chrome Debugger Protocol..
 It hosts a list of live Hermes VM runtime and forwards the commands to the right VM.
 In order for the debugger to work under the Chrome or VSCode Debugger, there needs to be
 a WebSocket proxy server setup on the host machine which will connect to this debugger. Chrome and VSCode
 connects to the WebSocket proxy server, which takes care of forwarding the requests to this server.
 */
class HermesDebuggerServer : public IWebSocketServerListener {
public:
    explicit HermesDebuggerServer(ILogger& logger);
    ~HermesDebuggerServer() override;

    void start();

    void stop();

    uint16_t getPort();

    std::vector<StringBox> getWebsocketTargets();

    void onClientConnected(const Ref<IWebSocketConnection>& client) override;

    void onClientDisconnected(const Ref<IWebSocketConnection>& client, const Error& error) override;

    void onHttp(const Ref<IWebSocketServerHttpResponse>& response) override;

    Ref<HermesDebuggerConnection> getDebuggerConnection(HermesRuntimeId runtimeId);

    Ref<HermesDebuggerRegistry> getRegistry();

    static HermesDebuggerServer* getInstance(ILogger& logger);

private:
    std::string getJsonList();

    Ref<ILogger> _logger;
    Ref<IWebSocketServer> _webSocketServer;
    Ref<HermesDebuggerRegistry> _registry;
    Mutex _mutex;
};

} // namespace Valdi::Hermes
