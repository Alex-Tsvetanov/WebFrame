//
//  WebSocketServerConnectionImpl.cpp
//  valdi
//
//  Created by Edward Lee on 02/20/2024.
//

#ifdef VALDI_HAS_WEBSOCKET_SERVER

#include "valdi/hermes/WebSocketServer/WebSocketServerConnectionImpl.hpp"

namespace Valdi {

WebSocketServerConnectionImpl::WebSocketServerConnectionImpl(WebSocketPPServer* server, websocketpp::connection_hdl hdl)
    : _server(server), _connection(hdl), _resource(server->get_con_from_hdl(hdl)->get_uri()->get_resource()) {}

WebSocketServerConnectionImpl::~WebSocketServerConnectionImpl() = default;

void WebSocketServerConnectionImpl::setDataListener(Shared<IWebSocketConnectionDataListener> listener) {
    std::lock_guard<Mutex> guard(_mutex);
    _dataListener = std::move(listener);
}

Shared<IWebSocketConnectionDataListener> WebSocketServerConnectionImpl::getDataListener() const {
    std::lock_guard<Mutex> guard(_mutex);
    return _dataListener;
}

void WebSocketServerConnectionImpl::setDisconnectListener(Shared<IWebSocketConnectionDisconnectListener> listener) {
    std::lock_guard<Mutex> guard(_mutex);
    _disconnectListener = std::move(listener);
}

Shared<IWebSocketConnectionDisconnectListener> WebSocketServerConnectionImpl::getDisconnectListener() const {
    std::lock_guard<Mutex> guard(_mutex);
    return _disconnectListener;
}

const std::string& WebSocketServerConnectionImpl::getResourceAddress() const {
    return _resource;
}

void WebSocketServerConnectionImpl::close(const Error& error) {
    websocketpp::lib::error_code ec;
    _server->close(_connection, websocketpp::close::status::normal, error.toString(), ec);
    auto listener = getDisconnectListener();
    if (listener != nullptr) {
        listener->onDisconnected(strongSmallRef(this), error);
    }
}

void WebSocketServerConnectionImpl::dataReceived(const BytesView& bytes) {
    auto listener = getDataListener();
    if (listener != nullptr) {
        listener->onDataReceived(strongSmallRef(this), bytes);
    }
}

websocketpp::connection_hdl WebSocketServerConnectionImpl::getConnectionHandle() const {
    return _connection;
}

void WebSocketServerConnectionImpl::submitData(const BytesView& bytes) {
    websocketpp::lib::error_code ec;
    _server->send(_connection, bytes.data(), bytes.size(), websocketpp::frame::opcode::text, ec);
}

} // namespace Valdi

#endif // VALDI_HAS_WEBSOCKET_SERVER
