//
//  RemoteModuleManager.cpp
//  valdi-ios
//
//  Created by Simon Corsin on 8/28/19.
//

#include "valdi/runtime/Resources/Remote/RemoteModuleManager.hpp"

#include "valdi/runtime/Interfaces/IDiskCache.hpp"
#include "valdi/runtime/Metrics/Metrics.hpp"
#include "valdi/runtime/Resources/AssetDensityResolver.hpp"
#include "valdi/runtime/Resources/Remote/DownloadableModuleManifestWrapper.hpp"
#include "valdi/runtime/Resources/Remote/RemoteModuleResources.hpp"
#include "valdi/runtime/Resources/ValdiModuleArchive.hpp"
#include "valdi/runtime/Utils/AsyncGroup.hpp"
#include "valdi_core/cpp/Resources/ValdiArchive.hpp"
#include "valdi_core/cpp/Threading/DispatchQueue.hpp"
#include "valdi_core/cpp/Utils/LoggerUtils.hpp"
#include "valdi_core/cpp/Utils/PathUtils.hpp"
#include "valdi_core/cpp/Utils/StringCache.hpp"
#include "valdi_core/cpp/Utils/ValdiObject.hpp"

#include "valdi_core/cpp/Interfaces/ILogger.hpp"

namespace Valdi {

class ValdiModuleArchiveWrapper : public ValdiObject {
public:
    explicit ValdiModuleArchiveWrapper(Ref<ValdiModuleArchive> decompressedBundle)
        : _decompressedBundle(std::move(decompressedBundle)) {}

    ~ValdiModuleArchiveWrapper() override = default;

    const Ref<ValdiModuleArchive>& getValdiModuleArchive() const {
        return _decompressedBundle;
    }

