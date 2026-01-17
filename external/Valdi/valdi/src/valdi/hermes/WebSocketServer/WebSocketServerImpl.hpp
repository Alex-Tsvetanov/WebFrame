//
//  WebSocketServerImpl.hpp
//  valdi
//
//  Created by Edward Lee on 02/20/2024.
//

#pragma once

#ifdef VALDI_HAS_WEBSOCKET_SERVER

#include "valdi/hermes/WebSocketServer/IWebSocketServer.hpp"
#include "valdi/hermes/WebSocketServer/WebSocketServerConnectionImpl.hpp"
#include "valdi_core/cpp/Threading/Thread.hpp"
#include "valdi_core/cpp/Utils/FlatMap.hpp"

namespace Valdi {

class ILogger;
class WebSocketServerImpl : public IWebSocketServer {
public:
    WebSocketServerImpl(uint16_t port, IWebSocketServerListener* listener);

    Result<Void> start() final;
    void stop() final;
    uint16_t getBoundPort() final;
    std::vector<std::string> getAvailableAddresses() final;
    bool ownsClient(const Ref<IWebSocketConnection>& client) const final;
    std::vector<Ref<IWebSocketConnection>> getConnectedClients() final;

    void onDisconnected(const Ref<WebSocketServerConnectionImpl>& connection, const Error& error);

private:
    void onOpen(websocketpp::connection_hdl hdl);
    void onClose(websocketpp::connection_hdl hdl);
    void onMessage(websocketpp::connection_hdl hdl, WebSocketPPServer::message_ptr msg);
    void onHttp(websocketpp::connection_hdl hdl);
    void runIOService();

    bool removeConnection(const Ref<WebSocketServerConnectionImpl>& connection);
    void appendConnection(const Ref<WebSocketServerConnectionImpl>& connection, websocketpp::uri_ptr uri);
    void closeAllConnections(const Error& error);
    Ref<WebSocketServerConnectionImpl> connectionFromHandle(websocketpp::connection_hdl hdl);

    Mutex _mutex;
    uint16_t _port;
    Ref<Thread> _asioThread;
    WebSocketPPServer _server;
    IWebSocketServerListener* _listener;
    FlatMap<websocketpp::uri_ptr, Ref<WebSocketServerConnectionImpl>> _connections;
};

} // namespace Valdi

#endif // VALDI_HAS_WEBSOCKET_SERVER
