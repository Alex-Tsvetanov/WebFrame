//
//  DirectionalAsset.hpp
//  valdi
//
//  Created by Simon Corsin on 5/1/23.
//

#pragma once

#include "valdi/runtime/Resources/AssetKey.hpp"
#include "valdi_core/cpp/Resources/Asset.hpp"

namespace Valdi {

/**
 A DirectionalAsset holds an LTR and RTL asset and can use one asset or the
 other based on whether it's used in a LTR or RTL environment.
 */
class DirectionalAsset : public Asset {
public:
    DirectionalAsset(const Ref<Asset>& ltrAsset, const Ref<Asset>& rtlAsset);
    ~DirectionalAsset() override;

    bool canBeMeasured() const final;

    StringBox getIdentifier() final;

    double getWidth() const final;
    double getHeight() const final;

    Ref<Asset> withDirection(bool rightToLeft) final;

    void addLoadObserver(const std::shared_ptr<snap::valdi_core::AssetLoadObserver>& observer,
                         snap::valdi_core::AssetOutputType outputType,
                         int32_t preferredWidth,
                         int32_t preferredHeight,
                         const Valdi::Value& associatedData) final;

    void removeLoadObserver(const std::shared_ptr<snap::valdi_core::AssetLoadObserver>& observer) final;

    void updateLoadObserverPreferredSize(const std::shared_ptr<snap::valdi_core::AssetLoadObserver>& observer,
                                         int32_t preferredWidth,
                                         int32_t preferredHeight) final;

    std::optional<AssetLocation> getResolvedLocation() const final;

private:
    Ref<Asset> _ltrAsset;
    Ref<Asset> _rtlAsset;
};

} // namespace Valdi
