//
//  RequestManagerMock.hpp
//  valdi-pc
//
//  Created by Simon Corsin on 9/27/19.
//

#pragma once

#include "valdi_core/Cancelable.hpp"
#include "valdi_core/HTTPRequest.hpp"
#include "valdi_core/HTTPRequestManager.hpp"
#include "valdi_core/HTTPRequestManagerCompletion.hpp"
#include "valdi_core/HTTPResponse.hpp"

#include "valdi_core/cpp/Interfaces/ILogger.hpp"
#include "valdi_core/cpp/Threading/DispatchQueue.hpp"
#include "valdi_core/cpp/Utils/Bytes.hpp"
#include "valdi_core/cpp/Utils/Result.hpp"

using namespace Valdi;

namespace ValdiTest {

class RequestTask;

class RequestManagerMock : public snap::valdi_core::HTTPRequestManager,
                           public std::enable_shared_from_this<RequestManagerMock> {
public:
    RequestManagerMock(ILogger& logger);
    ~RequestManagerMock();

    Result<snap::valdi_core::HTTPResponse> getResponse(const snap::valdi_core::HTTPRequest& request);

    void addMockedResponse(StringBox url, StringBox method, BytesView successBody);

    void addMockedResponse(const snap::valdi_core::HTTPRequest& request,
                           const snap::valdi_core::HTTPResponse& response);

    void addMockedResponse(const snap::valdi_core::HTTPRequest& request,
                           const Result<snap::valdi_core::HTTPResponse>& response);

    void setDisabled(bool disabled);

    std::shared_ptr<snap::valdi_core::Cancelable> performRequest(
        const snap::valdi_core::HTTPRequest& request,
        const std::shared_ptr<snap::valdi_core::HTTPRequestManagerCompletion>& completion) override;

    std::vector<Shared<RequestTask>> getAllPerformedTasks() const;

private:
    Ref<DispatchQueue> _queue;
    std::vector<std::pair<snap::valdi_core::HTTPRequest, Result<snap::valdi_core::HTTPResponse>>> _mockedResponses;
    [[maybe_unused]] ILogger& _logger;
    std::vector<Shared<RequestTask>> _tasks;
    bool _disabled = false;
};

} // namespace ValdiTest
