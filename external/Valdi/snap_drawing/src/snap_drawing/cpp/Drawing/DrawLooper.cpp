//
//  DrawLooper.cpp
//  snap_drawing-pc
//
//  Created by Simon Corsin on 1/21/22.
//

#include "snap_drawing/cpp/Drawing/DrawLooper.hpp"
#include "snap_drawing/cpp/Drawing/DrawOperation.hpp"
#include "utils/debugging/Assert.hpp"

#include "valdi_core/cpp/Utils/LoggerUtils.hpp"
#include "valdi_core/cpp/Utils/Trace.hpp"

namespace snap::drawing {

/**
 Max GPU cache size used by Skia.
 Skia uses a default of 256MB.
 Android computes the size based on the device screen size,
 for a Pixel XL the cache is 168MB.
 We use a hardcoded 128MB, which is lower than the default, while
 remaining fairly large already.
 */
static constexpr size_t kMaxGpuCacheSize = 128 * 1024 * 1024;
static constexpr size_t kMaxGpuCacheSizeInBackground = kMaxGpuCacheSize / 4;
static constexpr std::chrono::seconds kCacheExpirationSeconds = std::chrono::seconds(10);

class DrawLooperFrameCallback : public IFrameCallback {
public:
    explicit DrawLooperFrameCallback(DrawLooper* looper) : _drawLooper(Valdi::strongSmallRef(looper)) {}
    ~DrawLooperFrameCallback() override = default;

protected:
    Ref<DrawLooper> _drawLooper;
};

class DrawFramesCallback : public DrawLooperFrameCallback {
public:
    explicit DrawFramesCallback(DrawLooper* looper) : DrawLooperFrameCallback(looper) {}

    void onFrame(TimePoint time) override {
        _drawLooper->drawFrames(time);
    }
};

class ProcessFramesCallback : public DrawLooperFrameCallback {
public:
    explicit ProcessFramesCallback(DrawLooper* looper) : DrawLooperFrameCallback(looper) {}

    void onFrame(TimePoint time) override {
        _drawLooper->processFrames(time);
    }
};

class PerformCleanupCallback : public DrawLooperFrameCallback {
public:
    PerformCleanupCallback(DrawLooper* looper, DrawLooper::CleanUpMode cleanUpMode)
        : DrawLooperFrameCallback(looper), _cleanUpMode(cleanUpMode) {}

    void onFrame(TimePoint /*time*/) override {
        auto drawLock = _drawLooper->getDrawLock();
        _drawLooper->performCleanup(_cleanUpMode);
    }

private:
    DrawLooper::CleanUpMode _cleanUpMode;
};

class ConfigureCacheSizeCallback : public DrawLooperFrameCallback {
public:
    ConfigureCacheSizeCallback(DrawLooper* looper, const Ref<GraphicsContext>& graphicsContext)
        : DrawLooperFrameCallback(looper), _graphicsContext(graphicsContext) {}
    ~ConfigureCacheSizeCallback() override = default;

