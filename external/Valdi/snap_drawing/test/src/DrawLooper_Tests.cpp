#include <gtest/gtest.h>

#include "snap_drawing/cpp/Animations/Animation.hpp"
#include "snap_drawing/cpp/Drawing/DrawLooper.hpp"
#include "snap_drawing/cpp/Layers/ExternalLayer.hpp"
#include "snap_drawing/cpp/Layers/Layer.hpp"
#include "snap_drawing/cpp/Layers/LayerRoot.hpp"

#include "valdi_core/cpp/Utils/ByteBuffer.hpp"
#include "valdi_core/cpp/Utils/ConsoleLogger.hpp"

#include "TestBitmap.hpp"
#include "snap_drawing/cpp/Drawing/GraphicsContext/BitmapGraphicsContext.hpp"
#include "snap_drawing/cpp/Drawing/Surface/SurfacePresenterManager.hpp"

#include <deque>

using namespace Valdi;

namespace snap::drawing {

class TestFrameScheduler : public IFrameScheduler {
public:
    void advanceTime(TimeInterval duration) {
        _currentTime += Duration(duration);
    }

    bool runNextVSyncCallback() {
        return runNextCallback(_vsyncCallbacks);
    }

    bool runNextMainThreadCallback() {
        return runNextCallback(_mainThreadCallbacks);
    }

    size_t getMainThreadCallbacksSize() const {
        return _mainThreadCallbacks.size();
    }

    void onNextVSync(const Ref<IFrameCallback>& callback) override {
        _vsyncCallbacks.emplace_back(callback);
    }

    void onMainThread(const Ref<IFrameCallback>& callback) override {
        _mainThreadCallbacks.emplace_back(callback);
    }

private:
    std::deque<Ref<IFrameCallback>> _vsyncCallbacks;
    std::deque<Ref<IFrameCallback>> _mainThreadCallbacks;
    TimePoint _currentTime = TimePoint(0.0);

    bool runNextCallback(std::deque<Ref<IFrameCallback>>& callbacks) {
        if (callbacks.empty()) {
            return false;
        }

        auto cb = std::move(callbacks.front());
        callbacks.pop_front();

        cb->onFrame(_currentTime);

        return true;
    }
};

struct TestExternalSurface : public ExternalSurface {
    std::optional<ExternalSurfacePresenterState> presenterState;
};

std::ostream& operator<<(std::ostream& stream, const Surface& surface) {
    const auto* surfacePtr = &surface;

    if (dynamic_cast<const ExternalSurface*>(surfacePtr) != nullptr) {
        stream << "ExternalSurface ";
    } else if (dynamic_cast<const DrawableSurface*>(surfacePtr) != nullptr) {
        stream << "DrawableSurface ";
    } else {
        stream << "Surface ";
    }

    return stream << reinterpret_cast<const void*>(surfacePtr);
}

std::ostream& operator<<(std::ostream& stream, const Ref<Surface>& surface) {
    return stream << *surface.get();
}

std::ostream& operator<<(std::ostream& stream, const Ref<TestExternalSurface>& surface) {
    return stream << *surface.get();
}

struct SurfacePresenterEntry {
    Ref<Surface> surface;
    Ref<TestBitmap> bitmap;
};

struct PerformCleanupRequest {
    bool shouldPurgeScratchResources;
    std::chrono::seconds secondsNotUsed;
};

class TestGraphicsContext : public BitmapGraphicsContext {
public:
    void clearRequests() {
        std::lock_guard<Valdi::Mutex> guard(_mutex);
        _resourceCacheLimitBytesRequests.clear();
        _performCleanupRequests.clear();
    }

    std::vector<size_t> getResourceCacheLimitRequests() const {
        std::lock_guard<Valdi::Mutex> guard(_mutex);
        return _resourceCacheLimitBytesRequests;
    }

    std::vector<PerformCleanupRequest> getPerformCleanupRequests() const {
        std::lock_guard<Valdi::Mutex> guard(_mutex);
        return _performCleanupRequests;
    }

    void setResourceCacheLimit(size_t resourceCacheLimitBytes) override {
        std::lock_guard<Valdi::Mutex> guard(_mutex);
        _resourceCacheLimitBytesRequests.emplace_back(resourceCacheLimitBytes);
    }

    void performCleanup(bool shouldPurgeScratchResources, std::chrono::seconds secondsNotUsed) override {
        PerformCleanupRequest cleanUpRequest;
        cleanUpRequest.shouldPurgeScratchResources = shouldPurgeScratchResources;
        cleanUpRequest.secondsNotUsed = secondsNotUsed;

        std::lock_guard<Valdi::Mutex> guard(_mutex);
        _performCleanupRequests.emplace_back(cleanUpRequest);
    }

private:
    mutable Valdi::Mutex _mutex;
    std::vector<size_t> _resourceCacheLimitBytesRequests;
    std::vector<PerformCleanupRequest> _performCleanupRequests;
};

class TestSurfacePresenterManager : public SurfacePresenterManager {
public:
    TestSurfacePresenterManager() : _graphicsContext(makeShared<TestGraphicsContext>()) {}

    ~TestSurfacePresenterManager() override = default;

    const Ref<TestGraphicsContext>& getGraphicsContext() const {
        return _graphicsContext;
    }

    virtual Ref<TestBitmap> createBitmap() {
        return makeShared<SinglePixelBitmap>();
    }

    Ref<DrawableSurface> createDrawableSurface(const Ref<TestBitmap>& bitmap) {
        return _graphicsContext->createBitmapSurface(bitmap);
    }

    Ref<DrawableSurface> createPresenterWithDrawableSurface(SurfacePresenterId id, size_t zIndex) override {
        auto bitmap = createBitmap();
        auto surface = createDrawableSurface(bitmap);

        auto& entry = _surfacePresenters[id];
        entry.surface = surface;
        entry.bitmap = bitmap;

        insertSurface(surface, zIndex);

        return surface;
    }

    void createPresenterForExternalSurface(SurfacePresenterId id,
                                           size_t zIndex,
                                           ExternalSurface& externalSurface) override {
        auto surface = Ref<Surface>(&externalSurface);
        _surfacePresenters[id].surface = surface;

        insertSurface(surface, zIndex);
    }

    void setSurfacePresenterZIndex(SurfacePresenterId id, size_t zIndex) override {
        auto surface = _surfacePresenters[id].surface;
        SC_ASSERT(surface != nullptr);

        tryRemoveSurface(*surface);
        insertSurface(surface, zIndex);
    }

    void setExternalSurfacePresenterState(SurfacePresenterId id,
                                          const ExternalSurfacePresenterState& presenterState,
                                          const ExternalSurfacePresenterState* previousPresenterState) override {
        auto surface = Valdi::castOrNull<TestExternalSurface>(_surfacePresenters[id].surface);
        SC_ASSERT(surface != nullptr);

        surface->presenterState = presenterState;
    }

    void removeSurfacePresenter(SurfacePresenterId id) override {
        auto surface = _surfacePresenters[id].surface;
        SC_ASSERT(surface != nullptr);

        _surfacePresenters.erase(id);

        removeSurface(*surface);
    }

    std::optional<SurfacePresenterEntry> getPresenter(size_t zIndex) const {
        if (zIndex >= _surfaces.size()) {
            return std::nullopt;
        }

        const auto& surface = _surfaces[zIndex];

        for (const auto& it : _surfacePresenters) {
            if (it.second.surface == surface) {
                return {it.second};
            }
        }

        return std::nullopt;
    }

    Ref<TestBitmap> getSurfaceBitmap(size_t zIndex) {
        auto presenter = getPresenter(zIndex);
        if (!presenter) {
            return nullptr;
        }
        return presenter.value().bitmap;
    }

