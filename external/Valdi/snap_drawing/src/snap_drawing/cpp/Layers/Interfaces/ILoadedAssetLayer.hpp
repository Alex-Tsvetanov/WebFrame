//
//  ILoadedAssetLayer.hpp
//  snap_drawing
//
//  Created by Simon Corsin on 7/22/22.
//

#pragma once

#include "snap_drawing/cpp/Utils/Aliases.hpp"

namespace Valdi {
class LoadedAsset;
}

namespace snap::drawing {

class ILoadedAssetLayer {
public:
    virtual ~ILoadedAssetLayer() = default;

    virtual void onLoadedAssetChanged(const Ref<Valdi::LoadedAsset>& loadedAsset, bool shouldDrawFlipped) = 0;
};

} // namespace snap::drawing
