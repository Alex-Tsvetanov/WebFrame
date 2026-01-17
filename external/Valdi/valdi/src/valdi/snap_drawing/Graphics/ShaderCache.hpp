//
//  ShaderCache.hpp
//  snap_drawing-macos
//
//  Created by Ramzy Jaber on 4/1/22.
//

#pragma once

#include "valdi_core/cpp/Interfaces/ILogger.hpp"

#include "snap_drawing/cpp/Drawing/GraphicsContext/IShaderCache.hpp"
#include "valdi/runtime/Interfaces/IDiskCache.hpp"
#include "valdi_core/cpp/Threading/DispatchQueue.hpp"
#include "valdi_core/cpp/Utils/ByteBuffer.hpp"
#include "valdi_core/cpp/Utils/FlatMap.hpp"
#include "valdi_core/cpp/Utils/StringBox.hpp"
#include "valdi_core/cpp/Utils/Void.hpp"

namespace snap::drawing {

class ShaderCache : public IShaderCache {
public:
    ShaderCache(const Valdi::Ref<Valdi::IDiskCache>& diskCache,
                const Valdi::Ref<Valdi::DispatchQueue>& dispatchQueue,
                Valdi::ILogger& logger);
    ~ShaderCache() override = default;

    sk_sp<SkData> load(const SkData& key) override;
    void store(const SkData& key, const SkData& data) override;

private:
    Valdi::Result<Valdi::BytesView> loadShaderFromDisk(const Valdi::BytesView& keyData) const;
    void storeShaderToDisk(const Valdi::BytesView& key, const Valdi::BytesView& data) const;

    [[maybe_unused]] Valdi::ILogger& _logger;
    Valdi::Ref<Valdi::DispatchQueue> _queue;
    Valdi::Ref<Valdi::IDiskCache> _diskCache;
};

} // namespace snap::drawing
