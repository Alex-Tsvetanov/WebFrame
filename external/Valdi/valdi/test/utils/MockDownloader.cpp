//
//  MockDownloader.cpp
//  valdi-pc
//
//  Created by Ramzy Jaber on 4/25/22.
//

#include "MockDownloader.hpp"

#include "valdi_core/cpp/Utils/ContainerUtils.hpp"
#include "valdi_core/cpp/Utils/SimpleAtomicCancelable.hpp"
#include "valdi_test_utils.hpp"

using namespace snap::valdi;
using namespace Valdi;

Shared<snap::valdi_core::Cancelable> MockDownloader::downloadItem(
    const StringBox& url, Valdi::Function<void(const Result<BytesView>&)> completion) {
    _downloadRequests++;
    auto cancel = makeShared<SimpleAtomicCancelable>();

    auto weakThis = Valdi::weakRef(this);
    _workerQueue->async([url, cancel, weakThis, completion]() {
        if (cancel->wasCanceled()) {
            return;
        }

        auto strongThis = weakThis.lock();
        std::unique_lock<Mutex> guard(strongThis->_mutex);
        auto retval = strongThis->_responses[url];
        guard.unlock();
        completion(retval);
    });

    return cancel.toShared();
}

void MockDownloader::setDataResponse(const StringBox& url, const BytesView& response) {
    std::unique_lock<Mutex> guard(_mutex);
    _responses[url] = response;
}

void MockDownloader::setErrorResponse(const StringBox& url, const Error& response) {
    std::unique_lock<Mutex> guard(_mutex);
    _responses[url] = response;
}
