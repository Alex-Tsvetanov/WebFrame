//
//  ITCPServer.hpp
//  valdi-ios
//
//  Created by Simon Corsin on 10/3/19.
//

#pragma once

#include "valdi/runtime/Debugger/ITCPConnection.hpp"
#include "valdi_core/cpp/Threading/DispatchQueue.hpp"
#include "valdi_core/cpp/Utils/Bytes.hpp"
#include "valdi_core/cpp/Utils/Result.hpp"
#include "valdi_core/cpp/Utils/Shared.hpp"

#include <vector>

namespace Valdi {

class ITCPServerListener {
public:
    virtual ~ITCPServerListener() = default;

    virtual void onClientConnected(const Ref<ITCPConnection>& client) = 0;
    virtual void onClientDisconnected(const Ref<ITCPConnection>& client, const Error& error) = 0;
};

class ITCPServer : public SharedPtrRefCountable {
public:
    [[nodiscard]] virtual Result<Void> start() = 0;
    virtual void stop() = 0;

    virtual uint16_t getBoundPort() = 0;
    virtual std::vector<std::string> getAvailableAddresses() = 0;

    virtual bool ownsClient(const Ref<ITCPConnection>& client) const = 0;

    virtual std::vector<Ref<ITCPConnection>> getConnectedClients() = 0;
};

} // namespace Valdi
