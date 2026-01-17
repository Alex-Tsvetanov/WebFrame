//
//  IDaemonClient.hpp
//  valdi-desktop-apple
//
//  Created by Simon Corsin on 8/17/20.
//

#pragma once

#include "valdi_core/cpp/Utils/Function.hpp"
#include "valdi_core/cpp/Utils/Result.hpp"
#include "valdi_core/cpp/Utils/Shared.hpp"
#include "valdi_core/cpp/Utils/Value.hpp"
#include "valdi_core/cpp/Utils/ValueFunction.hpp"

namespace Valdi {

class BytesView;

class IDaemonClient {
public:
    virtual ~IDaemonClient() = default;

    virtual int getConnectionId() const = 0;

    virtual void submitRequest(const Value& payload, const Ref<ValueFunction>& callback) = 0;

    virtual void sendJsDebuggerInfo(uint16_t port, const std::vector<StringBox>& websocketTargets) = 0;
};

} // namespace Valdi
