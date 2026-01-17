//
//  BlockingAssetCompletionHandler.cpp
//  valdi-pc
//
//  Created by Ramzy Jaber on 4/25/22.
//

#include "BlockingAssetCompletionHandler.hpp"

#include "valdi_test_utils.hpp"

using namespace Valdi;

namespace ValdiTest {

BlockingAssetCompletionHandler::BlockingAssetCompletionHandler()
    : _resultHolder(ResultHolder<Ref<LoadedAsset>>::make()) {
    _completion = _resultHolder->makeCompletion();
};

void BlockingAssetCompletionHandler::onLoadComplete(const Result<Ref<LoadedAsset>>& result) {
    _completion(result);
}

Result<Ref<LoadedAsset>> BlockingAssetCompletionHandler::getResult() {
    return _resultHolder->waitForResult();
}
} // namespace ValdiTest
