//
//  IRuntimeListener.hpp
//  ValdiRuntime
//
//  Created by Simon Corsin on 5/24/18.
//  Copyright © 2018 Snap Inc. All rights reserved.
//

#pragma once

#include "valdi/runtime/Context/Context.hpp"

namespace Valdi {

class RenderRequest;
class Runtime;

class IRuntimeListener {
public:
    IRuntimeListener() = default;
    virtual ~IRuntimeListener() = default;

    virtual void onContextCreated(Runtime& runtime, const SharedContext& context) = 0;

    virtual void onContextDestroyed(Runtime& runtime, Context& context) = 0;

    /**
     Called whenever there isn't any Context alive anymore.
     */
    virtual void onAllContextsDestroyed(Runtime& runtime) = 0;

    virtual void onContextRendered(Runtime& runtime, const SharedContext& context) = 0;

    virtual void onContextLayoutBecameDirty(Runtime& runtime, const SharedContext& context) = 0;
};

} // namespace Valdi
