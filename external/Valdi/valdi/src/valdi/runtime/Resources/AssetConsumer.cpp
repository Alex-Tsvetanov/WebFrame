//
//  AssetConsumer.cpp
//  valdi
//
//  Created by Simon Corsin on 6/30/21.
//

#include "valdi/runtime/Resources/AssetConsumer.hpp"
#include "valdi/runtime/Context/Context.hpp"
#include "valdi/runtime/Resources/AssetLoaderCompletion.hpp"
#include "valdi_core/cpp/Resources/LoadedAsset.hpp"

namespace Valdi {

AssetConsumer::AssetConsumer() = default;
AssetConsumer::~AssetConsumer() {
    setLoadedAsset(Result<Ref<LoadedAsset>>());
}

AssetConsumerState AssetConsumer::getState() const {
    return _state;
}

void AssetConsumer::setState(AssetConsumerState state) {
    _state = state;
}

void AssetConsumer::setNotified(bool notified) {
    _notified = notified;
}

bool AssetConsumer::notified() const {
    return _notified;
}

void AssetConsumer::setPreferredWidth(int32_t preferredWidth) {
    _preferredWidth = preferredWidth;
}

int32_t AssetConsumer::getPreferredWidth() const {
    return _preferredWidth;
}

void AssetConsumer::setPreferredHeight(int32_t preferredHeight) {
    _preferredHeight = preferredHeight;
}

int32_t AssetConsumer::getPreferredHeight() const {
    return _preferredHeight;
}

void AssetConsumer::setObserver(const Shared<snap::valdi_core::AssetLoadObserver>& observer) {
    _observer = observer;
}

const Shared<snap::valdi_core::AssetLoadObserver>& AssetConsumer::getObserver() const {
    return _observer;
}

const Ref<Context>& AssetConsumer::getContext() const {
    return _context;
}

void AssetConsumer::setContext(const Ref<Context>& context) {
    _context = context;
}

void AssetConsumer::setLoadedAsset(const Result<Ref<LoadedAsset>>& loadedAsset) {
    auto previousLoadedAsset = std::move(_loadedAsset);

    _loadedAsset = loadedAsset;
}

const Result<Ref<LoadedAsset>>& AssetConsumer::getLoadedAsset() const {
    return _loadedAsset;
}

void AssetConsumer::setAssetLoaderCompletion(const Ref<AssetLoaderCompletion>& assetLoaderCompletion) {
    _assetLoaderCompletion = assetLoaderCompletion;
}

const Ref<AssetLoaderCompletion>& AssetConsumer::getAssetLoaderCompletion() const {
    return _assetLoaderCompletion;
}

void AssetConsumer::setOutputType(snap::valdi_core::AssetOutputType outputType) {
    _outputType = outputType;
}

snap::valdi_core::AssetOutputType AssetConsumer::getOutputType() const {
    return _outputType;
}

const Value& AssetConsumer::getAttachedData() const {
    return _attachedData;
}

void AssetConsumer::setAttachedData(const Value& attachedData) {
    _attachedData = attachedData;
}

} // namespace Valdi
