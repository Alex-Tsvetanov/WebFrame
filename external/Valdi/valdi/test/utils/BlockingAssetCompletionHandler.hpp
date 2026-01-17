//
//  BlockingAssetCompletionHandler.hpp
//  valdi-pc
//
//  Created by Ramzy Jaber on 4/25/22.
//

#pragma once

#include "TestAsyncUtils.hpp"
#include "valdi/runtime/Resources/AssetLoaderCompletion.hpp"
#include "valdi_core/cpp/Resources/LoadedAsset.hpp"

namespace ValdiTest {
class BlockingAssetCompletionHandler : public Valdi::AssetLoaderCompletion {
public:
    BlockingAssetCompletionHandler();

    ~BlockingAssetCompletionHandler() override = default;

    void onLoadComplete(const Valdi::Result<Valdi::Ref<Valdi::LoadedAsset>>& result) override;

    Valdi::Result<Valdi::Ref<Valdi::LoadedAsset>> getResult();

private:
    Valdi::Ref<ResultHolder<Valdi::Ref<LoadedAsset>>> _resultHolder;
    snap::CopyableFunction<void(Result<Valdi::Ref<LoadedAsset>>)> _completion;
    Valdi::Result<Valdi::Ref<LoadedAsset>> _result;
};

} // namespace ValdiTest