    Ref<SinglePixelBitmap> getSurfaceSinglePixelBitmap(size_t zIndex) {
        return Valdi::castOrNull<SinglePixelBitmap>(getSurfaceBitmap(zIndex));
    }

    void removeSurface(Surface& surface) {
        auto removed = tryRemoveSurface(surface);
        SC_ASSERT(removed);
    }

    const std::vector<Ref<Surface>>& getSurfaces() const {
        return _surfaces;
    }

private:
    Ref<TestGraphicsContext> _graphicsContext;
    Valdi::FlatMap<SurfacePresenterId, SurfacePresenterEntry> _surfacePresenters;
    std::vector<Ref<Surface>> _surfaces;

    bool tryRemoveSurface(Surface& surface) {
        auto it = _surfaces.begin();
        while (it != _surfaces.end()) {
            if (it->get() == &surface) {
                _surfaces.erase(it);
                return true;
            }
            it++;
        }

        return false;
    }

    void insertSurface(const Ref<Surface>& surface, size_t zIndex) {
        SC_ASSERT(zIndex <= _surfaces.size());
        SC_ASSERT(!tryRemoveSurface(*surface));
        _surfaces.insert(_surfaces.begin() + zIndex, surface);
    }
};

class TestDelayedSurfacePresenterManager : public TestSurfacePresenterManager {
    Ref<DrawableSurface> createPresenterWithDrawableSurface(SurfacePresenterId id, size_t zIndex) override {
        TestSurfacePresenterManager::createPresenterWithDrawableSurface(id, zIndex);
        return nullptr;
    }
};

class Test4PixelsBitmapSurfacePresenterManager : public TestSurfacePresenterManager {
public:
    Ref<TestBitmap> createBitmap() override {
        return makeShared<TestBitmap>(4, 4);
    }
};

struct TestDrawLooperEntryListener : public DrawLooperEntryListener {
    SurfacePresenterId presenterIdSequence = 0;

    SurfacePresenterId createSurfacePresenterId() override {
        return ++presenterIdSequence;
    }

    void onNeedsProcessFrame(DrawLooperEntry& entry) override {}

    void onDidDraw(DrawLooperEntry& entry,
                   const Ref<DisplayList>& displayList,
                   const CompositorPlaneList* planeList) override {}
};

struct DrawLooperTestContainer {
    Ref<Resources> resources;
    Ref<TestFrameScheduler> frameScheduler;
    Ref<DrawLooper> drawLooper;
    Ref<LayerRoot> layerRoot;

    DrawLooperTestContainer() {
        resources = makeShared<Resources>(nullptr, 1.0f, ConsoleLogger::getLogger());

        frameScheduler = makeShared<TestFrameScheduler>();
        drawLooper = makeShared<DrawLooper>(frameScheduler, resources->getLogger());

        layerRoot = makeLayerRoot();
    }

    Ref<TestSurfacePresenterManager> addLayerRootToLooper(const Ref<LayerRoot>& layerRoot) {
        auto surfacePresenterManager = makeShared<TestSurfacePresenterManager>();
        drawLooper->addLayerRoot(layerRoot, surfacePresenterManager, false);
        return surfacePresenterManager;
    }

