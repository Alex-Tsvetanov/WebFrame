//
//  GrGraphicsContext.hpp
//  snap_drawing-macos
//
//  Created by Simon Corsin on 2/7/22.
//

#pragma once

#include "snap_drawing/cpp/Drawing/GraphicsContext/GraphicsContext.hpp"
#include "snap_drawing/cpp/Drawing/GraphicsContext/IShaderCache.hpp"
#include "valdi_core/cpp/Utils/Shared.hpp"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wimport-preprocessor-directive-pedantic"

#include "include/gpu/ganesh/GrDirectContext.h"

#pragma clang diagnostic pop

class SkExecutor;

namespace snap::drawing {

class GrGraphicsContextOptions : public Valdi::SimpleRefCountable {
public:
    explicit GrGraphicsContextOptions();
    explicit GrGraphicsContextOptions(const Valdi::Ref<IShaderCache>& cache);

    ~GrGraphicsContextOptions() override;

    GrContextOptions getGrContextOptions() const;

private:
    std::unique_ptr<SkExecutor> _executor;
    Valdi::Ref<IShaderCache> _cache;
};

class GrGraphicsContext : public GraphicsContext {
public:
    GrGraphicsContext(const sk_sp<GrDirectContext>& grContext, const Ref<GrGraphicsContextOptions>& options);
    ~GrGraphicsContext() override;

    void setResourceCacheLimit(size_t resourceCacheLimitBytes) override;

    void performCleanup(bool shouldPurgeScratchResources, std::chrono::seconds secondsNotUsed) override;

    GrDirectContext& getGrContext() const;

    static void setGrResourceCacheLimit(const sk_sp<GrDirectContext>& grContext, size_t resourceCacheLimitBytes);
    static void performGrCleanup(const sk_sp<GrDirectContext>& grContext,
                                 bool shouldPurgeScratchResources,
                                 std::chrono::seconds secondsNotUsed);

private:
    sk_sp<GrDirectContext> _grContext;
    Ref<GrGraphicsContextOptions> _options;
};

} // namespace snap::drawing
