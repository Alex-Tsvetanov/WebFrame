//
//  AssetLoaderManager.cpp
//  valdi
//
//  Created by Simon Corsin on 7/9/21.
//

#include "valdi/runtime/Resources/AssetLoaderManager.hpp"
#include "valdi/runtime/Resources/AssetLoader.hpp"
#include "valdi/runtime/Resources/AssetLoaderFactory.hpp"
#include "valdi/runtime/Resources/AssetLoaderRemoteDownloaderAdapter.hpp"

namespace Valdi {

AssetLoaderManager::AssetLoaderManager() = default;
AssetLoaderManager::~AssetLoaderManager() = default;

void AssetLoaderManager::registerAssetLoader(const Ref<AssetLoader>& assetLoader) {
    std::lock_guard<Mutex> mutex(_mutex);
    lockFreeRegisterAssetLoader(assetLoader, nullptr, _entries.size());
}

void AssetLoaderManager::lockFreeRegisterAssetLoader(const Ref<AssetLoader>& assetLoader,
                                                     const Ref<AssetLoader>& sourceAssetLoader,
                                                     size_t insertionIndex) {
    auto outputType = assetLoader->getOutputType();
    const auto& schemes = assetLoader->getSupportedSchemes();

    for (const auto& scheme : schemes) {
        auto entry = _entries.emplace(_entries.begin() + insertionIndex);
        entry->urlScheme = scheme;
        entry->outputType = outputType;
        entry->assetLoader = assetLoader;
        entry->sourceAssetLoader = sourceAssetLoader;

        insertionIndex++;
    }
}

void AssetLoaderManager::unregisterAssetLoader(const Ref<AssetLoader>& assetLoader) {
    std::lock_guard<Mutex> mutex(_mutex);

    auto it = _entries.begin();
    while (it != _entries.end()) {
        if (it->assetLoader == assetLoader || it->sourceAssetLoader == assetLoader) {
            it = _entries.erase(it);
        } else {
            it++;
        }
    }
}

void AssetLoaderManager::registerAssetLoaderFactory(const Ref<AssetLoaderFactory>& assetLoaderFactory) {
    std::lock_guard<Mutex> mutex(_mutex);
    _factories.emplace_back(assetLoaderFactory);
}

void AssetLoaderManager::registerDownloaderForScheme(const StringBox& urlScheme,
                                                     const Ref<IRemoteDownloader>& downloader) {
    std::lock_guard<Mutex> mutex(_mutex);
    lockFreeRegisterDownloaderForScheme(urlScheme, downloader);
}

void AssetLoaderManager::lockFreeRegisterDownloaderForScheme(const StringBox& urlScheme,
                                                             const Ref<IRemoteDownloader>& downloader) {
    _downloaderByScheme[urlScheme] = downloader;
}

static bool canUseRegisteredAssetLoader(const RegisteredAssetLoader& registeredAssetLoader,
                                        const RegisteredAssetLoader* byteAssetLoaderCandidate) {
    if (registeredAssetLoader.sourceAssetLoader == nullptr || byteAssetLoaderCandidate == nullptr) {
        return true;
    }

    return byteAssetLoaderCandidate->assetLoader == registeredAssetLoader.sourceAssetLoader;
}

Ref<AssetLoader> AssetLoaderManager::resolveAssetLoader(const StringBox& urlScheme,
                                                        snap::valdi_core::AssetOutputType outputType) {
    std::lock_guard<Mutex> mutex(_mutex);

    // Could change later to a map for potentially faster query performance.
    const RegisteredAssetLoader* bytesAssetLoaderCandidate = nullptr;

    size_t i = _entries.size();
    while (i > 0) {
        i--;
        const auto& entry = _entries[i];
        if (entry.urlScheme == urlScheme) {
            if (entry.outputType == outputType && canUseRegisteredAssetLoader(entry, bytesAssetLoaderCandidate)) {
                return entry.assetLoader;
            }

            if (entry.outputType == snap::valdi_core::AssetOutputType::Bytes && bytesAssetLoaderCandidate == nullptr) {
                bytesAssetLoaderCandidate = &entry;
            }
        }
    }

    if (outputType != snap::valdi_core::AssetOutputType::Bytes) {
        const auto& downloaderIt = _downloaderByScheme.find(urlScheme);
        if (downloaderIt != _downloaderByScheme.end()) {
            return tryCreateAssetLoaderFromFactory(
                urlScheme, outputType, /* sourceAssetLoader */ nullptr, downloaderIt->second, _entries.size());
        } else if (bytesAssetLoaderCandidate != nullptr) {
            auto sourceAssetLoader = bytesAssetLoaderCandidate->assetLoader;
            auto remoteDownloader = makeShared<AssetLoaderRemoteDownloaderAdapter>(sourceAssetLoader);

            return tryCreateAssetLoaderFromFactory(urlScheme,
                                                   outputType,
                                                   sourceAssetLoader,
                                                   remoteDownloader,
                                                   bytesAssetLoaderCandidate - _entries.data());
        }
    }

    return nullptr;
}

Ref<AssetLoader> AssetLoaderManager::tryCreateAssetLoaderFromFactory(const StringBox& urlScheme,
                                                                     snap::valdi_core::AssetOutputType outputType,
                                                                     const Ref<AssetLoader>& sourceAssetLoader,
                                                                     const Ref<IRemoteDownloader>& downloader,
                                                                     size_t insertionIndex) {
    size_t i = _factories.size();
    while (i > 0) {
        i--;
        const auto& factory = _factories[i];

        if (factory->getOutputType() == outputType) {
            std::vector<StringBox> urlSchemes;
            urlSchemes.emplace_back(urlScheme);
            auto createdAssetLoader = factory->createAssetLoader(urlSchemes, downloader);

            lockFreeRegisterAssetLoader(createdAssetLoader, sourceAssetLoader, insertionIndex);

            return createdAssetLoader;
        }
    }

    return nullptr;
}

std::vector<Ref<AssetLoader>> AssetLoaderManager::getAllAssetLoaders() const {
    std::lock_guard<Mutex> mutex(_mutex);

    std::vector<Ref<AssetLoader>> allAssetLoaders;

    for (const auto& entry : _entries) {
        if (std::find(allAssetLoaders.begin(), allAssetLoaders.end(), entry.assetLoader) == allAssetLoaders.end()) {
            allAssetLoaders.emplace_back(entry.assetLoader);
        }
    }

    return allAssetLoaders;
}

} // namespace Valdi
