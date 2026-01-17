//
//  DirectionalAsset.cpp
//  valdi
//
//  Created by Simon Corsin on 5/1/23.
//

#include "valdi/runtime/Resources/DirectionalAsset.hpp"

namespace Valdi {

DirectionalAsset::DirectionalAsset(const Ref<Asset>& ltrAsset, const Ref<Asset>& rtlAsset)
    : _ltrAsset(ltrAsset), _rtlAsset(rtlAsset) {}
DirectionalAsset::~DirectionalAsset() = default;

bool DirectionalAsset::canBeMeasured() const {
    return _ltrAsset->canBeMeasured();
}

StringBox DirectionalAsset::getIdentifier() {
    return _ltrAsset->getIdentifier();
}

double DirectionalAsset::getWidth() const {
    return _ltrAsset->getWidth();
}

double DirectionalAsset::getHeight() const {
    return _ltrAsset->getHeight();
}

Ref<Asset> DirectionalAsset::withDirection(bool rightToLeft) {
    return rightToLeft ? _rtlAsset : _ltrAsset;
}

void DirectionalAsset::addLoadObserver(const std::shared_ptr<snap::valdi_core::AssetLoadObserver>& observer,
                                       snap::valdi_core::AssetOutputType outputType,
                                       int32_t preferredWidth,
                                       int32_t preferredHeight,
                                       const Valdi::Value& associatedData) {
    _ltrAsset->addLoadObserver(observer, outputType, preferredWidth, preferredHeight, associatedData);
}

void DirectionalAsset::removeLoadObserver(const std::shared_ptr<snap::valdi_core::AssetLoadObserver>& observer) {
    _ltrAsset->removeLoadObserver(observer);
}

void DirectionalAsset::updateLoadObserverPreferredSize(
    const std::shared_ptr<snap::valdi_core::AssetLoadObserver>& observer,
    int32_t preferredWidth,
    int32_t preferredHeight) {
    _ltrAsset->updateLoadObserverPreferredSize(observer, preferredWidth, preferredHeight);
}

std::optional<AssetLocation> DirectionalAsset::getResolvedLocation() const {
    return _ltrAsset->getResolvedLocation();
}

} // namespace Valdi
