//
//  TCPServerConnectionImpl.cpp
//  valdi-ios
//
//  Created by Simon Corsin on 10/2/19.
//

#include "valdi/runtime/Debugger/TCPServerConnectionImpl.hpp"
#include "valdi/runtime/Debugger/TCPServerImpl.hpp"

namespace Valdi {

TCPServerConnectionImpl::TCPServerConnectionImpl(TCPServerImpl* server)
    : TCPConnectionImpl(server->getIoService()), _server(server) {}

TCPServerConnectionImpl::~TCPServerConnectionImpl() = default;

TCPServerImpl& TCPServerConnectionImpl::getServer() const {
    return *_server;
}

} // namespace Valdi
