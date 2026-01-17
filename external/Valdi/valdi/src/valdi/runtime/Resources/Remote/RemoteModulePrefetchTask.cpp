//
//  RemoteModulePrefetchTask.cpp
//  valdi-ios
//
//  Created by Simon Corsin on 8/27/19.
//

#include "valdi/runtime/Resources/Remote/RemoteModulePrefetchTask.hpp"
#include "valdi_core/cpp/Utils/StringCache.hpp"
#include <fmt/format.h>
#include <fmt/ostream.h>

namespace Valdi {

void RemoteModulePrefetchTask::enter() {
    _group.enter();
}

void RemoteModulePrefetchTask::leave(Error error) {
    {
        std::lock_guard<Mutex> guard(_mutex);
        _errors.emplace_back(std::move(error));
    }

    _group.leave();
}

void RemoteModulePrefetchTask::leave() {
    _group.leave();
}

void RemoteModulePrefetchTask::onComplete(const Function<void(Result<Void>)>& completion) {
    Result<Void> result = Void();
    {
        std::lock_guard<Mutex> guard(_mutex);
        if (!_errors.empty()) {
            if (_errors.size() == 1) {
                result = Result<Void>(std::move(_errors[0]));
            } else {
                std::string errorMessage = fmt::format("Got {} errors:", _errors.size());

                for (const auto& error : _errors) {
                    errorMessage += "\n";
                    errorMessage += error.toString();
                }

                result = Error(StringCache::getGlobal().makeString(std::move(errorMessage)));
            }

            _errors.clear();
        }
    }

    completion(std::move(result));
}

void RemoteModulePrefetchTask::notify(Function<void(Result<Void>)> completion) {
    auto weakThis = weak_from_this();
    _group.notify([weakThis, completion = std::move(completion)]() {
        auto strongThis = weakThis.lock();
        if (strongThis != nullptr) {
            strongThis->onComplete(completion);
        }
    });
}

} // namespace Valdi