    VALDI_CLASS_HEADER_IMPL(ValdiModuleArchiveWrapper)

private:
    Ref<ValdiModuleArchive> _decompressedBundle;
};

// For modules, we keep them compressed on disk, we decompress them when we want to read them.

Result<Value> ModuleBundleResultTransformer::transform(const StringBox& /*localFilename*/,
                                                       const BytesView& data) const {
    Result<ValdiModuleArchive> decompressedBundleResult;
    if (_decompressionDisabled) {
        decompressedBundleResult = ValdiModuleArchive::deserialize(data);
    } else {
        decompressedBundleResult = ValdiModuleArchive::decompress(data.data(), data.size());
    }

    if (!decompressedBundleResult) {
        return decompressedBundleResult.moveError();
    }

    return Value(Valdi::makeShared<ValdiModuleArchiveWrapper>(
        Valdi::makeShared<ValdiModuleArchive>(decompressedBundleResult.moveValue())));
}

std::string_view ModuleBundleResultTransformer::getItemTypeDescription() const {
    return "module";
}

void ModuleBundleResultTransformer::setDecompressionDisabled(bool decompressionDisabled) {
    _decompressionDisabled = decompressionDisabled;
}

// For resources, we decompress the module and save every resource into an individual file on disk.
// We store a file that acts as the manifest for all the available files stored locally.

ResourcesBundleResultTransformer::ResourcesBundleResultTransformer(Ref<IDiskCache> diskCache, ILogger& logger)
    : _diskCache(std::move(diskCache)), _logger(logger) {}

Result<BytesView> ResourcesBundleResultTransformer::preprocess(const StringBox& localFilename,
                                                               const BytesView& remoteData) const {
    if (_diskCache == nullptr) {
        return Error("Cannot load resources without a disk cache");
    }

    _diskCache->remove(Path(localFilename));

    Result<ValdiModuleArchive> decompressedBundleResult;
    if (_decompressionDisabled) {
        decompressedBundleResult = ValdiModuleArchive::deserialize(remoteData);
    } else {
        decompressedBundleResult = ValdiModuleArchive::decompress(remoteData.data(), remoteData.size());
    }

    if (!decompressedBundleResult) {
        return decompressedBundleResult.moveError();
    }

    auto decompressedBundle = Valdi::makeShared<ValdiModuleArchive>(decompressedBundleResult.moveValue());

    ValdiArchiveBuilder manifestBuilder;

    Path cacheDirectoryPath(localFilename.toStringView());
    cacheDirectoryPath.removeFileExtension();
    cacheDirectoryPath.appendFileExtension("dir");

    for (const auto& path : decompressedBundle->getAllEntryPaths()) {
        auto entry = decompressedBundle->getEntry(path);
        SC_ASSERT(entry.has_value(), "Unable to retrieve module entry");

        auto bytesView = BytesView(decompressedBundle, entry->data, entry->size);

        auto cachePath = cacheDirectoryPath.appending(path.toStringView());

        auto storeSuccess = _diskCache->store(cachePath, bytesView);
        if (!storeSuccess) {
            return storeSuccess.error().rethrow(
                STRING_FORMAT("Failed to store resource item '{}' in disk cache", path));
        }

        manifestBuilder.addEntry(ValdiArchiveEntry(path, cachePath.toStringBox()));
    }

    auto moduleBytes = manifestBuilder.build();
    return moduleBytes->toBytesView();
}

std::string_view ResourcesBundleResultTransformer::getItemTypeDescription() const {
    return "assets package";
}

void ResourcesBundleResultTransformer::setDecompressionDisabled(bool decompressionDisabled) {
    _decompressionDisabled = decompressionDisabled;
}

void ResourcesBundleResultTransformer::doAppendKeyPathToResourceCacheUrl(
    const StringBox& localFilename,
    const StringBox& key,
    const StringBox& url,
    FlatMap<StringBox, StringBox>& resourceCacheUrls) const {
    if (resourceCacheUrls.find(key) != resourceCacheUrls.end()) {
        VALDI_ERROR(_logger, "Duplicate entry '{}' in remote resource artifact '{}'", key, localFilename);
        return;
    }
    resourceCacheUrls[key] = url;
}

Result<StringBox> ResourcesBundleResultTransformer::sanitizeAssetFilename(const StringBox& localFilename) {
    Path entryPath(localFilename.toStringView());
    if (entryPath.empty()) {
        return Error(STRING_FORMAT("Invalid asset filename '{}'", localFilename));
    }

    while (entryPath.getComponents().size() > 1) {
        entryPath.removeFirstComponent();
    }
    entryPath.removeFileExtension();

    auto cleanedFilename = entryPath.getLastComponent();
    // Also remove @ for iOS.
    auto atIndex = cleanedFilename.find_last_of('@');
    if (atIndex != std::string_view::npos) {
        cleanedFilename = cleanedFilename.substr(0, atIndex);
    }

    return StringCache::getGlobal().makeString(cleanedFilename).replacing('-', '_');
}

Result<Void> ResourcesBundleResultTransformer::appendEntryToResourceCacheUrl(
    const StringBox& localFilename,
    const ValdiArchiveEntry& entry,
    FlatMap<StringBox, StringBox>& resourceCacheUrls) const {
    if (_diskCache == nullptr) {
        return Error("Cannot load resources without a disk cache");
    }
    auto cachePath = Path(entry.getStringData());
    if (!_diskCache->exists(cachePath)) {
        return Error(STRING_FORMAT("Resource '{}' is missing from the disk cache", cachePath));
    }

    auto absoluteUrl = _diskCache->getAbsoluteURL(cachePath);
    doAppendKeyPathToResourceCacheUrl(localFilename, entry.filePath, absoluteUrl, resourceCacheUrls);

    // Also add an entry with a sanitized filename, so that we can query it from the name without the path and extension

    auto cleanedFilename = sanitizeAssetFilename(entry.filePath);
    if (!cleanedFilename) {
        VALDI_ERROR(_logger, "Error in remote resource artifact '{}': {}", entry.filePath, cleanedFilename.error());
        // We don't make this a full failure since other resources in this module might still work.
        return Void();
    }

    if (cleanedFilename.value() != entry.filePath) {
        doAppendKeyPathToResourceCacheUrl(localFilename, cleanedFilename.value(), absoluteUrl, resourceCacheUrls);
    }

    return Void();
}

Result<Value> ResourcesBundleResultTransformer::transform(const StringBox& localFilename, const BytesView& data) const {
    FlatMap<StringBox, StringBox> resourceCacheUrls;

    Result<Void> processResult = Void();

    ValdiArchive module(data.begin(), data.end());
    auto entries = module.getEntries();

    if (entries) {
        for (const auto& entry : entries.value()) {
            processResult = appendEntryToResourceCacheUrl(localFilename, entry, resourceCacheUrls);
            if (!processResult) {
                break;
            }
        }
    } else {
        processResult = Result<Void>(entries.moveError());
    }

    if (processResult.failure()) {
        return processResult.error().rethrow(STRING_FORMAT("Unable to load resources from '{}'", localFilename));
    }

    return Value(Valdi::makeShared<RemoteModuleResources>(std::move(resourceCacheUrls)));
}

RemoteModuleManager::RemoteModuleManager(const Ref<IDiskCache>& diskCache,
                                         const Holder<Shared<snap::valdi_core::HTTPRequestManager>>& requestManager,
                                         Ref<DispatchQueue> workerQueue,
                                         ILogger& logger,
                                         double deviceDensity)
    : _diskCache(diskCache),
      _workerQueue(std::move(workerQueue)),
      _logger(logger),
      _deviceDensity(deviceDensity),
      _resourcesTransformer(_diskCache, logger) {
    _downloader = Valdi::makeShared<RemoteDownloader>(_diskCache, requestManager, _workerQueue, _logger);
}

RemoteModuleManager::~RemoteModuleManager() = default;

void RemoteModuleManager::registerManifest(const StringBox& moduleName,
                                           const Ref<DownloadableModuleManifestWrapper>& moduleManifest) {
    std::lock_guard<Mutex> guard(_mutex);
    _registeredManifests[moduleName] = moduleManifest;
}

Ref<DownloadableModuleManifestWrapper> RemoteModuleManager::getRegisteredManifest(const StringBox& moduleName) const {
    std::lock_guard<Mutex> guard(_mutex);

    return lockFreeGetRegisteredManifest(moduleName);
}

Ref<DownloadableModuleManifestWrapper> RemoteModuleManager::lockFreeGetRegisteredManifest(
    const StringBox& moduleName) const {
    const auto& it = _registeredManifests.find(moduleName);
    if (it == _registeredManifests.end()) {
        return nullptr;
    }
    return it->second;
}

const Valdi::DownloadableModuleAssets* RemoteModuleManager::findBestAsset(
    const DownloadableModuleManifestWrapper& manifest) const {
    AssetDensityResolver densityResolver;
    for (const auto& asset : manifest.pb.assets()) {
        densityResolver.appendDensity(asset.device_density());
    }

    auto bestIndex = densityResolver.select(_deviceDensity);
    if (!bestIndex) {
        return nullptr;
    }

    return &manifest.pb.assets(static_cast<int>(bestIndex.value()));
}

std::optional<StringBox> RemoteModuleManager::findBestAssetUrl(
    const DownloadableModuleManifestWrapper& manifest) const {
    const auto* asset = findBestAsset(manifest);
    if (asset == nullptr) {
        return {};
    }

    return {StringCache::getGlobal().makeString(asset->artifact().url())};
}

void RemoteModuleManager::handleResourcesLoadCompleted(
    const StringBox& module,
    const Result<Value>& result,
    RemoteDownloaderLoadSource loadSource,
    const Function<void(Result<Ref<RemoteModuleResources>>)>& completion) {
    if (_metrics != nullptr) {
        if (loadSource == RemoteDownloaderLoadSourceNetwork) {
            _metrics->emitAssetsCacheMiss(module);

            if (result.failure()) {
                _metrics->emitAssetsDownloadFailure(module);
            } else {
                _metrics->emitAssetsDownloadSuccess(module);
            }
        } else if (loadSource == RemoteDownloaderLoadSourceDiskCache) {
            _metrics->emitAssetsCacheHit(module);
        }
    }

    if (result.failure()) {
        completion(result.error());
        return;
    }

    auto bundleResources = castOrNull<RemoteModuleResources>(result.value().getValdiObject());

    completion(bundleResources);
}

void RemoteModuleManager::loadResources(const StringBox& moduleName,
                                        Function<void(Result<Ref<RemoteModuleResources>>)> completion) {
    auto manifest = getRegisteredManifest(moduleName);
    if (manifest == nullptr) {
        completion(Error(STRING_FORMAT("Module '{}' doesnt have a downloadable manifest registered", moduleName)));
        return;
    }

    const auto* asset = findBestAsset(*manifest);
    if (asset == nullptr) {
        // No resources in this manifest, we consider this a special case of success
        static auto kEmptyResources = new RemoteModuleResources();

        completion(Ref(kEmptyResources));
        return;
    }
    const auto& artifact = asset->artifact();
    auto assetUrl = StringCache::getGlobal().makeString(artifact.url());
    const auto& sha256Digest = artifact.sha256digest();

    auto sha256DigestBytesView =
        BytesView(manifest, reinterpret_cast<const Byte*>(sha256Digest.data()), sha256Digest.size());

    auto weakThis = weakRef(this);
    _downloader->enqueue(
        moduleName.prepend("resources-"),
        assetUrl,
        _resourcesTransformer,
        sha256DigestBytesView,
        [weakThis, completion = std::move(completion), manifest, artifact, moduleName](auto result, auto loadSource) {
            auto strongThis = weakThis.lock();
            if (strongThis != nullptr) {
                strongThis->handleResourcesLoadCompleted(moduleName, result, loadSource, completion);
            }
        });
}

void RemoteModuleManager::handleModuleLoadCompleted(const Result<Value>& result,
                                                    const Function<void(Result<Ref<ValdiModuleArchive>>)>& completion) {
    if (result.failure()) {
        completion(result.error());
        return;
    }

    auto bundleWrapper = unsafeCast<ValdiModuleArchiveWrapper>(result.value().getValdiObject());

    completion(bundleWrapper->getValdiModuleArchive());
}

void RemoteModuleManager::loadModule(const StringBox& moduleName,
                                     Function<void(Result<Ref<ValdiModuleArchive>>)> completion) {
    auto manifest = getRegisteredManifest(moduleName);
    if (manifest == nullptr) {
        completion(Error(STRING_FORMAT("Module '{}' doesnt have a downloadable manifest registered", moduleName)));
        return;
    }

    auto url = StringCache::getGlobal().makeString(manifest->pb.artifact().url());
    const auto& sha256Digest = manifest->pb.artifact().sha256digest();
    auto sha256DigestBytesView =
        BytesView(manifest, reinterpret_cast<const Byte*>(sha256Digest.data()), sha256Digest.size());

    auto weakThis = weakRef(this);
    _downloader->enqueue(moduleName.prepend("module-"),
                         url,
                         _moduleTransformer,
                         sha256DigestBytesView,
                         [weakThis, completion = std::move(completion)](auto result, auto /*loadSource*/) {
                             auto strongThis = weakThis.lock();
                             if (strongThis != nullptr) {
                                 strongThis->handleModuleLoadCompleted(result, completion);
                             }
                         });
}

void RemoteModuleManager::setDecompressionDisabled(bool decompressionDisabled) {
    _moduleTransformer.setDecompressionDisabled(decompressionDisabled);
    _resourcesTransformer.setDecompressionDisabled(decompressionDisabled);
}

} // namespace Valdi
