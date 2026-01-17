//
//  AssetLoaderManager.hpp
//  valdi
//
//  Created by Simon Corsin on 7/9/21.
//

#pragma once

#include "valdi_core/cpp/Utils/FlatMap.hpp"
#include "valdi_core/cpp/Utils/Mutex.hpp"
#include "valdi_core/cpp/Utils/Shared.hpp"
#include "valdi_core/cpp/Utils/StringBox.hpp"

#include "valdi_core/AssetOutputType.hpp"

#include <mutex>
#include <vector>

namespace Valdi {

class AssetLoader;
class AssetLoaderFactory;
class IRemoteDownloader;

struct RegisteredAssetLoader {
    StringBox urlScheme;
    snap::valdi_core::AssetOutputType outputType;
    Ref<AssetLoader> assetLoader;
    // If the AssetLoader was generated from a factory, this will contain
    // the source asset loader from which this asset loader was created from
    Ref<AssetLoader> sourceAssetLoader;
};

class AssetLoaderManager : public SimpleRefCountable {
public:
    AssetLoaderManager();
    ~AssetLoaderManager() override;

    std::vector<Ref<AssetLoader>> getAllAssetLoaders() const;

    void registerAssetLoader(const Ref<AssetLoader>& assetLoader);
    void unregisterAssetLoader(const Ref<AssetLoader>& assetLoader);

    void registerAssetLoaderFactory(const Ref<AssetLoaderFactory>& assetLoaderFactory);

    void registerDownloaderForScheme(const StringBox& urlScheme, const Ref<IRemoteDownloader>& downloader);

    Ref<AssetLoader> resolveAssetLoader(const StringBox& urlScheme, snap::valdi_core::AssetOutputType outputType);

private:
    std::vector<RegisteredAssetLoader> _entries;
    std::vector<Ref<AssetLoaderFactory>> _factories;
    FlatMap<StringBox, Ref<IRemoteDownloader>> _downloaderByScheme;
    mutable Mutex _mutex;

    void lockFreeRegisterDownloaderForScheme(const StringBox& urlScheme, const Ref<IRemoteDownloader>& downloader);

    void lockFreeRegisterAssetLoader(const Ref<AssetLoader>& assetLoader,
                                     const Ref<AssetLoader>& sourceAssetLoader,
                                     size_t insertionIndex);
    Ref<AssetLoader> tryCreateAssetLoaderFromFactory(const StringBox& urlScheme,
                                                     snap::valdi_core::AssetOutputType outputType,
                                                     const Ref<AssetLoader>& sourceAssetLoader,
                                                     const Ref<IRemoteDownloader>& downloader,
                                                     size_t insertionIndex);
};

} // namespace Valdi
