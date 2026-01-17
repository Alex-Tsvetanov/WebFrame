//
//  GraphicsContext.hpp
//  snap_drawing
//
//  Created by Simon Corsin on 1/20/22.
//

#pragma once

#include "snap_drawing/cpp/Utils/Aliases.hpp"
#include "valdi_core/cpp/Utils/Result.hpp"

#include <chrono>

namespace snap::drawing {

class GraphicsContext : public Valdi::SimpleRefCountable {
public:
    virtual void setResourceCacheLimit(size_t resourceCacheLimitBytes) = 0;

    virtual void performCleanup(bool shouldPurgeScratchResources, std::chrono::seconds secondsNotUsed) = 0;

    virtual void commit();
};

} // namespace snap::drawing
