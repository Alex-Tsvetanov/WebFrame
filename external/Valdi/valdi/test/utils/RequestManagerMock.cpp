//
//  RequestManagerMock.cpp
//  valdi-pc
//
//  Created by Simon Corsin on 9/27/19.
//

#include "RequestManagerMock.hpp"
#include "valdi_core/cpp/Utils/Format.hpp"
#include "valdi_core/cpp/Utils/LoggerUtils.hpp"
#include "valdi_core/cpp/Utils/StringCache.hpp"

namespace ValdiTest {

class RequestTask : public snap::valdi_core::Cancelable {
public:
    RequestTask(snap::valdi_core::HTTPRequest request,
                std::shared_ptr<snap::valdi_core::HTTPRequestManagerCompletion> completion)
        : _request(std::move(request)), _completion(std::move(completion)) {}

    void complete(const Result<snap::valdi_core::HTTPResponse>& result) {
        std::lock_guard<std::mutex> guard(_mutex);
        auto completion = std::move(_completion);
        if (completion == nullptr) {
            return;
        }
        _response = {result};

        if (result.success()) {
            completion->onComplete(result.value());
        } else {
            completion->onFail(result.error().toString());
        }
    }

    void cancel() override {
        std::lock_guard<std::mutex> guard(_mutex);
        _completion = nullptr;
    }

    const snap::valdi_core::HTTPRequest& getRequest() const {
        return _request;
    }

    std::optional<Result<snap::valdi_core::HTTPResponse>> getResponse() const {
        std::lock_guard<std::mutex> guard(_mutex);
        return _response;
    }

private:
    mutable std::mutex _mutex;
    snap::valdi_core::HTTPRequest _request;
    std::optional<Result<snap::valdi_core::HTTPResponse>> _response;
    std::shared_ptr<snap::valdi_core::HTTPRequestManagerCompletion> _completion;
};

RequestManagerMock::RequestManagerMock(ILogger& logger) : _logger(logger) {
    _queue = DispatchQueue::create(STRING_LITERAL("Valdi Request Manager"), ThreadQoSClass::ThreadQoSClassHigh);
}

RequestManagerMock::~RequestManagerMock() {
    _queue->fullTeardown();
}

void RequestManagerMock::setDisabled(bool disabled) {
    _disabled = disabled;
}

Result<snap::valdi_core::HTTPResponse> RequestManagerMock::getResponse(const snap::valdi_core::HTTPRequest& request) {
    if (_disabled) {
        return Valdi::Error("Request Manager is disabled");
    }

    for (const auto& mockedResponse : _mockedResponses) {
        const auto& expectedRequest = mockedResponse.first;

        if (expectedRequest.url == request.url && expectedRequest.method == request.method) {
            const auto& response = mockedResponse.second;

            return response;
        }
    }

    return Valdi::Error(STRING_LITERAL("No mocked response for given request"));
}

void RequestManagerMock::addMockedResponse(StringBox url, StringBox method, BytesView successBody) {
    auto request = snap::valdi_core::HTTPRequest(std::move(url), std::move(method), Value(), std::nullopt, 0);
    auto response = snap::valdi_core::HTTPResponse(200, Value(), {std::move(successBody)});
    addMockedResponse(request, response);
}

void RequestManagerMock::addMockedResponse(const snap::valdi_core::HTTPRequest& request,
                                           const snap::valdi_core::HTTPResponse& response) {
    addMockedResponse(request, Result<snap::valdi_core::HTTPResponse>(response));
}

void RequestManagerMock::addMockedResponse(const snap::valdi_core::HTTPRequest& request,
                                           const Result<snap::valdi_core::HTTPResponse>& response) {
    auto weakThis = weak_from_this();
    _queue->async([=]() {
        auto strongThis = weakThis.lock();
        if (strongThis != nullptr) {
            strongThis->_mockedResponses.emplace_back(request, response);
        }
    });
}

std::shared_ptr<snap::valdi_core::Cancelable> RequestManagerMock::performRequest(
    const snap::valdi_core::HTTPRequest& request,
    const std::shared_ptr<snap::valdi_core::HTTPRequestManagerCompletion>& completion) {
    auto task = Valdi::makeShared<RequestTask>(request, completion);

    auto weakThis = weak_from_this();
    _queue->async([=]() {
        auto strongThis = weakThis.lock();
        if (strongThis != nullptr) {
            strongThis->_tasks.emplace_back(task);

            auto result = strongThis->getResponse(request);
            task->complete(result);
        }
    });

    return task;
}

std::vector<Shared<RequestTask>> RequestManagerMock::getAllPerformedTasks() const {
    std::vector<Shared<RequestTask>> tasks;
    _queue->sync([&]() { tasks = _tasks; });
    return tasks;
}

} // namespace ValdiTest
