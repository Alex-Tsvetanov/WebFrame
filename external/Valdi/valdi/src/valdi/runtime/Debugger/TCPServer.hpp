//
//  TCPServer.hpp
//  valdi-ios
//
//  Created by Simon Corsin on 9/27/19.
//

#pragma once

#include "valdi/runtime/Debugger/ITCPServer.hpp"
#include "valdi_core/cpp/Utils/Shared.hpp"

namespace Valdi {

class TCPServer {
public:
    static Ref<ITCPServer> create(uint32_t port, ITCPServerListener* listener);
};
} // namespace Valdi
