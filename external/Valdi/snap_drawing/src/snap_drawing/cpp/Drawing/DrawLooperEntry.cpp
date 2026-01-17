//
//  DrawLooperEntry.cpp
//  snap_drawing-pc
//
//  Created by Simon Corsin on 1/25/22.
//

#include "snap_drawing/cpp/Drawing/DrawLooperEntry.hpp"
#include "snap_drawing/cpp/Drawing/DrawOperation.hpp"
#include "valdi_core/cpp/Utils/Trace.hpp"
#include <bitset>

namespace snap::drawing {

DrawLooperEntry::DrawLooperEntry(const Ref<LayerRoot>& layerRoot,
                                 const Ref<SurfacePresenterManager>& surfacePresenterManager,
                                 DrawLooperEntryListener* listener)
    : _layerRoot(layerRoot), _surfacePresenterManager(surfacePresenterManager), _listener(listener) {}

DrawLooperEntry::~DrawLooperEntry() = default;

DrawLooperEntryDrawState DrawLooperEntry::getDrawState() const {
    DrawLooperEntryDrawState drawState;
    if (_displayList == nullptr) {
        return drawState;
    }

    auto frameTime = _displayList->getFrameTime();

    for (const auto& surfacePresenter : _surfacePresenters) {
        if (surfacePresenter.needsDrawForFrameTime(frameTime)) {
            drawState.needsDraw = true;
            if (surfacePresenter.needsSynchronousDraw()) {
                drawState.prefersSynchronousDraw = true;
            }
        }
    }

    return drawState;
}

bool DrawLooperEntry::needsProcessFrameAtTime(TimePoint frameTime) const {
    return _layerRoot->needsProcessFrame() &&
           (!_layerRoot->getLastAbsoluteFrameTime() || _layerRoot->getLastAbsoluteFrameTime().value() != frameTime);
}

void DrawLooperEntry::enqueueDisplayList(const Ref<DisplayList>& displayList) {
    _displayList = displayList;
}

Ref<DrawOperation> DrawLooperEntry::makeDrawOperation(bool shouldSwapToNextFrame) {
    SurfacePresenterList surfacePresenters;

    if (shouldSwapToNextFrame) {
        if (_displayList != nullptr) {
            auto frameTime = _displayList->getFrameTime();
            for (auto& presenter : _surfacePresenters) {
                if (presenter.needsDrawForFrameTime(frameTime)) {
                    presenter.setLastDrawnFrameTime({frameTime});
                    presenter.setNeedsSynchronousDraw(false);

                    auto& presenterToDraw = surfacePresenters.append(presenter.getId());
                    presenterToDraw = presenter;
                }
            }
        }
    } else {
        surfacePresenters = _surfacePresenters;
    }

    return Valdi::makeShared<DrawOperation>(_displayList, _surfacePresenterManager, std::move(surfacePresenters));
}

const Ref<LayerRoot>& DrawLooperEntry::getLayerRoot() const {
    return _layerRoot;
}

bool DrawLooperEntry::surfacePresentersNeedUpdate(const CompositorPlaneList& planeList) const {
    if (_surfacePresenters.size() != planeList.getPlanesCount()) {
        return true;
    }

    size_t i = 0;
    for (const auto& surfacePresenter : _surfacePresenters) {
        const auto& plane = planeList.getPlaneAtIndex(i);

        switch (plane.getType()) {
            case CompositorPlaneTypeDrawable: {
                if (!surfacePresenter.isDrawable()) {
                    return true;
                }
            } break;
            case CompositorPlaneTypeExternal: {
                if (surfacePresenter.getExternalSurface() !=
                    plane.getExternalSurfaceSnapshot()->getExternalSurface().get()) {
                    return true;
                }
                if (*surfacePresenter.getExternalSurfacePresenterState() != *plane.getExternalSurfacePresenterState()) {
                    return true;
                }
            } break;
        }

        i++;
    }

    return false;
}

bool DrawLooperEntry::setPresenterNeedsDraw(SurfacePresenterId id) {
    auto* presenter = _surfacePresenters.getForId(id);
    if (presenter == nullptr) {
        return false;
    }

    presenter->setLastDrawnFrameTime(std::nullopt);

    return true;
}

void DrawLooperEntry::setDisallowSynchronousDraw(bool disallowSynchronousDraw) {
    _disallowSynchronousDraw = disallowSynchronousDraw;
}

void DrawLooperEntry::removeSurfacePresenters() {
    updateSurfacePresenters(CompositorPlaneList());
}

void DrawLooperEntry::updateSurfacePresenters(const CompositorPlaneList& planeList) {
    auto newPlanesCount = planeList.getPlanesCount();
    size_t displayListPlaneIndex = 0;
    bool needSynchronousDraw = false;

    for (size_t i = 0; i < newPlanesCount; i++) {
        const auto& plane = planeList.getPlaneAtIndex(i);
        updateSurfaceForPlane(plane, i, &needSynchronousDraw, &displayListPlaneIndex);
    }

    while (_surfacePresenters.size() > newPlanesCount) {
        auto indexToRemove = _surfacePresenters.size() - 1;
        auto surfacePresenter = std::move(_surfacePresenters[indexToRemove]);
        _surfacePresenters.remove(indexToRemove);

        {
            VALDI_TRACE("SnapDrawing.removeSurfacePresenter");
            _surfacePresenterManager->removeSurfacePresenter(surfacePresenter.getId());
        }

        needSynchronousDraw = true;
    }

    if (needSynchronousDraw && !_disallowSynchronousDraw) {
        for (auto& presenter : _surfacePresenters) {
            if (presenter.isDrawable()) {
                presenter.setNeedsSynchronousDraw(true);
            }
        }
    }
}

bool DrawLooperEntry::setDrawableSurfaceForPresenterId(SurfacePresenterId id,
                                                       const Ref<DrawableSurface>& drawableSurface) {
    auto* surfacePresenter = _surfacePresenters.getForId(id);
    if (surfacePresenter == nullptr) {
        return false;
    }
    if (surfacePresenter->getDrawableSurface() != drawableSurface.get()) {
        surfacePresenter->setSurface(drawableSurface.get());
        surfacePresenter->setLastDrawnFrameTime(std::nullopt);
    }

    return true;
}

void DrawLooperEntry::createSurfaceForPlane(const CompositorPlane& plane,
                                            size_t zIndex,
                                            bool* needSynchronousDraw,
                                            size_t* displayListPlaneIndex) {
    auto surfacePresenterId = _listener->createSurfacePresenterId();

    SurfacePresenter& surfacePresenter = _surfacePresenters.insert(zIndex, surfacePresenterId);

    switch (plane.getType()) {
        case CompositorPlaneTypeDrawable: {
            surfacePresenter.setDisplayListPlaneIndex((*displayListPlaneIndex)++);
            surfacePresenter.setAsDrawable();
            VALDI_TRACE("SnapDrawing.createDrawablePresenter");
            auto drawableSurface =
                _surfacePresenterManager->createPresenterWithDrawableSurface(surfacePresenterId, zIndex);
            if (drawableSurface != nullptr) {
                surfacePresenter.setSurface(drawableSurface.get());
            }
            if (_surfacePresenters.size() > 1 && drawableSurface == nullptr) {
                // If we already had a presenter before creating this one and that the new presenter
                // doesn't have a surface readily available, we will need a synchronous draw once
                // the surface comes in to synchronize the surfaces together
                *needSynchronousDraw = true;
            }
        } break;
        case CompositorPlaneTypeExternal: {
            surfacePresenter.setDisplayListPlaneIndex(*displayListPlaneIndex);
            auto* externalSurfaceSnapshot = plane.getExternalSurfaceSnapshot();
            const auto& presenterState = *plane.getExternalSurfacePresenterState();
            surfacePresenter.setAsExternal(*externalSurfaceSnapshot->getExternalSurface(), presenterState);

            VALDI_TRACE("SnapDrawing.createExternalPresenter");
            _surfacePresenterManager->createPresenterForExternalSurface(
                surfacePresenterId, zIndex, *externalSurfaceSnapshot->getExternalSurface());
            _surfacePresenterManager->setExternalSurfacePresenterState(surfacePresenterId, presenterState, nullptr);
            *needSynchronousDraw = true;
        } break;
    }
}

void DrawLooperEntry::moveSurfacePresenter(SurfacePresenterId id,
                                           size_t fromIndex,
                                           size_t toIndex,
                                           bool* needSynchronousDraw) {
    _surfacePresenters.move(fromIndex, toIndex);
    _surfacePresenterManager->setSurfacePresenterZIndex(id, toIndex);
    *needSynchronousDraw = true;
}

bool DrawLooperEntry::tryReuseDrawableSurface(const CompositorPlane& /*plane*/,
                                              size_t zIndex,
                                              size_t planesCount,
                                              bool* needSynchronousDraw,
                                              size_t* displayListPlaneIndex) {
    for (size_t i = zIndex; i < planesCount; i++) {
        auto& existingSurfacePresenter = _surfacePresenters[i];
        if (existingSurfacePresenter.isDrawable()) {
            // We found a surface we can use
            existingSurfacePresenter.setDisplayListPlaneIndex((*displayListPlaneIndex)++);

            if (i != zIndex) {
                // zIndex has changed
                auto surfacePresenterId = existingSurfacePresenter.getId();
                moveSurfacePresenter(surfacePresenterId, i, zIndex, needSynchronousDraw);
            }
            return true;
        }
    }

    return false;
}

bool DrawLooperEntry::tryReuseExternalSurface(
    const CompositorPlane& plane,
    size_t zIndex,
    size_t planesCount,
    bool* needSynchronousDraw,
    size_t* displayListPlaneIndex) { // NOLINT(readability-non-const-parameter)
    auto* externalSurface = plane.getExternalSurfaceSnapshot()->getExternalSurface().get();
    const auto& presenterState = *plane.getExternalSurfacePresenterState();

    for (size_t i = zIndex; i < planesCount; i++) {
        auto& existingSurfacePresenter = _surfacePresenters[i];
        if (existingSurfacePresenter.getExternalSurface() == externalSurface) {
            existingSurfacePresenter.setDisplayListPlaneIndex((*displayListPlaneIndex));

            auto* externalSurfacePresenterPtr = existingSurfacePresenter.getExternalSurfacePresenterState();

            if (*externalSurfacePresenterPtr != presenterState) {
                // DisplayState has changed, update it and notify the SurfacePresenterManager
                auto oldExternalSurfacePresenterState = std::move(*externalSurfacePresenterPtr);
                *externalSurfacePresenterPtr = presenterState;

                _surfacePresenterManager->setExternalSurfacePresenterState(
                    existingSurfacePresenter.getId(), presenterState, &oldExternalSurfacePresenterState);
                *needSynchronousDraw = true;
            }

            if (i != zIndex) {
                // zIndex has changed
                moveSurfacePresenter(existingSurfacePresenter.getId(), i, zIndex, needSynchronousDraw);
            }

            return true;
        }
    }

    return false;
}

const SurfacePresenterList& DrawLooperEntry::getSurfacePresenters() const {
    return _surfacePresenters;
}

void DrawLooperEntry::updateSurfaceForPlane(const CompositorPlane& plane,
                                            size_t zIndex,
                                            bool* needSynchronousDraw,
                                            size_t* displayListPlaneIndex) {
    auto existingPlanesCount = _surfacePresenters.size();

    if (zIndex >= existingPlanesCount) {
        createSurfaceForPlane(plane, zIndex, needSynchronousDraw, displayListPlaneIndex);
        return;
    }

    switch (plane.getType()) {
        case CompositorPlaneTypeDrawable:
            if (!tryReuseDrawableSurface(
                    plane, zIndex, existingPlanesCount, needSynchronousDraw, displayListPlaneIndex)) {
                createSurfaceForPlane(plane, zIndex, needSynchronousDraw, displayListPlaneIndex);
            }
            break;
        case CompositorPlaneTypeExternal:
            if (!tryReuseExternalSurface(
                    plane, zIndex, existingPlanesCount, needSynchronousDraw, displayListPlaneIndex)) {
                createSurfaceForPlane(plane, zIndex, needSynchronousDraw, displayListPlaneIndex);
            }
            break;
    }
}

void DrawLooperEntry::onNeedsProcessFrame(LayerRoot& /*root*/) {
    _listener->onNeedsProcessFrame(*this);
}

void DrawLooperEntry::onDidDraw(LayerRoot& /*root*/,
                                const Ref<DisplayList>& displayList,
                                const CompositorPlaneList* planeList) {
    _listener->onDidDraw(*this, displayList, planeList);
}

} // namespace snap::drawing
