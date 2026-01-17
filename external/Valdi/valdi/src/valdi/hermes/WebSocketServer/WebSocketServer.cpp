//
//  WebSocketServer.cpp
//  valdi
//
//  Created by Edward Lee on 02/20/2024.
//

#include "valdi/hermes/WebSocketServer/WebSocketServer.hpp"

#ifdef VALDI_HAS_WEBSOCKET_SERVER
#include "valdi/hermes/WebSocketServer/WebSocketServerImpl.hpp"
#endif // VALDI_HAS_WEBSOCKET_SERVER

namespace Valdi {

Ref<IWebSocketServer> WebSocketServer::create(uint32_t port, IWebSocketServerListener* listener) {
#ifdef VALDI_HAS_WEBSOCKET_SERVER
    return Valdi::makeShared<WebSocketServerImpl>(port, listener);
#else
    return nullptr;
#endif // VALDI_HAS_WEBSOCKET_SERVER
}

} // namespace Valdi
