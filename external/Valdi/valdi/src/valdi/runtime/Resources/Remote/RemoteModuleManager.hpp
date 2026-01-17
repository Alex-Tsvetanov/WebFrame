//
//  RemoteModuleManager.hpp
//  valdi-ios
//
//  Created by Simon Corsin on 8/28/19.
//

#pragma once

#include "valdi/runtime/Resources/Remote/RemoteDownloader.hpp"
#include "valdi/valdi.pb.h"
#include "valdi_core/cpp/Utils/FlatMap.hpp"
#include "valdi_core/cpp/Utils/Function.hpp"
#include "valdi_core/cpp/Utils/Holder.hpp"
#include "valdi_core/cpp/Utils/Mutex.hpp"
#include "valdi_core/cpp/Utils/Result.hpp"
#include "valdi_core/cpp/Utils/Shared.hpp"
#include "valdi_core/cpp/Utils/StringBox.hpp"
#include <mutex>
#include <optional>
#include <vector>

namespace snap::valdi {
class HTTPRequestManager;
} // namespace snap::valdi

namespace Valdi {

class DownloadableModuleAssets;
class ILogger;
class ValdiModuleArchive;
class RemoteModuleResources;
class DispatchQueue;
struct ValdiArchiveEntry;
class IDiskCache;
class DownloadableModuleManifestWrapper;
class Metrics;

class ModuleBundleResultTransformer : public IRemoteDownloaderItemHandler {
public:
    Result<Value> transform(const StringBox& localFilename, const BytesView& data) const override;

    std::string_view getItemTypeDescription() const override;

    void setDecompressionDisabled(bool decompressionDisabled);

private:
    bool _decompressionDisabled = false;
};

class ResourcesBundleResultTransformer : public IRemoteDownloaderItemHandler {
public:
    ResourcesBundleResultTransformer(Ref<IDiskCache> diskCache, ILogger& logger);

    Result<BytesView> preprocess(const StringBox& localFilename, const BytesView& remoteData) const override;

    Result<Value> transform(const StringBox& localFilename, const BytesView& data) const override;

    std::string_view getItemTypeDescription() const override;

    void setDecompressionDisabled(bool decompressionDisabled);

    static Result<StringBox> sanitizeAssetFilename(const StringBox& localFilename);

private:
    Ref<IDiskCache> _diskCache;
    [[maybe_unused]] ILogger& _logger;
    bool _decompressionDisabled = false;

    [[nodiscard]] Result<Void> appendEntryToResourceCacheUrl(const StringBox& localFilename,
                                                             const ValdiArchiveEntry& entry,
                                                             FlatMap<StringBox, StringBox>& resourceCacheUrls) const;
    void doAppendKeyPathToResourceCacheUrl(const StringBox& localFilename,
                                           const StringBox& key,
                                           const StringBox& url,
                                           FlatMap<StringBox, StringBox>& resourceCacheUrls) const;
};

class RemoteModuleManager : public SharedPtrRefCountable {
public:
    RemoteModuleManager(const Ref<IDiskCache>& diskCache,
                        const Holder<Shared<snap::valdi_core::HTTPRequestManager>>& requestManager,
                        Ref<DispatchQueue> workerQueue,
                        ILogger& logger,
                        double deviceDensity);
    ~RemoteModuleManager() override;

    void registerManifest(const StringBox& moduleName, const Ref<DownloadableModuleManifestWrapper>& moduleManifest);
    Ref<DownloadableModuleManifestWrapper> getRegisteredManifest(const StringBox& moduleName) const;

    void loadModule(const StringBox& moduleName, Function<void(Result<Ref<ValdiModuleArchive>>)> completion);
    void loadResources(const StringBox& moduleName, Function<void(Result<Ref<RemoteModuleResources>>)> completion);

    void setMetrics(const Ref<Metrics>& metrics);

    void setDecompressionDisabled(bool decompressionDisabled);

private:
    Ref<IDiskCache> _diskCache;
    Ref<DispatchQueue> _workerQueue;
    [[maybe_unused]] ILogger& _logger;
    double _deviceDensity;
    ModuleBundleResultTransformer _moduleTransformer;
    ResourcesBundleResultTransformer _resourcesTransformer;
    mutable Mutex _mutex;

    FlatMap<StringBox, Ref<DownloadableModuleManifestWrapper>> _registeredManifests;

    Shared<RemoteDownloader> _downloader;
    Ref<Metrics> _metrics;

    Ref<DownloadableModuleManifestWrapper> lockFreeGetRegisteredManifest(const StringBox& moduleName) const;

    void enqueueRequest(const StringBox& localFilename,
                        const std::string& url,
                        Function<void(Result<Void>)> completion);

    void handleModuleLoadCompleted(const Result<Value>& result,
                                   const Function<void(Result<Ref<ValdiModuleArchive>>)>& completion);
    void handleResourcesLoadCompleted(const StringBox& module,
                                      const Result<Value>& result,
                                      RemoteDownloaderLoadSource loadSource,
                                      const Function<void(Result<Ref<RemoteModuleResources>>)>& completion);

    const Valdi::DownloadableModuleAssets* findBestAsset(const DownloadableModuleManifestWrapper& manifest) const;
    std::optional<StringBox> findBestAssetUrl(const DownloadableModuleManifestWrapper& manifest) const;
};

} // namespace Valdi
