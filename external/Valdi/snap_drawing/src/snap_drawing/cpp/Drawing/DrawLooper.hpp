//
//  DrawLooper.hpp
//  snap_drawing-pc
//
//  Created by Simon Corsin on 1/21/22.
//

#pragma once

#include "snap_drawing/cpp/Drawing/DrawLooperEntry.hpp"
#include "snap_drawing/cpp/Drawing/IFrameScheduler.hpp"
#include "snap_drawing/cpp/Layers/LayerRoot.hpp"
#include "snap_drawing/cpp/Utils/Aliases.hpp"
#include "valdi_core/cpp/Interfaces/ILogger.hpp"
#include "valdi_core/cpp/Utils/FlatMap.hpp"
#include "valdi_core/cpp/Utils/Mutex.hpp"
#include "valdi_core/cpp/Utils/SmallVector.hpp"

#include <vector>

namespace snap::drawing {

using EntriesLock = std::unique_lock<std::recursive_mutex>;
using DrawLock = std::unique_lock<std::recursive_mutex>;
using DrawOperationsBatch = Valdi::SmallVector<Ref<DrawOperation>, 8>;

class PerformCleanupCallback;
class ConfigureCacheSizeCallback;

/**
 The DrawLooper is a class that manages frame processing and drawing on LayerRoot objects.
 Frame processing is done in the main thread, while drawing is done on a background thread.
 It relies on a FrameScheduler instance to schedule VSync and main thread callbacks.

 The DrawLooper will keep enqueueing callbacks into the main thread as long as there
 are LayerRoot that needs frame processing, which can happen if they have running animations,
 gestures, or any external updates to the Layer tree. The FrameScheduler can dequeue those
 callbacks as fast it wants.

 Similarly, the DrawLooper will keep enqueueing callbacks into the draw thread on VSync as long
 as there are pending frames to be dequeued.

 Every added LayerRoot is associated an Entry, which holds the last and next frame to draw and
 the GraphicsContext associated to that LayerRoot. As such the looper is double buffered, as it
 can hold one pending frame to draw while another frame might be drawing. Whenever a frame is
 emitted during frame processing on the main thread, it is enqueued and later
 dequeued when drawing in the VSync thread. When the VSync callback is called, the looper goes
 its entries, extracts the pending frames that should be drawn, and then draw them.

 It uses 2 mutexes: a draw mutex and an entries mutex. The entries mutex is the main mutex for
 which a lock is acquired whenever doing any reading or writing on the entries that the looper holds.
 Most calls into the looper ends up acquiring the entries mutex. The draw mutex is locked at the
 beginning of vsync and released after draw. This ensures that mutations on the GraphicsContext
 of a LayerRoot can be done synchronously with the VSync thread.
 */
class DrawLooper : public Valdi::SimpleRefCountable, protected DrawLooperEntryListener {
public:
    DrawLooper(const Ref<IFrameScheduler>& frameScheduler, Valdi::ILogger& logger);
    ~DrawLooper() override;

    /**
     Append a LayerRoot into the looper and associate it with a SurfacePresenterManager.
     The looper will begin calling LayerRoot::processFrame() on the main thread whenever the LayerRoot
     needs to be updated.
     When the LayerRoot emits new DisplayList, the DrawLooper will use the given SurfacePresenterManager
     to create drawable or external surface presenters to draw the content of the DisplayList as needed.
     `disallowSynchronousDraw` controls whether native views are drawn synchronously.
     */
    void addLayerRoot(const Ref<LayerRoot>& layerRoot,
                      const Ref<SurfacePresenterManager>& surfacePresenterManager,
                      bool disallowSynchronousDraw);

    /**
     Remove a LayerRoot from the looper, which will stop the looper from calling LayerRoot::processFrame()
     and will remove all previously created presenters.
     */
    void removeLayerRoot(const Ref<LayerRoot>& layerRoot);

    /**
     Draw the DisplayList plane content from the given drawable presenter id and LayerRoot
     into the given canvas. This can be used to dump the content of the presenter into a separate canvas.
     */
    bool drawPlaneOfLayerRootForPresenterId(LayerRoot& layerRoot,
                                            SurfacePresenterId surfacePresenterId,
                                            DrawableSurfaceCanvas& canvas);

