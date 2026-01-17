//
//  AssetLoaderFactory.hpp
//  valdi-ios
//
//  Created by Simon Corsin on 7/18/22.
//

#pragma once

#include "valdi/runtime/Resources/AssetLoader.hpp"
#include "valdi_core/cpp/Utils/Shared.hpp"

namespace Valdi {

class IRemoteDownloader;

class AssetLoaderFactory : public SharedPtrRefCountable {
public:
    virtual snap::valdi_core::AssetOutputType getOutputType() const = 0;

    virtual Ref<AssetLoader> createAssetLoader(const std::vector<StringBox>& urlSchemes,
                                               const Ref<IRemoteDownloader>& downloader) = 0;
};

} // namespace Valdi
