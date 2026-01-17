#pragma once

#include "valdi/runtime/Resources/AssetLoader.hpp"

namespace Valdi {

class IRemoteDownloader;

/**
  Implementation of AssetLoader that can emit Bytes assets from a given IRemoteDownloader.
 */
class RemoteDownloaderToAssetLoaderAdapter : public AssetLoader {
public:
    RemoteDownloaderToAssetLoaderAdapter(const Ref<IRemoteDownloader>& remoteDownloader,
                                         std::vector<StringBox> supportedSchemes);
    ~RemoteDownloaderToAssetLoaderAdapter() override;

    snap::valdi_core::AssetOutputType getOutputType() const override;

    Result<Value> requestPayloadFromURL(const StringBox& url) override;

    Shared<snap::valdi_core::Cancelable> loadAsset(const Value& requestPayload,
                                                   int32_t preferredWidth,
                                                   int32_t preferredHeight,
                                                   const Value& associatedData,
                                                   const Ref<AssetLoaderCompletion>& completion) override;

private:
    Ref<IRemoteDownloader> _remoteDownloader;
};

} // namespace Valdi
