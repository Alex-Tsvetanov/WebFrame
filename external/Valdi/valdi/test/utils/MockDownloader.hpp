//
//  MockDownloader.hpp
//  valdi-pc
//
//  Created by Ramzy Jaber on 4/25/22.
//

#pragma once

#include "valdi/runtime/Interfaces/IRemoteDownloader.hpp"
#include "valdi_core/cpp/Threading/DispatchQueue.hpp"
#include "valdi_core/cpp/Utils/Bytes.hpp"
#include "valdi_core/cpp/Utils/FlatMap.hpp"
#include "valdi_core/cpp/Utils/Function.hpp"
#include "valdi_core/cpp/Utils/Mutex.hpp"
#include "valdi_core/cpp/Utils/Shared.hpp"
#include "valdi_core/cpp/Utils/StringBox.hpp"
#include <atomic>

class MockDownloader : public Valdi::IRemoteDownloader {
public:
    MockDownloader(const Valdi::Ref<Valdi::DispatchQueue>& queue) : _workerQueue(queue) {};
    ~MockDownloader() override = default;

    Valdi::Shared<snap::valdi_core::Cancelable> downloadItem(
        const Valdi::StringBox& url, Valdi::Function<void(const Valdi::Result<Valdi::BytesView>&)> completion) override;

    void setDataResponse(const Valdi::StringBox& url, const Valdi::BytesView& response);
    void setErrorResponse(const Valdi::StringBox& url, const Valdi::Error& response);

    uint64_t getDownloadRequests() {
        return _downloadRequests.load();
    };

private:
    std::atomic<uint32_t> _downloadRequests = 0;
    Valdi::FlatMap<Valdi::StringBox, Valdi::Result<Valdi::BytesView>> _responses;
    mutable Valdi::Mutex _mutex;
    Valdi::Ref<Valdi::DispatchQueue> _workerQueue;
};
