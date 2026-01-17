//
//  TestAsyncUtils.cpp
//  valdi-pc
//
//  Created by Simon Corsin on 9/26/19.
//

#include "TestAsyncUtils.hpp"
#include "valdi_core/cpp/Utils/Promise.hpp"

namespace ValdiTest {

Result<Value> waitForPromise(const Ref<Promise>& promise) {
    auto resultHolder = ResultHolder<Value>::make();

    promise->onComplete([completion = resultHolder->makeCompletion()](const auto& result) { completion(result); });

    return resultHolder->waitForResult();
}

} // namespace ValdiTest
