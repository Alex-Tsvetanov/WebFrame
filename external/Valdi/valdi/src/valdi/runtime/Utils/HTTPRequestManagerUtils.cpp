//
//  HTTPRequestManagerUtils.cpp
//  valdi-ios
//
//  Created by Simon Corsin on 8/30/19.
//

#include "valdi/runtime/Utils/HTTPRequestManagerUtils.hpp"
#include "valdi_core/cpp/Utils/StringCache.hpp"

namespace Valdi {

class HTTPRequestManagerCompletionWithFunction : public snap::valdi_core::HTTPRequestManagerCompletion {
public:
    explicit HTTPRequestManagerCompletionWithFunction(Function<void(Result<snap::valdi_core::HTTPResponse>)> function)
        : _function(std::move(function)) {}

    void onComplete(const snap::valdi_core::HTTPResponse& response) override {
        _function(response);
    }

    void onFail(const std::string& error) override {
        _function(Error(StringCache::getGlobal().makeString(error)));
    }

private:
    Function<void(Result<snap::valdi_core::HTTPResponse>)> _function;
};

std::shared_ptr<snap::valdi_core::HTTPRequestManagerCompletion> HTTPRequestManagerUtils::makeRequestCompletion(
    Function<void(Result<snap::valdi_core::HTTPResponse>)> function) { // NOLINT(performance-unnecessary-value-param)
    return Valdi::makeShared<HTTPRequestManagerCompletionWithFunction>(std::move(function));
}

} // namespace Valdi
