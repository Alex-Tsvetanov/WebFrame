//
//  ViewPreloader.cpp
//  valdi-ios
//
//  Created by Simon Corsin on 7/18/19.
//

#include "valdi/runtime/Views/ViewPreloader.hpp"
#include "utils/time/StopWatch.hpp"
#include "valdi/runtime/Utils/MainThreadManager.hpp"
#include "valdi_core/cpp/Threading/DispatchQueue.hpp"
#include "valdi_core/cpp/Utils/LoggerUtils.hpp"
#include "valdi_core/cpp/Utils/Trace.hpp"

namespace Valdi {

constexpr int64_t kMaxPreloadTimePerFrameMs = 10;

ViewPreloader::ViewPreloader(Ref<GlobalViewFactories> globalViewFactories,
                             MainThreadManager& mainThreadManager,
                             ILogger& logger)
    : _globalViewFactories(std::move(globalViewFactories)), _mainThreadManager(mainThreadManager), _logger(logger) {}

ViewPreloader::~ViewPreloader() = default;

void ViewPreloader::pausePreload() {
    std::lock_guard<std::mutex> lock(_mutex);
    _paused = true;
}

void ViewPreloader::resumePreload() {
    std::lock_guard<std::mutex> lock(_mutex);
    if (_paused) {
        _paused = false;
        schedulePreloadIfNeeded();
    }
}

void ViewPreloader::startPreload(const StringBox& className, size_t count) {
    std::lock_guard<std::mutex> lock(_mutex);
    const auto& it = _preloadCountByClassName.find(className);
    if (it == _preloadCountByClassName.end()) {
        _preloadCountByClassName[className] = count;
    } else {
        _preloadCountByClassName[className] = std::max(it->second, count);
    }

    schedulePreloadIfNeeded();
}

void ViewPreloader::schedulePreloadIfNeeded() {
    if (!_preloading && !_paused && !_preloadCountByClassName.empty()) {
        _preloading = true;
        schedulePreload();
    }
}

void ViewPreloader::stopPreload() {
    std::lock_guard<std::mutex> lock(_mutex);
    while (!_preloadCountByClassName.empty()) {
        _preloadCountByClassName.erase(_preloadCountByClassName.begin());
    }
}

void ViewPreloader::schedulePreload() {
    auto strongThis = strongRef(this);

    if (_workQueue != nullptr) {
        _workQueue->async([=]() { strongThis->preload(); });
    } else {
        _mainThreadManager.dispatch(nullptr, [=]() { strongThis->preload(); });
    }
}

void ViewPreloader::setWorkQueue(const Ref<DispatchQueue>& workQueue) {
    std::lock_guard<std::mutex> lock(_mutex);
    _workQueue = workQueue;
}

Ref<ViewFactory> ViewPreloader::dequeueNextViewFactoryToPreload() {
    std::lock_guard<std::mutex> lock(_mutex);

    while (true) {
        if (_preloadCountByClassName.empty() || _paused) {
            _preloading = false;
            return nullptr;
        }

        auto it = _preloadCountByClassName.begin();
        auto viewFactory = _globalViewFactories->getViewFactory(it->first);
        auto poolSize = viewFactory->getPoolSize();
        if (poolSize < it->second) {
            return viewFactory;
        } else {
            // Finished preloading this ViewFactory
            _preloadCountByClassName.erase(it);
        }
    }
}

bool ViewPreloader::preloadNext() {
    auto viewFactory = dequeueNextViewFactoryToPreload();
    if (viewFactory == nullptr) {
        return false;
    }

    auto view = viewFactory->createView(nullptr, nullptr, false);
    if (view != nullptr) {
        viewFactory->enqueueViewToPool(view);
    }

    return true;
}

void ViewPreloader::preload() {
    snap::utils::time::StopWatch sw;
    sw.start();

    VALDI_TRACE("Valdi.preloadViews");

    auto hasViewsToPreload = true;
    [[maybe_unused]] int preloadedViews = 0;
    while (hasViewsToPreload && sw.elapsedMs() < kMaxPreloadTimePerFrameMs) {
        if (preloadNext()) {
            preloadedViews++;
        } else {
            hasViewsToPreload = false;
        }
    }

    VALDI_DEBUG(_logger, "Preloaded {} views in {}", preloadedViews, sw.elapsed());

    if (hasViewsToPreload) {
        // We still have views to preload but we didn't have time to finish it yet
        schedulePreload();
    }
}

VALDI_CLASS_IMPL(ViewPreloader)

} // namespace Valdi
