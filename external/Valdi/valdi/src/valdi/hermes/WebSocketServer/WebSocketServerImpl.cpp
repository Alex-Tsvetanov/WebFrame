//
//  WebSocketServerImpl.cpp
//  valdi
//
//  Created by Edward Lee on 02/20/2024.
//
#ifdef VALDI_HAS_WEBSOCKET_SERVER

#include "valdi/hermes/WebSocketServer/WebSocketServerImpl.hpp"
#include "valdi_core/cpp/Utils/LoggerUtils.hpp"
#include "valdi_core/cpp/Utils/StringCache.hpp"

using websocketpp::lib::bind;
using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;

namespace Valdi {

class WebSocketServerDisconnectHandler : public SharedPtrRefCountable, public IWebSocketConnectionDisconnectListener {
public:
    explicit WebSocketServerDisconnectHandler(WebSocketServerImpl* server) : _weakServer(weakRef(server)) {}
    ~WebSocketServerDisconnectHandler() override = default;

    void onDisconnected(const Ref<IWebSocketConnection>& connection, const Error& error) override {
        auto server = getServer();
        if (server != nullptr) {
            server->onDisconnected(castOrNull<WebSocketServerConnectionImpl>(connection), error);
        }
    }

    Shared<WebSocketServerImpl> getServer() const {
        return _weakServer.lock();
    }

private:
    Weak<WebSocketServerImpl> _weakServer;
};

class WebSocketServerHttpResponseImpl : public IWebSocketServerHttpResponse {
public:
    using WebSocketPPServerConnection = websocketpp::endpoint<websocketpp::connection<websocketpp::config::asio>,
                                                              websocketpp::config::asio>::connection_ptr;
    WebSocketServerHttpResponseImpl(WebSocketPPServerConnection connection) : _connection(connection) {}
    ~WebSocketServerHttpResponseImpl() = default;

