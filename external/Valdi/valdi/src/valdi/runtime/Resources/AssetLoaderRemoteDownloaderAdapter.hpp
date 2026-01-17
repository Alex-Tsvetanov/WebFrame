//
//  AssetLoaderRemoteDownloaderAdapter.hpp
//  valdi
//
//  Created by Simon Corsin on 7/19/22.
//

#pragma once

#include "valdi/runtime/Interfaces/IRemoteDownloader.hpp"
#include "valdi/runtime/Resources/AssetLoader.hpp"

namespace Valdi {

/**
 Implemention of IRemoteDownloader which uses an AssetLoader
 to retrieve the bytes.
 */
class AssetLoaderRemoteDownloaderAdapter : public IRemoteDownloader {
public:
    explicit AssetLoaderRemoteDownloaderAdapter(const Ref<AssetLoader>& assetLoader);
    ~AssetLoaderRemoteDownloaderAdapter() override;

    const Ref<AssetLoader>& getAssetLoader() const;

    Shared<snap::valdi_core::Cancelable> downloadItem(const StringBox& url,
                                                      Function<void(const Result<BytesView>&)> completion) override;

private:
    Ref<AssetLoader> _assetLoader;
};

} // namespace Valdi