    /**
     Set the DrawableSurface instance of the drawable presenter id and LayerRoot.
     If the SurfacePresenterManager implementation cannot synchronously return a DrawableSurface
     when calling SurfacePresenterManager::createPresenterWithDrawableSurface(), like on Android for instance,
     the DrawableSurface should be provided later on when it becomes available so that the DrawLooper
     can draw into it.
     */
    void setDrawableSurfaceOfLayerRootForPresenterId(LayerRoot& layerRoot,
                                                     SurfacePresenterId surfacePresenterId,
                                                     const Ref<DrawableSurface>& drawableSurface);

    /**
     Marks that a SurfacePresenter with the given id and LayerRoot needs to be redrawn.
     The DrawLooper will forcefully trigger a draw on that presenter to draw the last DisplayList
     plane that was drawn into it.
     */
    void setPresenterOfLayerRootNeedsRedraw(LayerRoot& layerRoot, SurfacePresenterId surfacePresenterId);

    /**
     Append a GraphicsContext to the list of managed GraphicsContexts.
     The GraphicsContext will be periodically purged to reduce its memory usage,
     Its resource cache limit will be updated depending on the application state.
     */
    void appendManagedGraphicsContext(const Ref<GraphicsContext>& graphicsContext);

    /**
     Remove a GraphicsContext from the list of managed GraphicsContext.
     */
    void removeManagedGraphicsContext(const GraphicsContext* graphicsContext);

    /**
     Notifies the DrawLooper that the application is entering background
     */
    void onApplicationEnteringBackground();

    /**
     Notifies the DrawLooper that the application is entering foreground
     */
    void onApplicationEnteringForeground();

    /**
     Notifies the DrawLooper that the application has low memory.
     This will free up as much resources as possible from the managed
     GraphicsContexts.
     */
    void onApplicationIsInLowMemory();

    /**
     Call LayerRoot::processFrame() for all registered LayerRoot that needs to be processed.
     The processed LayerRoot might then emit new DisplayList if the processFrame() call ended up
     triggering a draw. If new DisplayLists were emitted, a DrawLooper::drawFrames() pass is then scheduled in the
     VSync thread.

     This is called from the IFrameScheduler::onMainThread callback, which should be called in the main thread.
     */
    void processFrames(TimePoint time);

    /**
     Draw the frames for all the registered LayerRoot that emitted new DisplayLists in the last processFrames()
     pass. This is called from the IFrameScheduler::onNextVsync callback, which should be called in a background thread.
     */
    void drawFrames(TimePoint time);

    DrawLock getDrawLock() const;

protected:
    void onNeedsProcessFrame(DrawLooperEntry& entry) override;

    void onDidDraw(DrawLooperEntry& entry,
                   const Ref<DisplayList>& displayList,
                   const CompositorPlaneList* planeList) override;

    SurfacePresenterId createSurfacePresenterId() override;

private:
    friend PerformCleanupCallback;
    friend ConfigureCacheSizeCallback;

    enum class CleanUpMode {
        PostDraw,
        EnteringBackground,
        TrimMemory,
    };

    Ref<IFrameScheduler> _frameScheduler;
    [[maybe_unused]] Valdi::ILogger& _logger;
    std::vector<Ref<DrawLooperEntry>> _entries;
    std::vector<Ref<GraphicsContext>> _managedGraphicsContexts;
    mutable std::recursive_mutex _mainThreadMutex;
    mutable std::recursive_mutex _drawMutex;
    SurfacePresenterId _surfacePresenterIdSequence = 0;
    bool _processingFrames = false;
    bool _processFrameScheduled = false;
    bool _drawScheduled = false;
    bool _inBackground = false;

    Ref<DrawLooperEntry> getEntryForLayer(LayerRoot& layerRoot) const;
    Ref<DrawLooperEntry> mustGetEntryForLayer(LayerRoot& layerRoot) const;

    EntriesLock getEntriesLock() const;

    void scheduleDraw(EntriesLock& entriesLock);
    void scheduleProcessFrame(EntriesLock& entriesLock);
    void schedulePerformCleanup(CleanUpMode cleanUpMode);

    void doScheduleDraw();
    void doScheduleProcessFrame();
    void drawEntry(DrawLooperEntry& entry);

    bool processFrameForNextLayer(TimePoint currentFrameTime);

    DrawOperationsBatch collectDrawOperations();
    void drawOperationsBatch(const DrawOperationsBatch& drawOperations);
    bool needsDraw() const;
    void performCleanup(CleanUpMode cleanUpMode);
};

} // namespace snap::drawing
