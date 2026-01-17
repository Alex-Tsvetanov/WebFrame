//
//  IWebSocketConnection.hpp
//  valdi
//
//  Created by Edward Lee on 02/20/2024.
//

#pragma once

#include "valdi_core/cpp/Utils/Bytes.hpp"
#include "valdi_core/cpp/Utils/Error.hpp"
#include "valdi_core/cpp/Utils/Shared.hpp"
#include "valdi_core/cpp/Utils/StringBox.hpp"

namespace Valdi {

class IWebSocketConnection;

class IWebSocketConnectionDataListener {
public:
    virtual ~IWebSocketConnectionDataListener() = default;

    virtual void onDataReceived(const Ref<IWebSocketConnection>& connection, const BytesView& bytes) = 0;
};

class IWebSocketConnectionDisconnectListener {
public:
    virtual ~IWebSocketConnectionDisconnectListener() = default;

    virtual void onDisconnected(const Ref<IWebSocketConnection>& connection, const Error& error) = 0;
};

class IWebSocketConnection : public SharedPtrRefCountable {
public:
    virtual void submitData(const BytesView& bytes) = 0;

    virtual void setDataListener(Shared<IWebSocketConnectionDataListener> listener) = 0;
    virtual Shared<IWebSocketConnectionDataListener> getDataListener() const = 0;

    virtual void setDisconnectListener(Shared<IWebSocketConnectionDisconnectListener> listener) = 0;
    virtual Shared<IWebSocketConnectionDisconnectListener> getDisconnectListener() const = 0;

    virtual void close(const Error& error) = 0;

    virtual const std::string& getResourceAddress() const = 0;
};

} // namespace Valdi
