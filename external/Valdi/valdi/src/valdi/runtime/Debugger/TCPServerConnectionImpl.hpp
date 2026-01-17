//
//  TCPServerConnectionImpl.hpp
//  valdi-ios
//
//  Created by Simon Corsin on 10/2/19.
//

#pragma once

#include "valdi/runtime/Debugger/TCPConnectionImpl.hpp"

namespace Valdi {

class TCPServerImpl;

class TCPServerConnectionImpl : public TCPConnectionImpl {
public:
    explicit TCPServerConnectionImpl(TCPServerImpl* server);
    ~TCPServerConnectionImpl() override;

    TCPServerImpl& getServer() const;

private:
    TCPServerImpl* _server;
};

} // namespace Valdi
