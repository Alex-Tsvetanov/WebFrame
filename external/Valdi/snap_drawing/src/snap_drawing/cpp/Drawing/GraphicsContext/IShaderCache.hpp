#pragma once

#include "valdi_core/cpp/Utils/Shared.hpp"

#ifdef SK_GANESH

#include "include/gpu/ganesh/GrDirectContext.h"

namespace snap::drawing {

class IShaderCache : public GrContextOptions::PersistentCache, public Valdi::SharedPtrRefCountable {};

} // namespace snap::drawing

#else

#include "include/core/SkData.h"

namespace snap::drawing {

class IShaderCache : public Valdi::SharedPtrRefCountable {
public:
    virtual sk_sp<SkData> load(const SkData& key) = 0;
    virtual void store(const SkData& key, const SkData& data) = 0;
};

} // namespace snap::drawing

#endif