    const std::string& getResource() const final {
        return _connection->get_resource();
    }
    void setBody(const std::string& body) final {
        _connection->set_body(body);
    }
    void setHeader(const std::string& key, const std::string& value) final {
        _connection->replace_header(key, value);
    }
    void setStatus(Status status) {
        _connection->set_status(static_cast<websocketpp::http::status_code::value>(status));
    }

private:
    WebSocketPPServerConnection _connection;
};

WebSocketServerImpl::WebSocketServerImpl(uint16_t port, IWebSocketServerListener* listener)
    : _port(port), _listener(listener) {
    _server.clear_access_channels(websocketpp::log::alevel::all);
    _server.set_message_handler(bind(&WebSocketServerImpl::onMessage, this, ::_1, ::_2));
    _server.set_open_handler(bind(&WebSocketServerImpl::onOpen, this, ::_1));
    _server.set_close_handler(bind(&WebSocketServerImpl::onClose, this, ::_1));
    _server.set_http_handler(bind(&WebSocketServerImpl::onHttp, this, ::_1));
}

Result<Void> WebSocketServerImpl::start() {
    std::unique_lock<Mutex> guard(_mutex);
    websocketpp::lib::error_code ec;
    if (_asioThread != nullptr) {
        return Void();
    }
    _server.init_asio(ec);
    if (ec) {
        return Error(ec.message().c_str());
    }

    _server.listen(_port, ec);
    if (ec) {
        return Error(ec.message().c_str());
    }

    _server.start_accept(ec);
    if (ec) {
        return Error(ec.message().c_str());
    }

    _asioThread = Thread::create(STRING_LITERAL("Valdi Websocket Server"), ThreadQoSClassNormal, [this]() {
                      runIOService();
                  }).moveValue();
    return Void();
}

void WebSocketServerImpl::stop() {
    websocketpp::lib::error_code ec;
    closeAllConnections(Error("Server stopped"));

    _server.stop_listening(ec);
    _server.stop();
    std::unique_lock<Mutex> guard(_mutex);
    auto thread = _asioThread;
    _asioThread = nullptr;
    if (thread != nullptr) {
        guard.unlock();
        thread->join();
    }
}

void WebSocketServerImpl::closeAllConnections(const Error& error) {
    while (true) {
        Ref<WebSocketServerConnectionImpl> connection;

        {
            std::lock_guard<Mutex> guard(_mutex);
            if (_connections.empty()) {
                return;
            }
            connection = _connections.begin()->second;
        }

        connection->close(error);
    }
}

void WebSocketServerImpl::runIOService() {
    try {
        _server.run();
    } catch (const websocketpp::exception& e) {
    }
}

uint16_t WebSocketServerImpl::getBoundPort() {
    websocketpp::lib::asio::error_code ec;
    auto endpoint = _server.get_local_endpoint(ec);
    if (ec) {
        return 0;
    }
    return endpoint.port();
}

std::vector<std::string> WebSocketServerImpl::getAvailableAddresses() {
    return {};
}

void WebSocketServerImpl::onOpen(websocketpp::connection_hdl hdl) {
    websocketpp::lib::error_code ec;
    auto connPtr = _server.get_con_from_hdl(hdl, ec);
    if (ec || connPtr == nullptr) {
        return;
    }

    auto client = makeShared<WebSocketServerConnectionImpl>(&_server, hdl);
    client->setDisconnectListener(makeShared<WebSocketServerDisconnectHandler>(this).toShared());
    appendConnection(client, connPtr->get_uri());
    _listener->onClientConnected(client);
}

void WebSocketServerImpl::onClose(websocketpp::connection_hdl hdl) {
    auto connection = connectionFromHandle(hdl);
    if (connection != nullptr) {
        onDisconnected(connection, Error("Connection closed"));
    }
}

void WebSocketServerImpl::onMessage(websocketpp::connection_hdl hdl, WebSocketPPServer::message_ptr msg) {
    auto connection = connectionFromHandle(hdl);
    if (connection != nullptr) {
        auto bytes = makeShared<Bytes>();
        bytes->assignData(reinterpret_cast<const Byte*>(msg->get_payload().data()), msg->get_payload().size());
        connection->dataReceived(BytesView(bytes));
    }
}

void WebSocketServerImpl::onHttp(websocketpp::connection_hdl hdl) {
    auto con = _server.get_con_from_hdl(hdl);
    auto httpResp = makeShared<WebSocketServerHttpResponseImpl>(con);
    _listener->onHttp(httpResp);
}

Ref<WebSocketServerConnectionImpl> WebSocketServerImpl::connectionFromHandle(websocketpp::connection_hdl hdl) {
    websocketpp::lib::error_code ec;
    std::lock_guard<Mutex> guard(_mutex);
    auto connPtr = _server.get_con_from_hdl(hdl, ec);
    if (ec || connPtr == nullptr) {
        return nullptr;
    }

    const auto& resource = connPtr->get_uri()->get_resource();
    for (const auto& connection : _connections) {
        if (connection.second->getResourceAddress() == resource) {
            return connection.second;
        }
    }
    return nullptr;
}

void WebSocketServerImpl::appendConnection(const Ref<WebSocketServerConnectionImpl>& connection,
                                           websocketpp::uri_ptr uri) {
    std::lock_guard<Mutex> guard(_mutex);
    _connections[uri] = connection;
}

bool WebSocketServerImpl::removeConnection(const Ref<WebSocketServerConnectionImpl>& connection) {
    std::lock_guard<Mutex> guard(_mutex);

    auto it = _connections.begin();
    while (it != _connections.end()) {
        if (it->second == connection) {
            _connections.erase(it);
            return true;
        } else {
            ++it;
        }
    }

    return false;
}

void WebSocketServerImpl::onDisconnected(const Ref<WebSocketServerConnectionImpl>& connection, const Error& error) {
    if (removeConnection(connection)) {
        _listener->onClientDisconnected(connection, error);
    }
}

std::vector<Ref<IWebSocketConnection>> WebSocketServerImpl::getConnectedClients() {
    std::lock_guard<Mutex> guard(_mutex);

    std::vector<Ref<IWebSocketConnection>> allConnections;
    allConnections.reserve(_connections.size());

    for (const auto& connection : _connections) {
        allConnections.emplace_back(connection.second);
    }

    return allConnections;
}

bool WebSocketServerImpl::ownsClient(const Ref<IWebSocketConnection>& client) const {
    auto disconnectHandler = castOrNull<WebSocketServerDisconnectHandler>(client->getDisconnectListener());
    if (disconnectHandler == nullptr) {
        return false;
    }
    return disconnectHandler->getServer().get() == this;
}

} // namespace Valdi

#endif // VALDI_HAS_WEBSOCKET_SERVER
