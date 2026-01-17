//
//  BytesAssetLoader.hpp
//  valdi
//
//  Created by Ramzy Jaber on 3/18/22.
//

#pragma once

#include "valdi/runtime/Resources/AssetLoader.hpp"
#include "valdi_core/cpp/Utils/Holder.hpp"

namespace snap::valdi_core {
class HTTPRequestManager;
} // namespace snap::valdi_core

namespace Valdi {

class RemoteDownloader;
class IDiskCache;
class ILogger;
class DispatchQueue;

/**
 Implementation of AssetLoader which can load bytes Asset when
 configured with a HTTP request manager.
 */
class BytesAssetLoader : public AssetLoader {
public:
    BytesAssetLoader(const Ref<IDiskCache>& diskCache,
                     const Holder<Shared<snap::valdi_core::HTTPRequestManager>>& requestManager,
                     const Ref<DispatchQueue>& workerQueue,
                     ILogger& logger);
    ~BytesAssetLoader() override;

    snap::valdi_core::AssetOutputType getOutputType() const override;

    Result<Value> requestPayloadFromURL(const StringBox& url) override;

    Shared<snap::valdi_core::Cancelable> loadAsset(const Value& requestPayload,
                                                   int32_t preferredWidth,
                                                   int32_t preferredHeight,
                                                   const Value& associatedData,
                                                   const Ref<AssetLoaderCompletion>& completion) override;

private:
    Shared<RemoteDownloader> _downloader;
};

} // namespace Valdi
