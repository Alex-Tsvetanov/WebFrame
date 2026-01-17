//
//  TCPServer.cpp
//  valdi-ios
//
//  Created by Simon Corsin on 9/27/19.
//

#include "valdi/runtime/Debugger/TCPServer.hpp"
#include "valdi/runtime/Debugger/TCPServerImpl.hpp"

namespace Valdi {

Ref<ITCPServer> TCPServer::create(uint32_t port, ITCPServerListener* listener) {
    return Valdi::makeShared<TCPServerImpl>(port, listener);
}

} // namespace Valdi
