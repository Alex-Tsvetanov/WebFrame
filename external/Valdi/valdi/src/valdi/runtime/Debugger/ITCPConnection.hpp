//
//  ITCPConnection.hpp
//  valdi-pc
//
//  Created by Simon Corsin on 10/14/20.
//

#pragma once

#include "valdi_core/cpp/Utils/Bytes.hpp"
#include "valdi_core/cpp/Utils/Error.hpp"
#include "valdi_core/cpp/Utils/Shared.hpp"
#include "valdi_core/cpp/Utils/StringBox.hpp"

namespace Valdi {

class ITCPConnection;

class ITCPConnectionDataListener {
public:
    virtual ~ITCPConnectionDataListener() = default;

    virtual void onDataReceived(const Ref<ITCPConnection>& connection, const BytesView& bytes) = 0;
};

class ITCPConnectionDisconnectListener {
public:
    virtual ~ITCPConnectionDisconnectListener() = default;

    virtual void onDisconnected(const Ref<ITCPConnection>& connection, const Error& error) = 0;
};

class ITCPConnection : public SharedPtrRefCountable {
public:
    virtual void submitData(const BytesView& bytes) = 0;

    virtual void setDataListener(Shared<ITCPConnectionDataListener> listener) = 0;
    virtual Shared<ITCPConnectionDataListener> getDataListener() const = 0;

    virtual void setDisconnectListener(Shared<ITCPConnectionDisconnectListener> listener) = 0;
    virtual Shared<ITCPConnectionDisconnectListener> getDisconnectListener() const = 0;

    virtual void close(const Error& error) = 0;

    virtual const std::string& getAddress() const = 0;
};

} // namespace Valdi
