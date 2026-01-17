//
//  HermesBytecodeCache.hpp
//  valdi-hermes
//
//  Created by Simon Corsin on 1/30/24.
//

#pragma once

#include "valdi/hermes/Hermes.hpp"
#include "valdi/runtime/Interfaces/IDiskCache.hpp"
#include "valdi/runtime/Interfaces/IJavaScriptContext.hpp"
#include "valdi_core/cpp/Utils/ByteBuffer.hpp"

namespace Valdi::Hermes {

class HermesBytecodeCache : public SimpleRefCountable {
public:
    HermesBytecodeCache(const Ref<IDiskCache>& diskCache);
    ~HermesBytecodeCache() override;

private:
    Ref<IDiskCache> _diskCache;
};

} // namespace Valdi::Hermes