    void onFrame(TimePoint /*time*/) override {
        auto drawLock = _drawLooper->getDrawLock();
        _graphicsContext->setResourceCacheLimit(kMaxGpuCacheSize);
    }

private:
    Ref<GraphicsContext> _graphicsContext;
};

DrawLooper::DrawLooper(const Ref<IFrameScheduler>& frameScheduler, Valdi::ILogger& logger)
    : _frameScheduler(frameScheduler), _logger(logger) {}

DrawLooper::~DrawLooper() = default;

void DrawLooper::addLayerRoot(const Ref<LayerRoot>& layerRoot,
                              const Ref<SurfacePresenterManager>& surfacePresenterManager,
                              bool disallowSynchronousDraw) {
    DrawLooperEntryListener* listener = this;
    auto entry = Valdi::makeShared<DrawLooperEntry>(layerRoot, surfacePresenterManager, listener);
    entry->setDisallowSynchronousDraw(disallowSynchronousDraw);

    {
        auto lock = getEntriesLock();
        SC_ASSERT(getEntryForLayer(*layerRoot) == nullptr);

        _entries.emplace_back(entry);
        layerRoot->setListener(entry.get());
        scheduleProcessFrame(lock);
    }

    layerRoot->setChildNeedsDisplay();
}

void DrawLooper::removeLayerRoot(const Ref<LayerRoot>& layerRoot) {
    Ref<DrawLooperEntry> entry;
    auto drawLock = getDrawLock();
    auto lock = getEntriesLock();

    auto it = _entries.begin();
    while (it != _entries.end()) {
        if (it->get()->getLayerRoot() == layerRoot) {
            layerRoot->setListener(nullptr);
            entry = *it;
            _entries.erase(it);

            entry->removeSurfacePresenters();

            if (!_managedGraphicsContexts.empty()) {
                schedulePerformCleanup(DrawLooper::CleanUpMode::PostDraw);
            }

            return;
        }
        it++;
    }
}

void DrawLooper::setDrawableSurfaceOfLayerRootForPresenterId(LayerRoot& layerRoot,
                                                             SurfacePresenterId surfacePresenterId,
                                                             const Ref<DrawableSurface>& drawableSurface) {
    auto drawLock = getDrawLock();
    auto lock = getEntriesLock();
    auto entry = getEntryForLayer(layerRoot);
    if (entry == nullptr) {
        return;
    }

    entry->setDrawableSurfaceForPresenterId(surfacePresenterId, drawableSurface);

    auto drawState = entry->getDrawState();
    if (drawState.needsDraw) {
        if (drawState.prefersSynchronousDraw) {
            drawEntry(*entry);
        } else {
            scheduleDraw(lock);
        }
    }
}

bool DrawLooper::drawPlaneOfLayerRootForPresenterId(LayerRoot& layerRoot,
                                                    SurfacePresenterId surfacePresenterId,
                                                    DrawableSurfaceCanvas& canvas) {
    auto lock = getEntriesLock();
    auto entry = getEntryForLayer(layerRoot);
    if (entry == nullptr) {
        return false;
    }

    auto drawOperation = entry->makeDrawOperation(false);

    lock.unlock();

    return drawOperation->drawForPresenterId(surfacePresenterId, canvas);
}

void DrawLooper::setPresenterOfLayerRootNeedsRedraw(LayerRoot& layerRoot, SurfacePresenterId surfacePresenterId) {
    auto entriesLock = getEntriesLock();
    auto entry = mustGetEntryForLayer(layerRoot);
    entry->setPresenterNeedsDraw(surfacePresenterId);
    scheduleDraw(entriesLock);
}

void DrawLooper::appendManagedGraphicsContext(const Ref<GraphicsContext>& graphicsContext) {
    auto drawLock = getDrawLock();
    _managedGraphicsContexts.emplace_back(graphicsContext);
    _frameScheduler->onNextVSync(Valdi::makeShared<ConfigureCacheSizeCallback>(this, graphicsContext));
}

void DrawLooper::removeManagedGraphicsContext(const GraphicsContext* graphicsContext) {
    auto drawLock = getDrawLock();
    auto it = _managedGraphicsContexts.begin();
    while (it != _managedGraphicsContexts.end()) {
        if (it->get() == graphicsContext) {
            it = _managedGraphicsContexts.erase(it);
        } else {
            it++;
        }
    }
}

void DrawLooper::onApplicationEnteringBackground() {
    {
        auto entriesLock = getEntriesLock();
        _inBackground = true;
    }
    schedulePerformCleanup(CleanUpMode::EnteringBackground);
}

void DrawLooper::onApplicationEnteringForeground() {
    auto entriesLock = getEntriesLock();
    _inBackground = false;
    scheduleProcessFrame(entriesLock);
}

void DrawLooper::onApplicationIsInLowMemory() {
    schedulePerformCleanup(CleanUpMode::TrimMemory);
}

void DrawLooper::onNeedsProcessFrame(DrawLooperEntry& /*entry*/) {
    auto entriesLock = getEntriesLock();
    scheduleProcessFrame(entriesLock);
}

SurfacePresenterId DrawLooper::createSurfacePresenterId() {
    return ++_surfacePresenterIdSequence;
}

void DrawLooper::onDidDraw(DrawLooperEntry& entry,
                           const Ref<DisplayList>& displayList,
                           const CompositorPlaneList* planeList) {
    auto entriesLock = getEntriesLock();

    if (planeList != nullptr) {
        if (entry.surfacePresentersNeedUpdate(*planeList)) {
            // Release the lock before taking the draw lock, to follow the same
            // locking ordering used in drawFrames
            entriesLock.unlock();
            auto drawLock = getDrawLock();
            entriesLock.lock();

            {
                VALDI_TRACE("SnapDrawing.updateSurfacePresenters");
                entry.updateSurfacePresenters(*planeList);
            }
            entry.enqueueDisplayList(displayList);

            if (entry.getDrawState().needsSynchronousDraw()) {
                drawEntry(entry);

                if (!_managedGraphicsContexts.empty()) {
                    schedulePerformCleanup(CleanUpMode::PostDraw);
                }
            }

            return;
        }
    }

    entry.enqueueDisplayList(displayList);
}

void DrawLooper::drawEntry(DrawLooperEntry& entry) {
    DrawOperationsBatch batch;
    batch.emplace_back(entry.makeDrawOperation(true));

    drawOperationsBatch(batch);
}

DrawOperationsBatch DrawLooper::collectDrawOperations() {
    DrawOperationsBatch drawOperations;
    auto entriesLock = getEntriesLock();

    for (const auto& it : _entries) {
        if (it->getDrawState().needsDraw) {
            drawOperations.emplace_back(it->makeDrawOperation(true));
        }
    }

    return drawOperations;
}

void DrawLooper::drawOperationsBatch(const DrawOperationsBatch& drawOperations) {
    Valdi::SmallVector<GraphicsContext*, 2> graphicsContexts;

    for (const auto& drawOperation : drawOperations) {
        while (drawOperation->hasNext()) {
            auto result = drawOperation->drawNext();

            if (!result) {
                VALDI_ERROR(_logger, "Failed to draw Surface: {}", result.error());
            } else {
                auto* graphicsContext = result.value();
                if (graphicsContext != nullptr &&
                    std::find(graphicsContexts.begin(), graphicsContexts.end(), graphicsContext) ==
                        graphicsContexts.end()) {
                    graphicsContexts.emplace_back(graphicsContext);
                }
            }
        }
    }

    for (auto* graphicsContext : graphicsContexts) {
        graphicsContext->commit();
    }
}

void DrawLooper::drawFrames(TimePoint /*time*/) {
    auto drawLock = getDrawLock();
    auto drawOperations = collectDrawOperations();
    drawOperationsBatch(drawOperations);
    performCleanup(DrawLooper::CleanUpMode::PostDraw);

    auto entriesLock = getEntriesLock();
    _drawScheduled = false;

    for (const auto& it : _entries) {
        if (it->getDrawState().needsDraw) {
            scheduleDraw(entriesLock);
            return;
        }
    }
}

void DrawLooper::processFrames(TimePoint time) {
    _processingFrames = true;

    while (processFrameForNextLayer(time)) {
    }

    bool needScheduleDraw = false;
    bool needScheduleProcessFrame = false;
    auto entriesLock = getEntriesLock();

    for (const auto& it : _entries) {
        if (it->getLayerRoot()->needsProcessFrame()) {
            needScheduleProcessFrame = true;
        }
        if (it->getDrawState().needsDraw) {
            needScheduleDraw = true;
        }
    }

    _processingFrames = false;
    _processFrameScheduled = false;

    if (needScheduleProcessFrame) {
        doScheduleProcessFrame();
    }

    if (needScheduleDraw && !_drawScheduled && !_inBackground) {
        doScheduleDraw();
    }
}

void DrawLooper::performCleanup(DrawLooper::CleanUpMode cleanUpMode) {
    for (const auto& managedGraphicsContext : _managedGraphicsContexts) {
        switch (cleanUpMode) {
            case CleanUpMode::PostDraw:
                managedGraphicsContext->performCleanup(false, kCacheExpirationSeconds);
                break;
            case CleanUpMode::EnteringBackground:
                managedGraphicsContext->setResourceCacheLimit(kMaxGpuCacheSizeInBackground);
                managedGraphicsContext->performCleanup(true, std::chrono::seconds(0));
                managedGraphicsContext->setResourceCacheLimit(kMaxGpuCacheSize);
                break;
            case CleanUpMode::TrimMemory:
                managedGraphicsContext->setResourceCacheLimit(0);
                managedGraphicsContext->performCleanup(true, std::chrono::seconds(0));
                managedGraphicsContext->setResourceCacheLimit(kMaxGpuCacheSize);
                break;
        }
    }
}

bool DrawLooper::processFrameForNextLayer(TimePoint currentFrameTime) {
    auto entriesLock = getEntriesLock();
    for (const auto& it : _entries) {
        if (it->needsProcessFrameAtTime(currentFrameTime)) {
            auto layerRoot = it->getLayerRoot();
            entriesLock.unlock();

            layerRoot->processFrame(currentFrameTime);
            return true;
        }
    }
    return false;
}

void DrawLooper::scheduleProcessFrame(EntriesLock& entriesLock) {
    if (_processingFrames || _processFrameScheduled) {
        return;
    }
    doScheduleProcessFrame();
    entriesLock.unlock();
}

void DrawLooper::scheduleDraw(EntriesLock& entriesLock) {
    if (_drawScheduled) {
        return;
    }
    doScheduleDraw();
    entriesLock.unlock();
}

void DrawLooper::schedulePerformCleanup(CleanUpMode cleanUpMode) {
    _frameScheduler->onNextVSync(Valdi::makeShared<PerformCleanupCallback>(this, cleanUpMode));
}

void DrawLooper::doScheduleProcessFrame() {
    _processFrameScheduled = true;
    _frameScheduler->onMainThread(Valdi::makeShared<ProcessFramesCallback>(this));
}

void DrawLooper::doScheduleDraw() {
    _drawScheduled = true;
    _frameScheduler->onNextVSync(Valdi::makeShared<DrawFramesCallback>(this));
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
Ref<DrawLooperEntry> DrawLooper::getEntryForLayer(LayerRoot& layerRoot) const {
    auto* entry = dynamic_cast<DrawLooperEntry*>(layerRoot.getListener());
    return Valdi::strongSmallRef(entry);
}

Ref<DrawLooperEntry> DrawLooper::mustGetEntryForLayer(LayerRoot& layerRoot) const {
    auto entry = getEntryForLayer(layerRoot);
    SC_ASSERT(entry != nullptr);
    return entry;
}

EntriesLock DrawLooper::getEntriesLock() const {
    return EntriesLock(_mainThreadMutex);
}

DrawLock DrawLooper::getDrawLock() const {
    return DrawLock(_drawMutex);
}

} // namespace snap::drawing
