//
//  RemoteModulePrefetchTask.hpp
//  valdi-ios
//
//  Created by Simon Corsin on 8/27/19.
//

#pragma once

#include "valdi/runtime/Utils/AsyncGroup.hpp"
#include "valdi_core/cpp/Utils/Mutex.hpp"
#include "valdi_core/cpp/Utils/Result.hpp"
#include "valdi_core/cpp/Utils/StringBox.hpp"
#include <vector>

namespace Valdi {

class RemoteModulePrefetchTask : public std::enable_shared_from_this<RemoteModulePrefetchTask> {
public:
    void enter();

    void leave(Error error);
    void leave();

    template<typename T>
    void leave(const Result<T>& result) {
        if (result.success()) {
            leave();
        } else {
            leave(result.error());
        }
    }

    void notify(Function<void(Result<Void>)> completion);

private:
    std::vector<Error> _errors;
    Mutex _mutex;
    AsyncGroup _group;

    void onComplete(const Function<void(Result<Void>)>& completion);
};

} // namespace Valdi
