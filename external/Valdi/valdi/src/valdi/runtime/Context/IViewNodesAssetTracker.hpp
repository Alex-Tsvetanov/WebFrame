#pragma once

#include "valdi/runtime/Context/RawViewNodeId.hpp"
#include "valdi_core/cpp/Utils/Shared.hpp"
#include "valdi_core/cpp/Utils/StringBox.hpp"

#include <optional>

namespace Valdi {
class Asset;

class IViewNodesAssetTracker : public SimpleRefCountable {
public:
    /**
     * Called whenever the ViewNode has began requesting a loaded asset representing of the given asset.
     */
    virtual void onBeganRequestingLoadedAsset(RawViewNodeId viewNodeId, const Ref<Asset>& asset) = 0;

    /**
     * Called whenever the ViewNode has stopped requesting a loaded asset representing of the given asset.
     */
    virtual void onEndRequestingLoadedAsset(RawViewNodeId viewNodeId, const Ref<Asset>& asset) = 0;

    /**
     * Called whenever a previously requested loaded asset representation has changed.
     */
    virtual void onLoadedAssetChanged(RawViewNodeId viewNodeId,
                                      const Ref<Asset>& asset,
                                      const std::optional<Valdi::StringBox>& error) = 0;
};

} // namespace Valdi