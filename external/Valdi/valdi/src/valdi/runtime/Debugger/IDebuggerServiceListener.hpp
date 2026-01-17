//
//  IDebuggerServiceListener.hpp
//  valdi
//
//  Created by Simon Corsin on 9/20/21.
//

#pragma once

#include "valdi_core/cpp/Utils/Shared.hpp"
#include <vector>

namespace Valdi {

struct Resource;
class IDaemonClient;
class BytesView;

class IDebuggerServiceListener : public SharedPtrRefCountable {
public:
    ~IDebuggerServiceListener() override = default;

    virtual void receivedUpdatedResources(const std::vector<Shared<Resource>>& resources) = 0;
    virtual void daemonClientConnected(const Shared<IDaemonClient>& daemonClient) = 0;
    virtual void daemonClientDisconnected(const Shared<IDaemonClient>& daemonClient) = 0;
    virtual void daemonClientDidReceiveClientPayload(const Shared<IDaemonClient>& daemonClient,
                                                     int senderClientId,
                                                     const BytesView& payload) = 0;
};

} // namespace Valdi
