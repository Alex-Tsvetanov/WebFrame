//
//  WebSocketServer.hpp
//  valdi
//
//  Created by Edward Lee on 02/20/2024.
//

#pragma once

#include "valdi/hermes/WebSocketServer/IWebSocketServer.hpp"
#include "valdi_core/cpp/Utils/Shared.hpp"

namespace Valdi {

class WebSocketServer {
public:
    static Ref<IWebSocketServer> create(uint32_t port, IWebSocketServerListener* listener);
};
} // namespace Valdi
