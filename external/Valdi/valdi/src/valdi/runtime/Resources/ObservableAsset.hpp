//
//  ObservableAsset.hpp
//  valdi
//
//  Created by Simon Corsin on 6/28/21.
//

#pragma once

#include "valdi/runtime/Resources/AssetKey.hpp"
#include "valdi_core/cpp/Resources/Asset.hpp"

namespace Valdi {

class AssetsManager;

/**
 ObservableAsset is the public API for an Asset object.
 It is held as a weak reference inside the AssetsManager.
 It does not actually hold its observers, they are held
 as consumers inside the private ManagedAsset instance.
 */
class ObservableAsset : public Asset {
public:
    ObservableAsset(const AssetKey& assetKey, Weak<AssetsManager> assetsManager);
    ~ObservableAsset() override;

    void setExpectedSize(int expectedWidth, int expectedHeight);

    bool canBeMeasured() const override;

    StringBox getIdentifier() override;

    double getWidth() const final;
    double getHeight() const final;

    void addLoadObserver(const std::shared_ptr<snap::valdi_core::AssetLoadObserver>& observer,
                         snap::valdi_core::AssetOutputType outputType,
                         int32_t preferredWidth,
                         int32_t preferredHeight,
                         const Valdi::Value& attachedData) override;

    void removeLoadObserver(const std::shared_ptr<snap::valdi_core::AssetLoadObserver>& observer) override;

    void updateLoadObserverPreferredSize(const std::shared_ptr<snap::valdi_core::AssetLoadObserver>& observer,
                                         int32_t preferredWidth,
                                         int32_t preferredHeight) override;

    std::optional<AssetLocation> getResolvedLocation() const override;

private:
    AssetKey _assetKey;
    Weak<AssetsManager> _assetsManager;
    int _expectedWidth = 0;
    int _expectedHeight = 0;

    Shared<AssetsManager> getAssetsManager() const;
};

} // namespace Valdi
