//
//  Bundle.cpp
//  ValdiRuntime
//
//  Created by Simon Corsin on 8/8/18.
//  Copyright © 2018 Snap Inc. All rights reserved.
//

#include "valdi/runtime/Resources/Bundle.hpp"
#include "utils/time/StopWatch.hpp"
#include "valdi/runtime/CSS/CSSDocument.hpp"
#include "valdi/runtime/Resources/AssetCatalog.hpp"
#include "valdi/runtime/Resources/ValdiModuleArchive.hpp"
#include "valdi_core/cpp/Attributes/AttributeUtils.hpp"
#include "valdi_core/cpp/Utils/LoggerUtils.hpp"
#include "valdi_core/cpp/Utils/StringCache.hpp"
#include "valdi_core/cpp/Utils/Trace.hpp"
#include "valdi_core/cpp/Utils/ValueUtils.hpp"
#include <fmt/ostream.h>

namespace Valdi {

STRING_CONST(loadStrategyFilePath, "load_strategy.json")

ModuleLoadStrategy::ModuleLoadStrategy(FlatMap<StringBox, ComponentLoadStrategy> strategies)
    : _strategies(std::move(strategies)) {}

ModuleLoadStrategy::~ModuleLoadStrategy() = default;

const ModuleLoadStrategy::ComponentLoadStrategy* ModuleLoadStrategy::getStrategyForComponentPath(
    const StringBox& componentPath) const {
    const auto& it = _strategies.find(componentPath);
    if (it == _strategies.end()) {
        return nullptr;
    }

    return &it->second;
}

Result<Ref<ModuleLoadStrategy>> ModuleLoadStrategy::parse(const Byte* data, size_t length) {
    auto result = jsonToValue(data, length);
    if (!result) {
        return result.moveError();
    }

    SimpleExceptionTracker exceptionTracker;
    auto map = result.value().checkedTo<Ref<ValueMap>>(exceptionTracker);
    if (!exceptionTracker) {
        return exceptionTracker.extractError();
    }

    FlatMap<StringBox, ComponentLoadStrategy> strategies;

    for (const auto& it : *map) {
        auto& strategy = strategies[it.first];

        auto moduleNamesValue = it.second.getMapValue("valdi_modules");
        auto moduleNamesArray = moduleNamesValue.checkedTo<Ref<ValueArray>>(exceptionTracker);
        if (!exceptionTracker) {
            return exceptionTracker.extractError();
        }

        for (const auto& moduleName : *moduleNamesArray) {
            strategy.valdiModules.emplace_back(moduleName.toStringBox());
        }
    }

    return makeShared<ModuleLoadStrategy>(std::move(strategies));
}

BundleInitializer::BundleInitializer(Bundle* bundle) : _bundle(strongSmallRef(bundle)), _lock(bundle->_mutex) {}
BundleInitializer::~BundleInitializer() {
    _bundle->_initialized.store(true, std::memory_order_relaxed);
}

void BundleInitializer::initWithLocalArchive(Ref<ValdiModuleArchive> decompressedBundle, bool hasRemoteAssets) {
    _bundle->_decompressedBundle = std::move(decompressedBundle);
    _bundle->_hasRemoteAssets = hasRemoteAssets;
}

void BundleInitializer::initWithRemoteArchive(bool hasRemoteAssets) {
    _bundle->_hasRemoteArchive = true;
    _bundle->_hasRemoteAssets = hasRemoteAssets;
}

const Ref<Bundle>& BundleInitializer::getBundle() const {
    return _bundle;
}

BundleResourceContent::BundleResourceContent() = default;
BundleResourceContent::BundleResourceContent(const BytesView& raw) : raw(raw) {}
BundleResourceContent::BundleResourceContent(const Ref<RefCountable>& processed) : processed(processed) {}
BundleResourceContent::~BundleResourceContent() = default;

Bundle::Bundle(StringBox name, ILogger& logger) : _name(std::move(name)), _logger(logger) {}

Bundle::~Bundle() = default;

BundleInitializer Bundle::prepareForInit() {
    return BundleInitializer(this);
}

bool Bundle::hasRemoteArchive() const {
    std::lock_guard<std::recursive_mutex> guard(_mutex);
    return _hasRemoteArchive;
}

bool Bundle::hasRemoteAssets() const {
    std::lock_guard<std::recursive_mutex> guard(_mutex);
    return _hasRemoteAssets;
}

Result<Void> Bundle::lockFreeLoadEntriesIfNeeded() {
    if (_loadedEntries) {
        return Void();
    }

    if (_decompressedBundle == nullptr) {
        return Valdi::Error(STRING_FORMAT("Bundle '{}' was not loaded", _name));
    }

    _loadedEntries = true;

    for (const auto& entryPath : _decompressedBundle->getAllEntryPaths()) {
        if (_entryByPath.find(entryPath) == _entryByPath.end()) {
            auto entry = _decompressedBundle->getEntry(entryPath);
            _entryByPath[entryPath] = BytesView(_decompressedBundle, entry->data, entry->size);
            _allEntryPaths.emplace_back(entryPath);
        }
    }

    return Void();
}

Result<BytesView> Bundle::getEntry(const StringBox& path) {
    std::lock_guard<std::recursive_mutex> guard(_mutex);

    auto archiveResult = lockFreeLoadEntriesIfNeeded();
    if (!archiveResult) {
        return archiveResult.moveError();
    }

    const auto& it = _entryByPath.find(path);
    if (it == _entryByPath.end()) {
        return Error(STRING_FORMAT("No item named '{}' in module '{}', available items are: {}",
                                   path,
                                   _name,
                                   StringBox::join(_allEntryPaths, ", ")));
    }

    return it->second;
}

bool Bundle::hasEntry(const StringBox& path) {
    std::lock_guard<std::recursive_mutex> guard(_mutex);
    lockFreeLoadEntriesIfNeeded();

    return _entryByPath.find(path) != _entryByPath.end();
}

void Bundle::setEntry(const StringBox& path, const BytesView& data) {
    std::lock_guard<std::recursive_mutex> guard(_mutex);

    const auto& it = _entryByPath.find(path);
    if (it == _entryByPath.end()) {
        _allEntryPaths.emplace_back(path);
    }
    _entryByPath[path] = data;
}

Result<JavaScriptFile> Bundle::getJs(const StringBox& jsPath) {
    std::lock_guard<std::recursive_mutex> guard(_mutex);
    const auto& it = _jsFilesByPath.find(jsPath);
    if (it != _jsFilesByPath.end()) {
        return it->second;
    }

    auto key = jsPath.append(".js");
    auto entryResult = getEntry(key);
    if (!entryResult) {
        return entryResult.moveError();
    }

    auto entry = entryResult.moveValue();

    auto jsFile = JavaScriptFile(entryResult.value(), StringBox::emptyString());

    _jsFilesByPath[jsPath] = jsFile;

    return jsFile;
}

std::vector<StringBox> Bundle::getAllEntryPaths() {
    std::lock_guard<std::recursive_mutex> guard(_mutex);
    lockFreeLoadEntriesIfNeeded();
    return _allEntryPaths;
}

std::vector<StringBox> Bundle::getAllJsPaths() {
    std::lock_guard<std::recursive_mutex> guard(_mutex);
    lockFreeLoadEntriesIfNeeded();

    std::vector<StringBox> output;
    for (const auto& entryPath : _allEntryPaths) {
        if (entryPath.hasSuffix(".js")) {
            auto withoutExtension = entryPath.substring(0, entryPath.length() - 3);
            output.emplace_back(withoutExtension);
        }
    }

    return output;
}

void Bundle::setJs(const StringBox& jsPath, const JavaScriptFile& jsFile) {
    std::lock_guard<std::recursive_mutex> guard(_mutex);
    _jsFilesByPath[jsPath] = jsFile;
}

Result<BundleResourceContent> Bundle::getResourceContent(const StringBox& path) {
    std::lock_guard<std::recursive_mutex> guard(_mutex);
    const auto& it = _resourceContentByPath.find(path);
    if (it != _resourceContentByPath.end()) {
        return it->second;
    }

    auto entryResult = getEntry(path);
    if (!entryResult) {
        return entryResult.moveError();
    }

    auto entry = entryResult.moveValue();
    return BundleResourceContent(entry);
}

void Bundle::setResourceContent(const StringBox& path, const BundleResourceContent& resourceContent) {
    std::lock_guard<std::recursive_mutex> guard(_mutex);
    _resourceContentByPath[path] = resourceContent;
}

Result<Ref<CSSDocument>> Bundle::getCSSDocument(const StringBox& path, AttributeIds& attributeIds) {
    std::lock_guard<std::recursive_mutex> guard(_mutex);

    const auto& it = _cssDocumentByPath.find(path);
    if (it != _cssDocumentByPath.end()) {
        return it->second;
    }

    auto entryResult = getEntry(path);
    if (!entryResult) {
        return entryResult.moveError();
    }

    auto entry = entryResult.moveValue();

    auto documentResult = CSSDocument::parse(ResourceId(_name, path), entry.data(), entry.size(), attributeIds);
    if (documentResult) {
        _cssDocumentByPath[path] = documentResult.value();
    }

    return documentResult;
}

void Bundle::setCSSDocument(const StringBox& path, const Ref<CSSDocument>& cssDocument) {
    std::lock_guard<std::recursive_mutex> guard(_mutex);
    _cssDocumentByPath[path] = cssDocument;
}

bool Bundle::hasModuleLoadStrategy() {
    return hasEntry(loadStrategyFilePath());
}

Result<Ref<ModuleLoadStrategy>> Bundle::getModuleLoadStrategy() {
    std::lock_guard<std::recursive_mutex> guard(_mutex);
    auto resourceContent = getResourceContent(loadStrategyFilePath());
    if (!resourceContent) {
        return resourceContent.moveError();
    }

    auto moduleLoadStragey = castOrNull<ModuleLoadStrategy>(resourceContent.value().processed);
    if (moduleLoadStragey != nullptr) {
        return moduleLoadStragey;
    }

    auto parsed = ModuleLoadStrategy::parse(resourceContent.value().raw.data(), resourceContent.value().raw.size());
    if (!parsed) {
        return parsed.moveError();
    }

    setResourceContent(loadStrategyFilePath(), BundleResourceContent(parsed.value()));

    return parsed.value();
}

template<typename T>
void removeUnusedItems(FlatMap<StringBox, T>& items) {
    auto it = items.begin();
    while (it != items.end()) {
        if (it->second.use_count() == 1) {
            it = items.erase(it);
        } else {
            ++it;
        }
    }
}

void Bundle::unloadUnusedResources() {
    std::lock_guard<std::recursive_mutex> guard(_mutex);
    removeUnusedItems(_cssDocumentByPath);
}

Result<Ref<AssetCatalog>> Bundle::getAssetCatalog(const StringBox& assetCatalogPath) {
    std::lock_guard<std::recursive_mutex> guard(_mutex);

    return lockFreeGetAssetCatalog(assetCatalogPath);
}

void Bundle::setAssetCatalog(const StringBox& assetCatalogPath, const Ref<AssetCatalog>& assetCatalog) {
    std::lock_guard<std::recursive_mutex> guard(_mutex);
    _assetCatalogByPath[assetCatalogPath] = assetCatalog;
}

Result<Ref<AssetCatalog>> Bundle::lockFreeGetAssetCatalog(const StringBox& assetCatalogPath) {
    const auto& it = _assetCatalogByPath.find(assetCatalogPath);
    if (it != _assetCatalogByPath.end()) {
        return it->second;
    }

    auto entry = getEntry(assetCatalogPath.append(".assetcatalog"));
    if (!entry) {
        return entry.moveError();
    }

    auto catalogResult = AssetCatalog::parse(entry.value().data(), entry.value().size());
    if (!catalogResult) {
        return catalogResult.moveError();
    }

    auto catalog = catalogResult.moveValue();

    _assetCatalogByPath[assetCatalogPath] = catalog;

    return catalog;
}

const StringBox& Bundle::getName() const {
    return _name;
}

/**
 * Retrieves the 'hash' file form the bundle
 *
 * Returns:
 * - A Result<StringBox> containing the 'hash' entry if it exists. Error if it does not.
 * Notes:
 * - Thread-safe. Using a recursive mutex to guard access.
 * - See BundleResourcesProcessor.swift for how the 'hash' file is generated.
 */
Result<StringBox> Bundle::getHash() {
    static auto kHashName = STRING_LITERAL("hash");
    std::lock_guard<std::recursive_mutex> guard(_mutex);

    auto entryResult = getResourceContent(kHashName);
    if (!entryResult) {
        return entryResult.moveError();
    }
    auto hash = StringCache::getGlobal().makeString(entryResult.value().raw.asStringView());
    return hash;
}

void Bundle::setLoadedArchiveIfNeeded(const Ref<ValdiModuleArchive>& archive) {
    std::lock_guard<std::recursive_mutex> guard(_mutex);
    if (_decompressedBundle == nullptr) {
        _decompressedBundle = archive;
    }
}

bool Bundle::hasLoadedArchive() const {
    std::lock_guard<std::recursive_mutex> guard(_mutex);
    return _decompressedBundle != nullptr;
}

bool Bundle::hasRemoteArchiveNeedingLoad() const {
    std::lock_guard<std::recursive_mutex> guard(_mutex);
    return _hasRemoteArchive && _decompressedBundle == nullptr;
}

std::unique_lock<std::recursive_mutex> Bundle::lock() const {
    return std::unique_lock<std::recursive_mutex>(_mutex);
}

JavaScriptFile::JavaScriptFile() = default;
JavaScriptFile::~JavaScriptFile() = default;
JavaScriptFile::JavaScriptFile(const BytesView& content, const StringBox& sourceMap)
    : content(content), sourceMap(sourceMap) {}

} // namespace Valdi
