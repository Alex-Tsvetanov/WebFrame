//
//  Bundle.hpp
//  ValdiRuntime
//
//  Created by Simon Corsin on 8/8/18.
//  Copyright © 2018 Snap Inc. All rights reserved.
//

#pragma once

#include <atomic>
#include <memory>
#include <mutex>
#include <string>

#include "valdi_core/cpp/Utils/Bytes.hpp"
#include "valdi_core/cpp/Utils/FlatMap.hpp"
#include "valdi_core/cpp/Utils/Mutex.hpp"
#include "valdi_core/cpp/Utils/Result.hpp"
#include "valdi_core/cpp/Utils/Shared.hpp"
#include "valdi_core/cpp/Utils/StringBox.hpp"

namespace Valdi {

class Bundle;
class ILogger;
class Asset;
class CSSDocument;
class AssetCatalog;

struct JavaScriptFile {
    BytesView content;
    StringBox sourceMap;

    JavaScriptFile();
    JavaScriptFile(const BytesView& content, const StringBox& sourceMap);
    ~JavaScriptFile();
};

class ModuleLoadStrategy : public SimpleRefCountable {
public:
    struct ComponentLoadStrategy {
        std::vector<StringBox> valdiModules;
    };

    ModuleLoadStrategy(FlatMap<StringBox, ComponentLoadStrategy> strategies);
    ~ModuleLoadStrategy() override;

    const ComponentLoadStrategy* getStrategyForComponentPath(const StringBox& componentPath) const;

    static Result<Ref<ModuleLoadStrategy>> parse(const Byte* data, size_t length);

private:
    FlatMap<StringBox, ComponentLoadStrategy> _strategies;
};

class ValdiModuleArchive;
class ValdiModuleArchiveLoader;
class AttributeIds;

class Bundle;

class BundleInitializer {
public:
    explicit BundleInitializer(Bundle* bundle);
    ~BundleInitializer();

    void initWithLocalArchive(Ref<ValdiModuleArchive> decompressedBundle, bool hasRemoteAssets);
    void initWithRemoteArchive(bool hasRemoteAssets);

    const Ref<Bundle>& getBundle() const;

private:
    Ref<Bundle> _bundle;
    std::lock_guard<std::recursive_mutex> _lock;
};

struct BundleResourceContent {
    BytesView raw;
    Ref<RefCountable> processed;

    BundleResourceContent();
    BundleResourceContent(const BytesView& raw);
    BundleResourceContent(const Ref<RefCountable>& processed);
    ~BundleResourceContent();
};

class Bundle : public SimpleRefCountable {
public:
    Bundle(StringBox name, ILogger& logger);
    ~Bundle() override;

    BundleInitializer prepareForInit();

    bool hasRemoteAssets() const;

    Result<JavaScriptFile> getJs(const StringBox& jsPath);

    /**
     Allows to replace a js file without changing the entire bundle.
     */
    void setJs(const StringBox& jsPath, const JavaScriptFile& jsFile);

    std::vector<StringBox> getAllJsPaths();

    Result<Ref<CSSDocument>> getCSSDocument(const StringBox& path, AttributeIds& attributeIds);

    void setCSSDocument(const StringBox& path, const Ref<CSSDocument>& cssDocument);

    Result<BundleResourceContent> getResourceContent(const StringBox& path);
    void setResourceContent(const StringBox& path, const BundleResourceContent& resourceContent);

    Result<Ref<ModuleLoadStrategy>> getModuleLoadStrategy();
    bool hasModuleLoadStrategy();

    const StringBox& getName() const;

    Result<StringBox> getHash();

    void unloadUnusedResources();

    Result<BytesView> getEntry(const StringBox& path);
    void setEntry(const StringBox& path, const BytesView& data);
    bool hasEntry(const StringBox& path);
    std::vector<StringBox> getAllEntryPaths();

    Result<Ref<AssetCatalog>> getAssetCatalog(const StringBox& assetCatalogPath);

    void setAssetCatalog(const StringBox& assetCatalogPath, const Ref<AssetCatalog>& assetCatalog);

    bool hasRemoteArchive() const;
    bool hasRemoteArchiveNeedingLoad() const;

    void setLoadedArchiveIfNeeded(const Ref<ValdiModuleArchive>& archive);
    bool hasLoadedArchive() const;

    std::unique_lock<std::recursive_mutex> lock() const;

    bool initialized() const {
        return _initialized.load(std::memory_order_relaxed);
    }

private:
    friend BundleInitializer;

    StringBox _name;

    Ref<ValdiModuleArchive> _decompressedBundle;
    bool _hasRemoteAssets = false;
    bool _hasRemoteArchive = false;
    bool _loadedEntries = false;
    std::atomic<bool> _initialized = false;

    [[maybe_unused]] ILogger& _logger;
    mutable std::recursive_mutex _mutex;

    FlatMap<StringBox, JavaScriptFile> _jsFilesByPath;
    FlatMap<StringBox, Ref<CSSDocument>> _cssDocumentByPath;
    FlatMap<StringBox, Ref<AssetCatalog>> _assetCatalogByPath;
    FlatMap<StringBox, BundleResourceContent> _resourceContentByPath;
    FlatMap<StringBox, BytesView> _entryByPath;
    std::vector<StringBox> _allEntryPaths;

    Result<Void> lockFreeLoadEntriesIfNeeded();

    Result<Ref<AssetCatalog>> lockFreeGetAssetCatalog(const StringBox& assetCatalogPath);
};

} // namespace Valdi
