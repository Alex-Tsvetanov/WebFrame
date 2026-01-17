//
//  ContextComponentRenderer.hpp
//  valdi-ios
//
//  Created by Simon Corsin on 7/17/19.
//

#pragma once

#include "valdi_core/cpp/Context/ContextId.hpp"

namespace Valdi {

/**
 Abstraction over an object that can render the backing component of a context.
 */
class ContextComponentRenderer {
public:
    ContextComponentRenderer() = default;
    virtual ~ContextComponentRenderer() = default;

    virtual void render(ContextId contextId) = 0;
};

} // namespace Valdi
