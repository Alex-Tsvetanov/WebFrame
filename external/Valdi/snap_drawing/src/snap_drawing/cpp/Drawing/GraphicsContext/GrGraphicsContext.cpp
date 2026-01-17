//
//  GrGraphicsContext.cpp
//  snap_drawing-macos
//
//  Created by Simon Corsin on 2/7/22.
//

#ifdef SK_GANESH

#include "snap_drawing/cpp/Drawing/GraphicsContext/GrGraphicsContext.hpp"
#include "include/core/SkExecutor.h"
#include <iostream>

namespace snap::drawing {

GrGraphicsContextOptions::GrGraphicsContextOptions() : GrGraphicsContextOptions(nullptr) {};

GrGraphicsContextOptions::~GrGraphicsContextOptions() = default;

GrGraphicsContextOptions::GrGraphicsContextOptions(const Valdi::Ref<IShaderCache>& cache)
    : _executor(SkExecutor::MakeFIFOThreadPool(1, true)), _cache(cache) {};

GrContextOptions GrGraphicsContextOptions::getGrContextOptions() const {
    GrContextOptions options;

    options.fExecutor = _executor.get();

    if (_cache != nullptr) {
        options.fPersistentCache = _cache.get();
        options.fShaderCacheStrategy = GrContextOptions::ShaderCacheStrategy::kBackendBinary;
    }
    return options;
}

GrGraphicsContext::GrGraphicsContext(const sk_sp<GrDirectContext>& grContext,
                                     const Ref<GrGraphicsContextOptions>& options)
    : _grContext(grContext), _options(options) {}

GrGraphicsContext::~GrGraphicsContext() {
    if (_grContext != nullptr) {
        _grContext->abandonContext();
        _grContext = nullptr;
    }

    _options = nullptr;
}

void GrGraphicsContext::setResourceCacheLimit(size_t resourceCacheLimitBytes) {
    setGrResourceCacheLimit(_grContext, resourceCacheLimitBytes);
}

void GrGraphicsContext::performCleanup(bool shouldPurgeScratchResources, std::chrono::seconds secondsNotUsed) {
    performGrCleanup(_grContext, shouldPurgeScratchResources, secondsNotUsed);
}

void GrGraphicsContext::setGrResourceCacheLimit(const sk_sp<GrDirectContext>& grContext,
                                                size_t resourceCacheLimitBytes) {
    if (grContext == nullptr) {
        return;
    }

    grContext->setResourceCacheLimit(resourceCacheLimitBytes);
}

void GrGraphicsContext::performGrCleanup(const sk_sp<GrDirectContext>& grContext,
                                         bool shouldPurgeScratchResources,
                                         std::chrono::seconds secondsNotUsed) {
    if (grContext == nullptr) {
        return;
    }

    if (shouldPurgeScratchResources) {
        grContext->purgeUnlockedResources(GrPurgeResourceOptions::kScratchResourcesOnly);
    }
    grContext->performDeferredCleanup(secondsNotUsed);
}

GrDirectContext& GrGraphicsContext::getGrContext() const {
    return *_grContext;
}

} // namespace snap::drawing

#endif