    Ref<LayerRoot> makeLayerRoot() const {
        auto contentLayer = makeShared<Layer>(resources);
        contentLayer->setBackgroundColor(Color::black());
        auto layerRoot = makeShared<LayerRoot>(resources);
        layerRoot->setContentLayer(contentLayer, ContentLayerSizingModeMatchSize);
        layerRoot->setSize(Size::make(1.0f, 1.0f), 1.0);
        return layerRoot;
    }
};

TEST(DrawLooper, addRemoveListenerOnLayerRoot) {
    DrawLooperTestContainer container;

    ASSERT_FALSE(container.layerRoot->getListener() != nullptr);

    container.addLayerRootToLooper(container.layerRoot);

    ASSERT_TRUE(container.layerRoot->getListener() != nullptr);

    container.drawLooper->removeLayerRoot(container.layerRoot);

    ASSERT_FALSE(container.layerRoot->getListener() != nullptr);
}

TEST(DrawLooper, schedulesProcessFramesOnLayerUpdates) {
    DrawLooperTestContainer container;

    ASSERT_FALSE(container.frameScheduler->runNextVSyncCallback());
    ASSERT_FALSE(container.frameScheduler->runNextMainThreadCallback());

    container.addLayerRootToLooper(container.layerRoot);

    ASSERT_TRUE(container.layerRoot->needsProcessFrame());

    ASSERT_TRUE(container.frameScheduler->runNextMainThreadCallback());

    ASSERT_FALSE(container.layerRoot->needsProcessFrame());
    ASSERT_FALSE(container.frameScheduler->runNextMainThreadCallback());

    container.layerRoot->getContentLayer()->setBackgroundColor(Color::white());
    container.frameScheduler->advanceTime(1.0);

    ASSERT_TRUE(container.layerRoot->needsProcessFrame());
    ASSERT_TRUE(container.frameScheduler->runNextMainThreadCallback());

    ASSERT_FALSE(container.layerRoot->needsProcessFrame());
    ASSERT_FALSE(container.frameScheduler->runNextMainThreadCallback());
}

TEST(DrawLooper, processesFrameAgainWhenSwitchingContentLayer) {
    DrawLooperTestContainer container;

    ASSERT_FALSE(container.frameScheduler->runNextVSyncCallback());
    ASSERT_FALSE(container.frameScheduler->runNextMainThreadCallback());

    container.addLayerRootToLooper(container.layerRoot);

    ASSERT_TRUE(container.layerRoot->needsProcessFrame());

    ASSERT_TRUE(container.frameScheduler->runNextMainThreadCallback());

    ASSERT_FALSE(container.layerRoot->needsProcessFrame());
    ASSERT_FALSE(container.frameScheduler->runNextMainThreadCallback());

    auto contentLayer = container.layerRoot->getContentLayer();
    container.layerRoot->setContentLayer(nullptr, ContentLayerSizingModeMatchSize);

    ASSERT_TRUE(container.layerRoot->needsProcessFrame());

    container.frameScheduler->advanceTime(1.0);

    ASSERT_TRUE(container.frameScheduler->runNextMainThreadCallback());

    ASSERT_FALSE(container.layerRoot->needsProcessFrame());
    ASSERT_FALSE(container.frameScheduler->runNextMainThreadCallback());

    container.layerRoot->setContentLayer(contentLayer, ContentLayerSizingModeMatchSize);
    container.frameScheduler->advanceTime(1.0);

    ASSERT_TRUE(container.layerRoot->needsProcessFrame());
    ASSERT_TRUE(container.frameScheduler->runNextMainThreadCallback());

    ASSERT_FALSE(container.layerRoot->needsProcessFrame());
    ASSERT_FALSE(container.frameScheduler->runNextMainThreadCallback());
}

TEST(DrawLooper, scheduleProcessFramesOnceForAllLayers) {
    DrawLooperTestContainer container;

    ASSERT_EQ(static_cast<size_t>(0), container.frameScheduler->getMainThreadCallbacksSize());

    container.addLayerRootToLooper(container.layerRoot);
    container.layerRoot->getContentLayer()->setBackgroundColor(Color::white());

    auto newLayerRoot = container.makeLayerRoot();
    container.addLayerRootToLooper(newLayerRoot);

    ASSERT_EQ(static_cast<size_t>(1), container.frameScheduler->getMainThreadCallbacksSize());

    ASSERT_TRUE(container.layerRoot->needsProcessFrame());
    ASSERT_TRUE(newLayerRoot->needsProcessFrame());

    ASSERT_TRUE(container.frameScheduler->runNextMainThreadCallback());

    ASSERT_FALSE(container.layerRoot->needsProcessFrame());
    ASSERT_FALSE(newLayerRoot->needsProcessFrame());
    ASSERT_FALSE(container.frameScheduler->runNextMainThreadCallback());
}

TEST(DrawLooper, repeatedlyProcessesFramesWhenLayerNeedsProcessFrame) {
    DrawLooperTestContainer container;

    container.addLayerRootToLooper(container.layerRoot);

    auto animation =
        Valdi::makeShared<Animation>(Duration(1.0), InterpolationFunctions::linear(), [](Layer& view, double ratio) {
            view.setScaleX(static_cast<Scalar>(ratio));
            view.setScaleY(static_cast<Scalar>(ratio));
        });

    container.layerRoot->getContentLayer()->addAnimation(STRING_LITERAL("scale"), animation);

    ASSERT_TRUE(container.frameScheduler->runNextMainThreadCallback());
    ASSERT_EQ(static_cast<Scalar>(0.0f), container.layerRoot->getContentLayer()->getScaleX());
    ASSERT_EQ(static_cast<Scalar>(0.0f), container.layerRoot->getContentLayer()->getScaleY());

    container.frameScheduler->advanceTime(0.5);

    ASSERT_TRUE(container.frameScheduler->runNextMainThreadCallback());
    ASSERT_EQ(static_cast<Scalar>(0.5f), container.layerRoot->getContentLayer()->getScaleX());
    ASSERT_EQ(static_cast<Scalar>(0.5f), container.layerRoot->getContentLayer()->getScaleY());

    container.frameScheduler->advanceTime(0.5);

    ASSERT_TRUE(container.frameScheduler->runNextMainThreadCallback());
    ASSERT_EQ(static_cast<Scalar>(1.0f), container.layerRoot->getContentLayer()->getScaleX());
    ASSERT_EQ(static_cast<Scalar>(1.0f), container.layerRoot->getContentLayer()->getScaleY());

    container.frameScheduler->advanceTime(0.5);

    ASSERT_FALSE(container.frameScheduler->runNextMainThreadCallback());
};

TEST(DrawLooper, schedulesDraw) {
    DrawLooperTestContainer container;

    auto surfacePresenterManager = container.addLayerRootToLooper(container.layerRoot);

    ASSERT_FALSE(container.frameScheduler->runNextVSyncCallback());

    ASSERT_TRUE(container.frameScheduler->runNextMainThreadCallback());

    ASSERT_TRUE(container.frameScheduler->runNextVSyncCallback());

    auto pixelBitmap = surfacePresenterManager->getSurfaceSinglePixelBitmap(0);

    ASSERT_TRUE(pixelBitmap != nullptr);

    ASSERT_EQ(Color::black(), pixelBitmap->getPixel());

    ASSERT_FALSE(container.frameScheduler->runNextVSyncCallback());

    container.frameScheduler->advanceTime(1.0);

    container.layerRoot->getContentLayer()->setBackgroundColor(Color::red());

    ASSERT_FALSE(container.frameScheduler->runNextVSyncCallback());

    ASSERT_TRUE(container.frameScheduler->runNextMainThreadCallback());
    ASSERT_TRUE(container.frameScheduler->runNextVSyncCallback());

    ASSERT_EQ(Color::red(), pixelBitmap->getPixel());

    container.frameScheduler->advanceTime(1.0);
    ASSERT_FALSE(container.frameScheduler->runNextMainThreadCallback());
    ASSERT_FALSE(container.frameScheduler->runNextVSyncCallback());
}

TEST(DrawLooper, redrawsOnDrawableSurfaceChange) {
    DrawLooperTestContainer container;

    auto surfacePresenterManager = makeShared<TestDelayedSurfacePresenterManager>();
    container.drawLooper->addLayerRoot(container.layerRoot, surfacePresenterManager, false);

    ASSERT_TRUE(container.frameScheduler->runNextMainThreadCallback());
    ASSERT_FALSE(container.frameScheduler->runNextVSyncCallback());

    auto pixelBitmap = surfacePresenterManager->getSurfaceSinglePixelBitmap(0);

    ASSERT_TRUE(pixelBitmap != nullptr);

    ASSERT_EQ(Color::white(), pixelBitmap->getPixel());

    container.drawLooper->setDrawableSurfaceOfLayerRootForPresenterId(
        *container.layerRoot, 1, surfacePresenterManager->createDrawableSurface(pixelBitmap));

    ASSERT_FALSE(container.frameScheduler->runNextMainThreadCallback());
    ASSERT_TRUE(container.frameScheduler->runNextVSyncCallback());

    ASSERT_EQ(Color::black(), pixelBitmap->getPixel());

    pixelBitmap->setPixel(Color::red());
    auto newPixelBitmap = Valdi::makeShared<SinglePixelBitmap>();

    container.drawLooper->setDrawableSurfaceOfLayerRootForPresenterId(
        *container.layerRoot, 1, surfacePresenterManager->createDrawableSurface(newPixelBitmap));

    ASSERT_EQ(Color::white(), newPixelBitmap->getPixel());

    ASSERT_FALSE(container.frameScheduler->runNextMainThreadCallback());
    ASSERT_TRUE(container.frameScheduler->runNextVSyncCallback());

    ASSERT_EQ(Color::red(), pixelBitmap->getPixel());
    ASSERT_EQ(Color::black(), newPixelBitmap->getPixel());

    container.drawLooper->setDrawableSurfaceOfLayerRootForPresenterId(*container.layerRoot, 1, nullptr);

    ASSERT_FALSE(container.frameScheduler->runNextVSyncCallback());
}

TEST(DrawLooper, redrawsWhenFrameReceivedDuringDraw) {
    DrawLooperTestContainer container;

    auto surfacePresenterManager = container.addLayerRootToLooper(container.layerRoot);
    container.layerRoot->getContentLayer()->setBackgroundColor(Color::blue());

    ASSERT_TRUE(container.frameScheduler->runNextMainThreadCallback());
    ASSERT_TRUE(container.frameScheduler->runNextVSyncCallback());

    auto pixelBitmap = surfacePresenterManager->getSurfaceSinglePixelBitmap(0);

    ASSERT_TRUE(pixelBitmap != nullptr);

    bool didUpdate = false;

    pixelBitmap->setOnLockCallback([&]() {
        if (didUpdate) {
            return;
        }
        didUpdate = true;
        // Update the tree while drawing to simulate a UI thread update while the drawer is drawing
        container.layerRoot->getContentLayer()->setBackgroundColor(Color::red());
        container.frameScheduler->advanceTime(1.0);
        ASSERT_TRUE(container.frameScheduler->runNextMainThreadCallback());
    });

    ASSERT_EQ(Color::blue(), pixelBitmap->getPixel());

    container.frameScheduler->advanceTime(1.0);
    container.layerRoot->getContentLayer()->setBackgroundColor(Color::black());

    ASSERT_TRUE(container.frameScheduler->runNextMainThreadCallback());
    ASSERT_TRUE(container.frameScheduler->runNextVSyncCallback());

    ASSERT_EQ(Color::black(), pixelBitmap->getPixel());

    ASSERT_TRUE(container.frameScheduler->runNextVSyncCallback());

    ASSERT_EQ(Color::red(), pixelBitmap->getPixel());

    ASSERT_FALSE(container.frameScheduler->runNextVSyncCallback());
    ASSERT_FALSE(container.frameScheduler->runNextMainThreadCallback());
}

TEST(DrawLooper, redrawsWhenRemovingAndReaddingLayerRoot) {
    DrawLooperTestContainer container;

    container.layerRoot->getContentLayer()->setBackgroundColor(Color::blue());

    auto surfacePresenterManager = makeShared<TestSurfacePresenterManager>();
    container.drawLooper->addLayerRoot(container.layerRoot, surfacePresenterManager, true);

    ASSERT_TRUE(container.frameScheduler->runNextMainThreadCallback());
    ASSERT_TRUE(container.frameScheduler->runNextVSyncCallback());

    ASSERT_FALSE(container.frameScheduler->runNextMainThreadCallback());
    ASSERT_FALSE(container.frameScheduler->runNextVSyncCallback());

    ASSERT_EQ(static_cast<size_t>(1), surfacePresenterManager->getSurfaces().size());

    container.frameScheduler->advanceTime(1.0);
    container.drawLooper->removeLayerRoot(container.layerRoot);

    ASSERT_FALSE(container.frameScheduler->runNextMainThreadCallback());
    ASSERT_FALSE(container.frameScheduler->runNextVSyncCallback());

    ASSERT_EQ(static_cast<size_t>(0), surfacePresenterManager->getSurfaces().size());

    container.frameScheduler->advanceTime(1.0);
    container.drawLooper->addLayerRoot(container.layerRoot, surfacePresenterManager, false);

    ASSERT_TRUE(container.frameScheduler->runNextMainThreadCallback());
    ASSERT_TRUE(container.frameScheduler->runNextVSyncCallback());

    ASSERT_FALSE(container.frameScheduler->runNextMainThreadCallback());
    ASSERT_FALSE(container.frameScheduler->runNextVSyncCallback());

    ASSERT_EQ(static_cast<size_t>(1), surfacePresenterManager->getSurfaces().size());
}

TEST(DrawLooper, canSplitDrawingWhenUsingExternalSurface) {
    DrawLooperTestContainer container;

    auto surfacePresenterManager = makeShared<Test4PixelsBitmapSurfacePresenterManager>();
    container.drawLooper->addLayerRoot(container.layerRoot, surfacePresenterManager, false);
    container.layerRoot->getContentLayer()->setBackgroundColor(Color::blue());

    auto beforeSiblingLayer = makeLayer<Layer>(container.resources);
    beforeSiblingLayer->setBackgroundColor(Color::green());
    beforeSiblingLayer->setFrame(Rect::makeXYWH(1, 1, 1, 1));

    auto externalLayer = makeLayer<ExternalLayer>(container.resources);
    externalLayer->setFrame(Rect::makeXYWH(1, 1, 2, 2));

    auto afterSiblingLayer = makeLayer<Layer>(container.resources);
    afterSiblingLayer->setBackgroundColor(Color::red());
    afterSiblingLayer->setFrame(Rect::makeXYWH(2, 2, 1, 1));

    container.layerRoot->getContentLayer()->addChild(beforeSiblingLayer);
    container.layerRoot->getContentLayer()->addChild(externalLayer);
    container.layerRoot->getContentLayer()->addChild(afterSiblingLayer);

    container.layerRoot->setSize(Size::make(4, 4), 1);

    ASSERT_TRUE(container.frameScheduler->runNextMainThreadCallback());
    ASSERT_TRUE(container.frameScheduler->runNextVSyncCallback());

    auto backgroundBitmap = surfacePresenterManager->getSurfaceBitmap(0);
    SC_ASSERT(backgroundBitmap != nullptr);

    // Since no surfaces are set, the background bitmap should have all the content in it

    ASSERT_EQ(*BitmapBuilder(4, 4)
                   .row({Color::blue(), Color::blue(), Color::blue(), Color::blue()})
                   .row({
                       Color::blue(),
                       Color::green(),
                       Color::blue(),
                       Color::blue(),
                   })
                   .row({

                       Color::blue(),
                       Color::blue(),
                       Color::red(),
                       Color::blue(),
                   })
                   .row({
                       Color::blue(),
                       Color::blue(),
                       Color::blue(),
                       Color::blue(),
                   })
                   .build(),
              *backgroundBitmap);

    auto foregroundBitmap = surfacePresenterManager->getSurfaceBitmap(1);
    ASSERT_TRUE(foregroundBitmap == nullptr);

    auto externalSurface = makeShared<TestExternalSurface>();
    externalLayer->setExternalSurface(externalSurface);

    container.frameScheduler->advanceTime(1.0);

    ASSERT_TRUE(container.frameScheduler->runNextMainThreadCallback());
    // Drawing should be synchronous when an external surface is being updated
    ASSERT_FALSE(container.frameScheduler->runNextVSyncCallback());

    ASSERT_EQ(static_cast<size_t>(3), surfacePresenterManager->getSurfaces().size());
    ASSERT_EQ(externalSurface, surfacePresenterManager->getSurfaces()[1]);

    foregroundBitmap = surfacePresenterManager->getSurfaceBitmap(2);
    ASSERT_TRUE(foregroundBitmap != nullptr);

    // background should not have the red color from the sibling after the surface
    ASSERT_EQ(*BitmapBuilder(4, 4)
                   .row({
                       Color::blue(),
                       Color::blue(),
                       Color::blue(),
                       Color::blue(),
                   })
                   .row({
                       Color::blue(),
                       Color::green(),
                       Color::blue(),
                       Color::blue(),
                   })
                   .row({
                       Color::blue(),
                       Color::blue(),
                       Color::blue(),
                       Color::blue(),
                   })
                   .row({
                       Color::blue(),
                       Color::blue(),
                       Color::blue(),
                       Color::blue(),
                   })
                   .build(),
              *backgroundBitmap);

    // red color should be set
    ASSERT_EQ(*BitmapBuilder(4, 4)
                   .row({
                       Color::transparent(),
                       Color::transparent(),
                       Color::transparent(),
                       Color::transparent(),
                   })
                   .row({
                       Color::transparent(),
                       Color::transparent(),
                       Color::transparent(),
                       Color::transparent(),
                   })
                   .row({
                       Color::transparent(),
                       Color::transparent(),
                       Color::red(),
                       Color::transparent(),
                   })
                   .row({
                       Color::transparent(),
                       Color::transparent(),
                       Color::transparent(),
                       Color::transparent(),
                   })
                   .build(),
              *foregroundBitmap);

    ASSERT_TRUE(externalSurface->presenterState);

    Matrix expectedTransform;

    ASSERT_EQ(Rect::makeXYWH(1, 1, 2, 2), externalSurface->presenterState.value().frame);
    ASSERT_EQ(expectedTransform, externalSurface->presenterState.value().transform);
    ASSERT_EQ(1.0, externalSurface->presenterState.value().opacity);
}

TEST(DrawLooper, drawsSynchronouslyWhenUpdatingPresenters) {
    DrawLooperTestContainer container;

    auto surfacePresenterManager = makeShared<TestSurfacePresenterManager>();
    container.drawLooper->addLayerRoot(container.layerRoot, surfacePresenterManager, false);
    container.layerRoot->getContentLayer()->setBackgroundColor(Color::blue());

    auto externalLayer = makeLayer<ExternalLayer>(container.resources);
    auto externalSurface = makeShared<TestExternalSurface>();
    externalLayer->setExternalSurface(externalSurface);
    externalLayer->setFrame(Rect::makeXYWH(0, 0, 1, 1));

    container.layerRoot->getContentLayer()->addChild(externalLayer);

    container.layerRoot->setSize(Size::make(1, 1), 1);

    ASSERT_TRUE(container.frameScheduler->runNextMainThreadCallback());

    ASSERT_EQ(static_cast<size_t>(2), surfacePresenterManager->getSurfaces().size());
    auto pixelBitmap = surfacePresenterManager->getSurfaceSinglePixelBitmap(0);
    ASSERT_TRUE(pixelBitmap != nullptr);
    ASSERT_EQ(externalSurface, surfacePresenterManager->getSurfaces()[1]);

    // Color should have been applied immediately given that we have an external surface
    ASSERT_EQ(Color::blue(), pixelBitmap->getPixel());

    ASSERT_FALSE(container.frameScheduler->runNextVSyncCallback());

    container.frameScheduler->advanceTime(1.0);
    container.layerRoot->getContentLayer()->setBackgroundColor(Color::red());

    ASSERT_TRUE(container.frameScheduler->runNextMainThreadCallback());

    // Since we are not updating presenters, the update should be async
    ASSERT_EQ(Color::blue(), pixelBitmap->getPixel());

    ASSERT_TRUE(container.frameScheduler->runNextVSyncCallback());

    ASSERT_EQ(Color::red(), pixelBitmap->getPixel());

    container.frameScheduler->advanceTime(1.0);
    container.layerRoot->getContentLayer()->setBackgroundColor(Color::white());
    externalLayer->setOpacity(0.5);

    ASSERT_TRUE(container.frameScheduler->runNextMainThreadCallback());

    // Color should have been applied immediately, as the external layer was updated
    ASSERT_EQ(Color::white(), pixelBitmap->getPixel());

    ASSERT_FALSE(container.frameScheduler->runNextVSyncCallback());

    auto foregroundLayer = makeLayer<Layer>(container.resources);
    foregroundLayer->setBackgroundColor(Color::green());
    foregroundLayer->setFrame(Rect::makeXYWH(0, 0, 1, 1));

    container.frameScheduler->advanceTime(1.0);
    container.layerRoot->getContentLayer()->addChild(foregroundLayer);
    container.layerRoot->getContentLayer()->setBackgroundColor(Color::blue());

    ASSERT_TRUE(container.frameScheduler->runNextMainThreadCallback());

    // When just adding a Drawable presenter, drawing should stay async
    ASSERT_EQ(Color::white(), pixelBitmap->getPixel());

    ASSERT_EQ(static_cast<size_t>(3), surfacePresenterManager->getSurfaces().size());

    ASSERT_TRUE(container.frameScheduler->runNextVSyncCallback());

    ASSERT_EQ(Color::blue(), pixelBitmap->getPixel());

    container.frameScheduler->advanceTime(1.0);
    container.layerRoot->getContentLayer()->setBackgroundColor(Color::red());
    foregroundLayer->removeFromParent();

    ASSERT_TRUE(container.frameScheduler->runNextMainThreadCallback());

    // When removing a Drawable presenter, drawing should be sync
    ASSERT_EQ(Color::red(), pixelBitmap->getPixel());
    ASSERT_FALSE(container.frameScheduler->runNextVSyncCallback());

    ASSERT_EQ(static_cast<size_t>(2), surfacePresenterManager->getSurfaces().size());

    container.frameScheduler->advanceTime(1.0);
    container.layerRoot->getContentLayer()->setBackgroundColor(Color::blue());
    externalLayer->removeFromParent();

    ASSERT_TRUE(container.frameScheduler->runNextMainThreadCallback());

    // When removing an external layer, drawing should be sync
    ASSERT_EQ(Color::blue(), pixelBitmap->getPixel());
    ASSERT_FALSE(container.frameScheduler->runNextVSyncCallback());

    ASSERT_EQ(static_cast<size_t>(1), surfacePresenterManager->getSurfaces().size());
}

TEST(DrawLooper, drawsAsynchronouslyWhenUpdatingPresenters) {
    DrawLooperTestContainer container;

    auto surfacePresenterManager = makeShared<TestSurfacePresenterManager>();
    // Specify to disallow sync drawing here.
    container.drawLooper->addLayerRoot(container.layerRoot, surfacePresenterManager, true);
    container.layerRoot->getContentLayer()->setBackgroundColor(Color::blue());

    auto externalLayer = makeLayer<ExternalLayer>(container.resources);
    auto externalSurface = makeShared<TestExternalSurface>();
    externalLayer->setExternalSurface(externalSurface);
    externalLayer->setFrame(Rect::makeXYWH(0, 0, 1, 1));

    container.layerRoot->getContentLayer()->addChild(externalLayer);

    container.layerRoot->setSize(Size::make(1, 1), 1);

    ASSERT_TRUE(container.frameScheduler->runNextMainThreadCallback());

    ASSERT_EQ(static_cast<size_t>(2), surfacePresenterManager->getSurfaces().size());
    auto pixelBitmap = surfacePresenterManager->getSurfaceSinglePixelBitmap(0);
    ASSERT_TRUE(pixelBitmap != nullptr);
    ASSERT_EQ(externalSurface, surfacePresenterManager->getSurfaces()[1]);

    // Color should NOT have been applied immediately given that we specified to disallow sync drawing
    ASSERT_EQ(Color::white(), pixelBitmap->getPixel());

    // Run a vsync which should then update the surface
    ASSERT_TRUE(container.frameScheduler->runNextVSyncCallback());
    pixelBitmap = surfacePresenterManager->getSurfaceSinglePixelBitmap(0);
    ASSERT_EQ(Color::blue(), pixelBitmap->getPixel());
}

TEST(DrawLooper, drawsSynchronouslyWhenReceivingDrawableSurfaceAfterUpdatingPresenters) {
    DrawLooperTestContainer container;

    auto surfacePresenterManager = makeShared<TestSurfacePresenterManager>();
    container.drawLooper->addLayerRoot(container.layerRoot, surfacePresenterManager, false);
    container.layerRoot->getContentLayer()->setBackgroundColor(Color::blue());

    auto externalLayer = makeLayer<ExternalLayer>(container.resources);
    externalLayer->setExternalSurface(makeShared<TestExternalSurface>());
    externalLayer->setFrame(Rect::makeXYWH(0, 0, 1, 1));

    container.layerRoot->getContentLayer()->addChild(externalLayer);

    container.layerRoot->setSize(Size::make(1, 1), 1);

    auto foregroundLayer = makeLayer<Layer>(container.resources);
    foregroundLayer->setBackgroundColor(Color::green());
    foregroundLayer->setFrame(Rect::makeXYWH(0, 0, 1, 1));

    container.layerRoot->getContentLayer()->addChild(foregroundLayer);

    ASSERT_TRUE(container.frameScheduler->runNextMainThreadCallback());
    // Drawing should have been synchronous
    ASSERT_FALSE(container.frameScheduler->runNextVSyncCallback());

    ASSERT_EQ(static_cast<size_t>(3), surfacePresenterManager->getSurfaces().size());
    auto presenter = surfacePresenterManager->getPresenter(0);
    ASSERT_TRUE(presenter.has_value());
    auto pixelBitmap = surfacePresenterManager->getSurfaceSinglePixelBitmap(0);
    ASSERT_TRUE(pixelBitmap != nullptr);

    ASSERT_EQ(Color::blue(), pixelBitmap->getPixel());

    container.frameScheduler->advanceTime(1.0);

    externalLayer->setExternalSurface(nullptr);

    // Explicitly remove the DrawableSurface attached to the background presenter
    container.drawLooper->setDrawableSurfaceOfLayerRootForPresenterId(*container.layerRoot, 1, nullptr);

    ASSERT_TRUE(container.frameScheduler->runNextMainThreadCallback());
    // No VSync should have happened since none of the drawable presenters have a surface
    ASSERT_FALSE(container.frameScheduler->runNextVSyncCallback());

    // Since the external surface was removed from the ExternalLayer, it should have resolved to 1 drawable presenter.
    ASSERT_EQ(static_cast<size_t>(1), surfacePresenterManager->getSurfaces().size());

    // Surface should not have been updated
    ASSERT_EQ(Color::blue(), pixelBitmap->getPixel());

    auto surface = Valdi::castOrNull<DrawableSurface>(presenter.value().surface);
    ASSERT_TRUE(surface != nullptr);
    container.drawLooper->setDrawableSurfaceOfLayerRootForPresenterId(*container.layerRoot, 1, surface);

    ASSERT_FALSE(container.frameScheduler->runNextMainThreadCallback());
    // It should have drawn synchronously, without scheduling a vsync
    ASSERT_FALSE(container.frameScheduler->runNextVSyncCallback());

    // Color should have been updated by setting the DrawableSurface
    // The green comes from the foreground layer covering the background layer
    ASSERT_EQ(Color::green(), pixelBitmap->getPixel());
}

TEST(DrawLooper, canConfigureManagedGraphicsContext) {
    DrawLooperTestContainer container;

    auto surfacePresenterManager = makeShared<TestSurfacePresenterManager>();
    auto graphicsContext = surfacePresenterManager->getGraphicsContext();

    ASSERT_FALSE(container.frameScheduler->runNextVSyncCallback());

    ASSERT_EQ(static_cast<size_t>(0), graphicsContext->getResourceCacheLimitRequests().size());
    ASSERT_EQ(static_cast<size_t>(0), graphicsContext->getPerformCleanupRequests().size());

    container.drawLooper->appendManagedGraphicsContext(surfacePresenterManager->getGraphicsContext());

    ASSERT_TRUE(container.frameScheduler->runNextVSyncCallback());
    ASSERT_FALSE(container.frameScheduler->runNextVSyncCallback());

    ASSERT_EQ(static_cast<size_t>(1), graphicsContext->getResourceCacheLimitRequests().size());
    ASSERT_EQ(static_cast<size_t>(0), graphicsContext->getPerformCleanupRequests().size());

    ASSERT_EQ(static_cast<size_t>(134217728), graphicsContext->getResourceCacheLimitRequests()[0]);
}

TEST(DrawLooper, performCleanUpOnGraphicsContextWhenEnteringBackground) {
    DrawLooperTestContainer container;

    auto surfacePresenterManager = makeShared<TestSurfacePresenterManager>();
    auto graphicsContext = surfacePresenterManager->getGraphicsContext();

    container.drawLooper->appendManagedGraphicsContext(surfacePresenterManager->getGraphicsContext());

    ASSERT_TRUE(container.frameScheduler->runNextVSyncCallback());
    ASSERT_FALSE(container.frameScheduler->runNextVSyncCallback());
    graphicsContext->clearRequests();

    container.drawLooper->onApplicationEnteringBackground();

    ASSERT_TRUE(container.frameScheduler->runNextVSyncCallback());
    ASSERT_FALSE(container.frameScheduler->runNextVSyncCallback());

    ASSERT_EQ(static_cast<size_t>(2), graphicsContext->getResourceCacheLimitRequests().size());
    ASSERT_EQ(static_cast<size_t>(1), graphicsContext->getPerformCleanupRequests().size());

    ASSERT_EQ(static_cast<size_t>(33554432), graphicsContext->getResourceCacheLimitRequests()[0]);
    ASSERT_EQ(static_cast<size_t>(134217728), graphicsContext->getResourceCacheLimitRequests()[1]);
    ASSERT_TRUE(graphicsContext->getPerformCleanupRequests()[0].shouldPurgeScratchResources);
    ASSERT_EQ(std::chrono::seconds(0), graphicsContext->getPerformCleanupRequests()[0].secondsNotUsed);
}

TEST(DrawLooper, performCleanUpOnGraphicsContextOnLowMemory) {
    DrawLooperTestContainer container;

    auto surfacePresenterManager = makeShared<TestSurfacePresenterManager>();
    auto graphicsContext = surfacePresenterManager->getGraphicsContext();

    container.drawLooper->appendManagedGraphicsContext(surfacePresenterManager->getGraphicsContext());

    ASSERT_TRUE(container.frameScheduler->runNextVSyncCallback());
    ASSERT_FALSE(container.frameScheduler->runNextVSyncCallback());
    graphicsContext->clearRequests();

    container.drawLooper->onApplicationIsInLowMemory();

    ASSERT_TRUE(container.frameScheduler->runNextVSyncCallback());
    ASSERT_FALSE(container.frameScheduler->runNextVSyncCallback());

    ASSERT_EQ(static_cast<size_t>(2), graphicsContext->getResourceCacheLimitRequests().size());
    ASSERT_EQ(static_cast<size_t>(1), graphicsContext->getPerformCleanupRequests().size());

    ASSERT_EQ(static_cast<size_t>(0), graphicsContext->getResourceCacheLimitRequests()[0]);
    ASSERT_EQ(static_cast<size_t>(134217728), graphicsContext->getResourceCacheLimitRequests()[1]);
    ASSERT_TRUE(graphicsContext->getPerformCleanupRequests()[0].shouldPurgeScratchResources);
    ASSERT_EQ(std::chrono::seconds(0), graphicsContext->getPerformCleanupRequests()[0].secondsNotUsed);
}

TEST(DrawLooper, performCleanUpOnGraphicsContextAfterDraw) {
    DrawLooperTestContainer container;

    auto surfacePresenterManager = makeShared<TestSurfacePresenterManager>();
    auto graphicsContext = surfacePresenterManager->getGraphicsContext();

    container.drawLooper->appendManagedGraphicsContext(surfacePresenterManager->getGraphicsContext());

    ASSERT_TRUE(container.frameScheduler->runNextVSyncCallback());
    ASSERT_FALSE(container.frameScheduler->runNextVSyncCallback());
    graphicsContext->clearRequests();

    container.drawLooper->addLayerRoot(container.layerRoot, surfacePresenterManager, false);
    ASSERT_FALSE(container.frameScheduler->runNextVSyncCallback());

    ASSERT_TRUE(container.frameScheduler->runNextMainThreadCallback());

    ASSERT_TRUE(container.frameScheduler->runNextVSyncCallback());

    ASSERT_EQ(static_cast<size_t>(0), graphicsContext->getResourceCacheLimitRequests().size());
    ASSERT_EQ(static_cast<size_t>(1), graphicsContext->getPerformCleanupRequests().size());

    ASSERT_FALSE(graphicsContext->getPerformCleanupRequests()[0].shouldPurgeScratchResources);
    ASSERT_EQ(std::chrono::seconds(10), graphicsContext->getPerformCleanupRequests()[0].secondsNotUsed);

    container.frameScheduler->advanceTime(1.0);

    container.layerRoot->getContentLayer()->setBackgroundColor(Color::red());

    ASSERT_TRUE(container.frameScheduler->runNextMainThreadCallback());
    ASSERT_TRUE(container.frameScheduler->runNextVSyncCallback());

    ASSERT_EQ(static_cast<size_t>(0), graphicsContext->getResourceCacheLimitRequests().size());
    ASSERT_EQ(static_cast<size_t>(2), graphicsContext->getPerformCleanupRequests().size());

    ASSERT_FALSE(graphicsContext->getPerformCleanupRequests()[1].shouldPurgeScratchResources);
    ASSERT_EQ(std::chrono::seconds(10), graphicsContext->getPerformCleanupRequests()[1].secondsNotUsed);
}

TEST(DrawLooper, destroysPresentersOnRemove) {
    DrawLooperTestContainer container;

    auto surfacePresenterManager = container.addLayerRootToLooper(container.layerRoot);

    ASSERT_TRUE(container.layerRoot->needsProcessFrame());
    ASSERT_TRUE(container.frameScheduler->runNextMainThreadCallback());

    ASSERT_EQ(static_cast<size_t>(1), surfacePresenterManager->getSurfaces().size());

    container.drawLooper->removeLayerRoot(container.layerRoot);

    ASSERT_EQ(static_cast<size_t>(0), surfacePresenterManager->getSurfaces().size());
}

TEST(DrawLooper, doesNotDrawWhenInBackground) {
    DrawLooperTestContainer container;

    auto surfacePresenterManager = makeShared<TestSurfacePresenterManager>();
    auto graphicsContext = surfacePresenterManager->getGraphicsContext();

    container.drawLooper->appendManagedGraphicsContext(surfacePresenterManager->getGraphicsContext());

    ASSERT_TRUE(container.frameScheduler->runNextVSyncCallback());
    ASSERT_FALSE(container.frameScheduler->runNextVSyncCallback());
    graphicsContext->clearRequests();

    container.drawLooper->addLayerRoot(container.layerRoot, surfacePresenterManager, false);

    ASSERT_TRUE(container.frameScheduler->runNextMainThreadCallback());
    ASSERT_TRUE(container.frameScheduler->runNextVSyncCallback());

    container.drawLooper->onApplicationEnteringBackground();
    ASSERT_TRUE(container.frameScheduler->runNextVSyncCallback());

    container.frameScheduler->advanceTime(1.0);
    container.layerRoot->getContentLayer()->setBackgroundColor(Color::red());

    ASSERT_TRUE(container.frameScheduler->runNextMainThreadCallback());
    // No VSync callbacks should be attempted when we are in background
    ASSERT_FALSE(container.frameScheduler->runNextVSyncCallback());

    container.frameScheduler->advanceTime(2.0);
    container.layerRoot->getContentLayer()->setBackgroundColor(Color::green());

    ASSERT_TRUE(container.frameScheduler->runNextMainThreadCallback());
    ASSERT_FALSE(container.frameScheduler->runNextVSyncCallback());

    container.drawLooper->onApplicationEnteringForeground();

    ASSERT_TRUE(container.frameScheduler->runNextMainThreadCallback());
    // Should redraw after enterering foreground if we had a pending draw
    ASSERT_TRUE(container.frameScheduler->runNextVSyncCallback());

    container.frameScheduler->advanceTime(3.0);
    container.layerRoot->getContentLayer()->setBackgroundColor(Color::red());

    ASSERT_TRUE(container.frameScheduler->runNextMainThreadCallback());
    ASSERT_TRUE(container.frameScheduler->runNextVSyncCallback());
}

void updateSurfacePresenters(const Ref<DrawLooperEntry>& entry, const CompositorPlaneList& planeList) {
    entry->updateSurfacePresenters(planeList);
    // Check that the presenter states are correct

    const auto& surfacePresenters = entry->getSurfacePresenters();

    ASSERT_EQ(planeList.getPlanesCount(), surfacePresenters.size());

    size_t displayListPlaneIndex = 0;
    for (const auto& surfacePresenter : surfacePresenters) {
        ASSERT_EQ(displayListPlaneIndex, surfacePresenter.getDisplayListPlaneIndex());
        if (surfacePresenter.isDrawable()) {
            displayListPlaneIndex++;
        }
    }
}

TEST(DrawLooperEntry, canCreateDrawableSurfaces) {
    DrawLooperTestContainer container;
    auto surfacePresenterManager = makeShared<TestSurfacePresenterManager>();
    TestDrawLooperEntryListener listener;
    auto entry = makeShared<DrawLooperEntry>(container.layerRoot, surfacePresenterManager, &listener);

    CompositorPlaneList planeList;

    ASSERT_FALSE(entry->surfacePresentersNeedUpdate(planeList));

    planeList.appendDrawableSurface();

    ASSERT_TRUE(entry->surfacePresentersNeedUpdate(planeList));

    updateSurfacePresenters(entry, planeList);

    ASSERT_FALSE(entry->surfacePresentersNeedUpdate(planeList));

    ASSERT_EQ(static_cast<size_t>(1), surfacePresenterManager->getSurfaces().size());

    auto firstSurface = surfacePresenterManager->getSurfaces()[0];

    planeList.appendDrawableSurface();

    ASSERT_TRUE(entry->surfacePresentersNeedUpdate(planeList));

    updateSurfacePresenters(entry, planeList);

    ASSERT_FALSE(entry->surfacePresentersNeedUpdate(planeList));

    ASSERT_EQ(static_cast<size_t>(2), surfacePresenterManager->getSurfaces().size());

    ASSERT_EQ(firstSurface, surfacePresenterManager->getSurfaces()[0]);
    ASSERT_NE(firstSurface, surfacePresenterManager->getSurfaces()[1]);
}

TEST(DrawLooperEntry, destroysDanglingSurfaces) {
    DrawLooperTestContainer container;
    auto surfacePresenterManager = makeShared<TestSurfacePresenterManager>();

    TestDrawLooperEntryListener listener;
    auto entry = makeShared<DrawLooperEntry>(container.layerRoot, surfacePresenterManager, &listener);

    CompositorPlaneList planeList;

    planeList.appendDrawableSurface();
    planeList.appendDrawableSurface();
    planeList.appendDrawableSurface();

    ASSERT_TRUE(entry->surfacePresentersNeedUpdate(planeList));

    updateSurfacePresenters(entry, planeList);

    ASSERT_EQ(static_cast<size_t>(3), surfacePresenterManager->getSurfaces().size());

    auto firstSurface = surfacePresenterManager->getSurfaces()[0];
    auto secondSurface = surfacePresenterManager->getSurfaces()[1];

    planeList.removePlaneAtIndex(2);

    ASSERT_TRUE(entry->surfacePresentersNeedUpdate(planeList));

    updateSurfacePresenters(entry, planeList);

    ASSERT_FALSE(entry->surfacePresentersNeedUpdate(planeList));

    ASSERT_EQ(static_cast<size_t>(2), surfacePresenterManager->getSurfaces().size());

    ASSERT_EQ(firstSurface, surfacePresenterManager->getSurfaces()[0]);
    ASSERT_EQ(secondSurface, surfacePresenterManager->getSurfaces()[1]);
}

TEST(DrawLooperEntry, canReuseSurfaces) {
    DrawLooperTestContainer container;
    auto surfacePresenterManager = makeShared<TestSurfacePresenterManager>();

    TestDrawLooperEntryListener listener;
    auto entry = makeShared<DrawLooperEntry>(container.layerRoot, surfacePresenterManager, &listener);

    auto externalSurface1 = makeShared<TestExternalSurface>();
    auto externalSurface2 = makeShared<TestExternalSurface>();
    auto externalSurfaceSnapshot1 = makeShared<ExternalSurfaceSnapshot>(externalSurface1);
    auto externalSurfaceSnapshot2 = makeShared<ExternalSurfaceSnapshot>(externalSurface2);

    CompositorPlaneList planeList;

    planeList.appendDrawableSurface();
    planeList.appendPlane(CompositorPlane(externalSurfaceSnapshot1, ExternalSurfacePresenterState()));
    planeList.appendDrawableSurface();
    planeList.appendPlane(CompositorPlane(externalSurfaceSnapshot2, ExternalSurfacePresenterState()));
    planeList.appendDrawableSurface();

    ASSERT_TRUE(entry->surfacePresentersNeedUpdate(planeList));
    updateSurfacePresenters(entry, planeList);
    ASSERT_FALSE(entry->surfacePresentersNeedUpdate(planeList));

    ASSERT_EQ(static_cast<size_t>(5), surfacePresenterManager->getSurfaces().size());

    ASSERT_TRUE(castOrNull<DrawableSurface>(surfacePresenterManager->getSurfaces()[0]) != nullptr);
    ASSERT_EQ(externalSurface1, surfacePresenterManager->getSurfaces()[1]);
    ASSERT_TRUE(castOrNull<DrawableSurface>((surfacePresenterManager->getSurfaces()[2])) != nullptr);
    ASSERT_EQ(externalSurface2, surfacePresenterManager->getSurfaces()[3]);
    ASSERT_TRUE(castOrNull<DrawableSurface>((surfacePresenterManager->getSurfaces()[4])) != nullptr);

    auto firstDrawableSurface = surfacePresenterManager->getSurfaces()[0];
    auto secondDrawableSurface = surfacePresenterManager->getSurfaces()[2];
    auto thirdDrawableSurface = surfacePresenterManager->getSurfaces()[4];

    planeList.clear();

    planeList.appendPlane(CompositorPlane(externalSurfaceSnapshot2, ExternalSurfacePresenterState()));
    planeList.appendPlane(CompositorPlane(externalSurfaceSnapshot1, ExternalSurfacePresenterState()));
    planeList.appendDrawableSurface();
    planeList.appendDrawableSurface();
    planeList.appendDrawableSurface();

    ASSERT_TRUE(entry->surfacePresentersNeedUpdate(planeList));
    updateSurfacePresenters(entry, planeList);
    ASSERT_FALSE(entry->surfacePresentersNeedUpdate(planeList));

    ASSERT_EQ(static_cast<size_t>(5), surfacePresenterManager->getSurfaces().size());

    ASSERT_EQ(externalSurface2, surfacePresenterManager->getSurfaces()[0]);
    ASSERT_EQ(externalSurface1, surfacePresenterManager->getSurfaces()[1]);
    ASSERT_EQ(firstDrawableSurface, surfacePresenterManager->getSurfaces()[2]);
    ASSERT_EQ(secondDrawableSurface, surfacePresenterManager->getSurfaces()[3]);
    ASSERT_EQ(thirdDrawableSurface, surfacePresenterManager->getSurfaces()[4]);

    planeList.clear();

    planeList.appendPlane(CompositorPlane(externalSurfaceSnapshot2, ExternalSurfacePresenterState()));
    planeList.appendDrawableSurface();
    planeList.appendDrawableSurface();
    planeList.appendPlane(CompositorPlane(externalSurfaceSnapshot1, ExternalSurfacePresenterState()));
    planeList.appendDrawableSurface();

    ASSERT_TRUE(entry->surfacePresentersNeedUpdate(planeList));
    updateSurfacePresenters(entry, planeList);
    ASSERT_FALSE(entry->surfacePresentersNeedUpdate(planeList));

    ASSERT_EQ(static_cast<size_t>(5), surfacePresenterManager->getSurfaces().size());

    ASSERT_EQ(externalSurface2, surfacePresenterManager->getSurfaces()[0]);
    ASSERT_EQ(firstDrawableSurface, surfacePresenterManager->getSurfaces()[1]);
    ASSERT_EQ(secondDrawableSurface, surfacePresenterManager->getSurfaces()[2]);
    ASSERT_EQ(externalSurface1, surfacePresenterManager->getSurfaces()[3]);
    ASSERT_EQ(thirdDrawableSurface, surfacePresenterManager->getSurfaces()[4]);

    planeList.clear();

    planeList.appendDrawableSurface();
    planeList.appendDrawableSurface();
    planeList.appendDrawableSurface();

    ASSERT_TRUE(entry->surfacePresentersNeedUpdate(planeList));
    updateSurfacePresenters(entry, planeList);
    ASSERT_FALSE(entry->surfacePresentersNeedUpdate(planeList));

    ASSERT_EQ(static_cast<size_t>(3), surfacePresenterManager->getSurfaces().size());

    ASSERT_EQ(firstDrawableSurface, surfacePresenterManager->getSurfaces()[0]);
    ASSERT_EQ(secondDrawableSurface, surfacePresenterManager->getSurfaces()[1]);
    ASSERT_EQ(thirdDrawableSurface, surfacePresenterManager->getSurfaces()[2]);

    planeList.appendPlane(CompositorPlane(externalSurfaceSnapshot1, ExternalSurfacePresenterState()));
    ASSERT_TRUE(entry->surfacePresentersNeedUpdate(planeList));

    updateSurfacePresenters(entry, planeList);

    ASSERT_FALSE(entry->surfacePresentersNeedUpdate(planeList));

    ASSERT_EQ(static_cast<size_t>(4), surfacePresenterManager->getSurfaces().size());

    ASSERT_EQ(firstDrawableSurface, surfacePresenterManager->getSurfaces()[0]);
    ASSERT_EQ(secondDrawableSurface, surfacePresenterManager->getSurfaces()[1]);
    ASSERT_EQ(thirdDrawableSurface, surfacePresenterManager->getSurfaces()[2]);
    ASSERT_EQ(externalSurface1, surfacePresenterManager->getSurfaces()[3]);

    planeList.insertPlane(CompositorPlane(externalSurfaceSnapshot2, ExternalSurfacePresenterState()), 1);

    ASSERT_TRUE(entry->surfacePresentersNeedUpdate(planeList));
    updateSurfacePresenters(entry, planeList);
    ASSERT_FALSE(entry->surfacePresentersNeedUpdate(planeList));

    ASSERT_EQ(static_cast<size_t>(5), surfacePresenterManager->getSurfaces().size());

    ASSERT_EQ(firstDrawableSurface, surfacePresenterManager->getSurfaces()[0]);
    ASSERT_EQ(externalSurface2, surfacePresenterManager->getSurfaces()[1]);
    ASSERT_EQ(secondDrawableSurface, surfacePresenterManager->getSurfaces()[2]);
    ASSERT_EQ(thirdDrawableSurface, surfacePresenterManager->getSurfaces()[3]);
    ASSERT_EQ(externalSurface1, surfacePresenterManager->getSurfaces()[4]);
}

TEST(DrawLooperEntry, detectsExternalSurfacePresenterStateChange) {
    DrawLooperTestContainer container;
    auto surfacePresenterManager = makeShared<TestSurfacePresenterManager>();

    TestDrawLooperEntryListener listener;
    auto entry = makeShared<DrawLooperEntry>(container.layerRoot, surfacePresenterManager, &listener);

    auto externalSurface = makeShared<TestExternalSurface>();
    auto externalSurfaceSnapshot = makeShared<ExternalSurfaceSnapshot>(externalSurface);

    CompositorPlaneList planeList;

    ExternalSurfacePresenterState presenterState;

    planeList.appendDrawableSurface();
    planeList.appendPlane(CompositorPlane(externalSurfaceSnapshot, presenterState));
    planeList.appendDrawableSurface();

    ASSERT_TRUE(entry->surfacePresentersNeedUpdate(planeList));
    updateSurfacePresenters(entry, planeList);
    ASSERT_FALSE(entry->surfacePresentersNeedUpdate(planeList));

    ASSERT_TRUE(externalSurface->presenterState);
    ASSERT_EQ(presenterState, externalSurface->presenterState.value());

    auto updateAndCheck = [&]() {
        ASSERT_NE(presenterState, externalSurface->presenterState.value());

        planeList.getPlaneAtIndex(1) = CompositorPlane(externalSurfaceSnapshot, presenterState);

        ASSERT_TRUE(entry->surfacePresentersNeedUpdate(planeList));
        updateSurfacePresenters(entry, planeList);
        ASSERT_FALSE(entry->surfacePresentersNeedUpdate(planeList));

        ASSERT_EQ(presenterState, externalSurface->presenterState.value());
    };

    // Updating frame
    presenterState.frame = Rect::makeXYWH(0, 0, 40, 40);

    updateAndCheck();

    // Updating transform
    presenterState.transform.setScaleX(2.0);

    updateAndCheck();

    // Updating clipPath
    presenterState.clipPath.addRect(Rect::makeXYWH(0, 0, 40, 40), true);

    updateAndCheck();

    // Updating opacity
    presenterState.opacity = 0.5;
    updateAndCheck();
}

} // namespace snap::drawing
