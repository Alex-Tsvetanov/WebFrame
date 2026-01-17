//
//  ManagedAsset.cpp
//  valdi
//
//  Created by Simon Corsin on 6/29/21.
//

#include "valdi/runtime/Resources/ManagedAsset.hpp"
#include "valdi/runtime/Resources/AssetLoader.hpp"
#include "valdi/runtime/Resources/AssetLoaderCompletion.hpp"
#include "valdi/runtime/Resources/AssetRequestPayloadCache.hpp"

namespace Valdi {

ManagedAsset::ManagedAsset() = default;
ManagedAsset::~ManagedAsset() = default;

Shared<ObservableAsset> ManagedAsset::getObservable() const {
    return _observable.lock();
}

void ManagedAsset::setObservable(const Shared<ObservableAsset>& observable) {
    _observable = observable;
}

AssetState ManagedAsset::getState() const {
    return _assetState;
}

void ManagedAsset::setState(AssetState assetState) {
    _assetState = assetState;
}

const Result<AssetLocation>& ManagedAsset::getResolvedAssetLocation() const {
    return _assetLocation;
}

void ManagedAsset::setResolvedAssetLocation(const Result<AssetLocation>& resolvedAssetLocation) {
    _assetLocation = resolvedAssetLocation;
}

Ref<AssetConsumer> ManagedAsset::addConsumer() {
    auto consumer = makeShared<AssetConsumer>();
    _consumers.emplace_back(consumer);
    return consumer;
}

void ManagedAsset::removeConsumer(const Ref<AssetConsumer>& consumer) {
    auto index = getConsumerIndex(consumer);
    if (index) {
        _consumers.erase(_consumers.begin() + index.value());
    }
}

std::optional<size_t> ManagedAsset::getConsumerIndex(const Ref<AssetConsumer>& consumer) const {
    for (size_t i = 0; i < _consumers.size(); i++) {
        if (_consumers[i] == consumer) {
            return {i};
        }
    }

    return std::nullopt;
}

const Ref<AssetConsumer>& ManagedAsset::getConsumer(size_t index) const {
    return _consumers[index];
}

size_t ManagedAsset::getConsumersSize() const {
    return _consumers.size();
}

bool ManagedAsset::hasConsumers() const {
    return getConsumersSize() > 0;
}

uint64_t ManagedAsset::getResolveId() const {
    return _resolveId;
}

void ManagedAsset::setResolveId(uint64_t resolveId) {
    _resolveId = resolveId;
}

Ref<AssetRequestPayloadCache> ManagedAsset::getPayloadCacheForAssetLoader(const Ref<AssetLoader>& assetLoader) {
    for (const auto& it : _payloadCache) {
        if (it->getAssetLoader() == assetLoader) {
            return it;
        }
    }

    auto cache = makeShared<AssetRequestPayloadCache>(assetLoader);
    _payloadCache.insert(_payloadCache.end(), cache);

    return cache;
}

void ManagedAsset::clearPayloadCache() {
    _payloadCache.clear();
}

std::ostream& operator<<(std::ostream& os, const AssetState& k) noexcept {
    switch (k) {
        case AssetStateInitial:
            os << "Initial";
            break;
        case AssetStateResolvingLocation:
            os << "ResolvingLocation";
            break;
        case AssetStateFailedPermanently:
            os << "FailedPermanently";
            break;
        case AssetStateFailedRetryable:
            os << "FailedRetryable";
            break;
        case AssetStateReady:
            os << "Ready";
            break;
    }

    return os;
}

} // namespace Valdi
