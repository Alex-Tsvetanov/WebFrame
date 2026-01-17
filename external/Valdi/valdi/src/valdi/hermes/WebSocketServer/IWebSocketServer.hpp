//
//  IWebSocketServer.hpp
//  valdi
//
//  Created by Edward Lee on 02/20/2024.
//

#pragma once

#include "valdi/hermes/WebSocketServer/IWebSocketConnection.hpp"
#include "valdi_core/cpp/Utils/Result.hpp"
#include "valdi_core/cpp/Utils/Shared.hpp"

namespace Valdi {

class IWebSocketServerHttpResponse : public SharedPtrRefCountable {
public:
    enum Status {
        OK = 200,
        NOT_FOUND = 404,
    };
    virtual ~IWebSocketServerHttpResponse() = default;
    virtual const std::string& getResource() const = 0;
    virtual void setBody(const std::string& body) = 0;
    virtual void setHeader(const std::string& key, const std::string& value) = 0;
    virtual void setStatus(Status status) = 0;
};

class IWebSocketServerListener {
public:
    virtual ~IWebSocketServerListener() = default;

    virtual void onHttp(const Ref<IWebSocketServerHttpResponse>& client) = 0;
    virtual void onClientConnected(const Ref<IWebSocketConnection>& client) = 0;
    virtual void onClientDisconnected(const Ref<IWebSocketConnection>& client, const Error& error) = 0;
};

class IWebSocketServer : public SharedPtrRefCountable {
public:
    [[nodiscard]] virtual Result<Void> start() = 0;

    virtual void stop() = 0;

    virtual uint16_t getBoundPort() = 0;
    virtual std::vector<std::string> getAvailableAddresses() = 0;

    virtual bool ownsClient(const Ref<IWebSocketConnection>& client) const = 0;

    virtual std::vector<Ref<IWebSocketConnection>> getConnectedClients() = 0;
};

} // namespace Valdi
