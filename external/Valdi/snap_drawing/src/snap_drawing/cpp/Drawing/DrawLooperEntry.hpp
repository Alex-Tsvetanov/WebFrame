//
//  DrawLooperEntry.hpp
//  snap_drawing-pc
//
//  Created by Simon Corsin on 1/25/22.
//

#pragma once

#include "snap_drawing/cpp/Drawing/Composition/CompositorPlaneList.hpp"
#include "snap_drawing/cpp/Drawing/Surface/DrawableSurface.hpp"
#include "snap_drawing/cpp/Drawing/Surface/SurfacePresenterList.hpp"
#include "snap_drawing/cpp/Drawing/Surface/SurfacePresenterManager.hpp"
#include "snap_drawing/cpp/Layers/LayerRoot.hpp"
#include "valdi_core/cpp/Utils/SmallVector.hpp"

namespace snap::drawing {

class DrawLooperEntry;

class DrawLooperEntryListener {
public:
    virtual ~DrawLooperEntryListener() = default;

    virtual SurfacePresenterId createSurfacePresenterId() = 0;

    virtual void onNeedsProcessFrame(DrawLooperEntry& entry) = 0;

    virtual void onDidDraw(DrawLooperEntry& entry,
                           const Ref<DisplayList>& displayList,
                           const CompositorPlaneList* planeList) = 0;
};

class DrawOperation;

struct DrawLooperEntryDrawState {
    bool needsDraw = false;
    bool prefersSynchronousDraw = false;

    constexpr bool needsSynchronousDraw() const {
        return needsDraw && prefersSynchronousDraw;
    }
};

/**
 The DrawLooperEntry represents a LayerRoot that was added to a DrawLooper.
 It contains a SurfacePresenterManager, which is used to create presenters
 that will display the drawn content of the LayerRoot.
 The entry manages the presenters for the LayerRoot. It will call into the
 SurfacePresenterManager to create or update presenters when needed.
 The entry holds the display list that should be drawn next into the presenter.
 */
class DrawLooperEntry : public Valdi::SimpleRefCountable, public LayerRootListener {
public:
    DrawLooperEntry(const Ref<LayerRoot>& layerRoot,
                    const Ref<SurfacePresenterManager>& surfacePresenterManager,
                    DrawLooperEntryListener* listener);
    ~DrawLooperEntry() override;

    bool needsProcessFrameAtTime(TimePoint frameTime) const;
    DrawLooperEntryDrawState getDrawState() const;
    bool setPresenterNeedsDraw(SurfacePresenterId id);

    void setDisallowSynchronousDraw(bool disallowSynchronousDraw);

    void enqueueDisplayList(const Ref<DisplayList>& displayList);

    Ref<DrawOperation> makeDrawOperation(bool shouldSwapToNextFrame);

    const Ref<LayerRoot>& getLayerRoot() const;

    bool surfacePresentersNeedUpdate(const CompositorPlaneList& planeList) const;
    void updateSurfacePresenters(const CompositorPlaneList& planeList);
    void removeSurfacePresenters();

    bool setDrawableSurfaceForPresenterId(SurfacePresenterId id, const Ref<DrawableSurface>& drawableSurface);

    const SurfacePresenterList& getSurfacePresenters() const;

    void onNeedsProcessFrame(LayerRoot& root) override;

    void onDidDraw(LayerRoot& root, const Ref<DisplayList>& displayList, const CompositorPlaneList* planeList) override;

private:
    Ref<LayerRoot> _layerRoot;
    Ref<SurfacePresenterManager> _surfacePresenterManager;
    DrawLooperEntryListener* _listener;
    SurfacePresenterList _surfacePresenters;
    Ref<DisplayList> _displayList;
    bool _disallowSynchronousDraw = false;

    void updateSurfaceForPlane(const CompositorPlane& plane,
                               size_t zIndex,
                               bool* needSynchronousDraw,
                               size_t* displayListPlaneIndex);
    void createSurfaceForPlane(const CompositorPlane& plane,
                               size_t zIndex,
                               bool* needSynchronousDraw,
                               size_t* displayListPlaneIndex);

    bool tryReuseExternalSurface(const CompositorPlane& plane,
                                 size_t zIndex,
                                 size_t planesCount,
                                 bool* needSynchronousDraw,
                                 size_t* displayListPlaneIndex);
    bool tryReuseDrawableSurface(const CompositorPlane& plane,
                                 size_t zIndex,
                                 size_t planesCount,
                                 bool* needSynchronousDraw,
                                 size_t* displayListPlaneIndex);

    void moveSurfacePresenter(SurfacePresenterId id, size_t fromIndex, size_t toIndex, bool* needSynchronousDraw);
};

} // namespace snap::drawing
