//
//  WebSocketServerConnectionImpl.hpp
//  valdi
//
//  Created by Edward Lee on 02/20/2024.
//

#pragma once

#ifdef VALDI_HAS_WEBSOCKET_SERVER

#include "valdi/hermes/WebSocketServer/IWebSocketConnection.hpp"
#include "valdi/hermes/WebSocketServer/WebSocketPP.hpp"
#include "valdi_core/cpp/Utils/Bytes.hpp"
#include "valdi_core/cpp/Utils/Mutex.hpp"
#include <array>
#include <deque>

namespace Valdi {

class WebSocketServerConnectionImpl : public IWebSocketConnection {
public:
    explicit WebSocketServerConnectionImpl(WebSocketPPServer* server, websocketpp::connection_hdl hdl);
    ~WebSocketServerConnectionImpl() override;

    void submitData(const BytesView& bytes) final;

    void setDataListener(Shared<IWebSocketConnectionDataListener> listener) final;
    Shared<IWebSocketConnectionDataListener> getDataListener() const final;

    void setDisconnectListener(Shared<IWebSocketConnectionDisconnectListener> listener) final;
    Shared<IWebSocketConnectionDisconnectListener> getDisconnectListener() const final;

    void close(const Error& error) final;

    const std::string& getResourceAddress() const final;

    void dataReceived(const BytesView& bytes);

    websocketpp::connection_hdl getConnectionHandle() const;

private:
    WebSocketPPServer* _server;
    websocketpp::connection_hdl _connection;
    std::string _resource;
    mutable Mutex _mutex;
    Shared<IWebSocketConnectionDataListener> _dataListener = nullptr;
    Shared<IWebSocketConnectionDisconnectListener> _disconnectListener = nullptr;
};

} // namespace Valdi

#endif // VALDI_HAS_WEBSOCKET_SERVER
