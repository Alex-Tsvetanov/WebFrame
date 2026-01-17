//
//  TCPClient.hpp
//  valdi-pc
//
//  Created by Simon Corsin on 10/14/20.
//

#pragma once

#include "valdi/runtime/Debugger/ITCPConnection.hpp"
#include "valdi_core/cpp/Utils/Function.hpp"
#include "valdi_core/cpp/Utils/Result.hpp"
#include "valdi_core/cpp/Utils/Shared.hpp"

namespace Valdi {

class TCPClientImpl;

class ITCPClientListener {
public:
    virtual ~ITCPClientListener() = default;

    virtual void onConnected(const Ref<ITCPConnection>& connection) = 0;
    virtual void onDisconnected(const Error& error) = 0;
};

class TCPClient : public SimpleRefCountable {
public:
    TCPClient();
    ~TCPClient() override;

    void connect(const std::string& address, int32_t port, const Shared<ITCPClientListener>& listener);

    void disconnect();

private:
    Ref<TCPClientImpl> _impl;
};

} // namespace Valdi
