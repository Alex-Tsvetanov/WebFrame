//
//  Compositor.cpp
//  snap_drawing-macos
//
//  Created by Simon Corsin on 2/1/22.
//

#include "snap_drawing/cpp/Drawing/Composition/Compositor.hpp"
#include "CompositionState.hpp"
#include "snap_drawing/cpp/Drawing/Composition/CompositorPlane.hpp"
#include "snap_drawing/cpp/Drawing/Composition/CompositorPlaneList.hpp"
#include "snap_drawing/cpp/Drawing/Composition/ResolvedPlane.hpp"
#include "snap_drawing/cpp/Drawing/Mask/IMask.hpp"
#include "snap_drawing/cpp/Drawing/Surface/ExternalSurface.hpp"
#include "utils/debugging/Assert.hpp"

#include "valdi_core/cpp/Utils/LoggerUtils.hpp"
#include "valdi_core/cpp/Utils/Trace.hpp"

namespace snap::drawing {

// Since we use a bitmask for the presence field in order to remain efficient,
// we support a maximum of 64 composited planes.
constexpr size_t kMaxSurfacesCount = sizeof(uint64_t) * 8;

// Represents a pushContext operation inside a DisplayList on which
// the clipRect, matrix and opacity have been resolved
struct VisitedContext {
    // Bitmask that tells which planes have this pushContext
    // operation been inserted into.
    uint64_t planePresenceField = 0;
    CompositionState compositionState;
    // The original pushContext operation from which this VisitedContext comes from
    const Operations::PushContext* pushContext;
    // The last clipping operation that has been set
    const Operations::ClipOperation* clipOperation = nullptr;

    inline VisitedContext(CompositionState&& compositionState, const Operations::PushContext* pushContext)
        : compositionState(std::move(compositionState)), pushContext(pushContext) {}

    template<typename F>
    inline void forEachPlaneIndex(F&& fn) {
        auto planePresenceField = this->planePresenceField;

        while (planePresenceField != 0) {
            // ctzl counts the number of zeroes before the first non zero bit,
            // which here tells us the planeIndex.
            // Can be replaced by std::countr_one once we compile using C++20
            auto planeIndex = static_cast<uint64_t>(__builtin_ctzll(planePresenceField));
            uint64_t t = planePresenceField & -planePresenceField;
            planePresenceField ^= t;
            fn(planeIndex);
        }
    }
};

struct SubmittedPrepareMask {
    IMask* mask;
    uint64_t planeIndex;

    SubmittedPrepareMask(IMask* mask, uint64_t planeIndex) : mask(mask), planeIndex(planeIndex) {}
};

class PopulatePlanesVisitor {
public:
    PopulatePlanesVisitor(DisplayList& displayList, Valdi::ILogger& logger)
        : _displayList(displayList),
          _displayListFrame(0, 0, displayList.getSize().width, displayList.getSize().height),
          _logger(logger) {
        appendContext(CompositionState(), nullptr);
    }

    void visit(const Operations::PushContext& pushContext) {
        auto& topContext = getCurrentContext();

        appendContext(topContext.compositionState.pushContext(pushContext.opacity, pushContext.matrix), &pushContext);
    }

    void visit(const Operations::PopContext& /*popContext*/) {
        auto& topContext = getCurrentContext();

        topContext.forEachPlaneIndex([&](uint64_t planeIndex) {
            setCurrentPlaneIndex(planeIndex);
            _displayList.popContext();
        });

        _visitedContexts.pop_back();
    }

    void visit(const Operations::ClipRect& clipRect) {
        auto& currentContext = getCurrentContext();

        currentContext.compositionState.clipRect(clipRect.width, clipRect.height);

        onClipUpdated(currentContext, clipRect);
    }

    void visit(const Operations::ClipRound& clipRound) {
        auto& currentContext = getCurrentContext();

        currentContext.compositionState.clipRound(clipRound.borderRadius, clipRound.width, clipRound.height);

        onClipUpdated(currentContext, clipRound);
    }

    void visit(const Operations::DrawPicture& drawPicture) {
        auto absoluteClippedPictureRect =
            resolveAbsoluteClippedRect(fromSkValue<Rect>(drawPicture.picture->cullRect()));

        auto& resolvedPlane = resolveRegularPlane(absoluteClippedPictureRect);
        appendDrawPictureToPlane(drawPicture, absoluteClippedPictureRect, resolvedPlane);
    }

    void visit(const Operations::DrawExternalSurface& drawExternalSurface) {
        auto& current = getCurrentContext();

        auto& externalSurface = drawExternalSurface.externalSurfaceSnapshot->getExternalSurface();

        auto externalSurfaceRect =
            Rect::makeXYWH(0, 0, externalSurface->getRelativeSize().width, externalSurface->getRelativeSize().height);

        auto absoluteOpacity = drawExternalSurface.opacity * current.compositionState.getAbsoluteOpacity();
        auto absoluteSurfaceRect = current.compositionState.getAbsoluteRect(externalSurfaceRect);

        appendExternalPlane(drawExternalSurface.externalSurfaceSnapshot,
                            current.compositionState.getAbsoluteMatrix(),
                            current.compositionState.getAbsoluteClipPath(),
                            absoluteOpacity,
                            absoluteSurfaceRect);
    }

    void visit(const Operations::PrepareMask& prepareMask) {
        auto absoluteClippedRect = resolveAbsoluteClippedRect(prepareMask.mask->getBounds());

        auto& resolvedPlane = resolveRegularPlane(absoluteClippedRect);

        resolvedPlane.bbox->insert(absoluteClippedRect);

        syncDisplayListWithPlaneIfNeeded(resolvedPlane);
        _displayList.appendPrepareMask(prepareMask.mask);

        _submittedPrepareMasks.emplace_back(prepareMask.mask, resolvedPlane.planeIndex);
    }

    void visit(const Operations::ApplyMask& applyMask) {
        auto index = _submittedPrepareMasks.size();
        while (index > 0) {
            index--;
            auto& it = _submittedPrepareMasks[index];

            if (it.mask == applyMask.mask) {
                setCurrentPlaneIndex(it.planeIndex);
                _displayList.appendApplyMask(applyMask.mask);
                _submittedPrepareMasks.erase(_submittedPrepareMasks.begin() + index);
                return;
            }
        }
    }

    const Valdi::SmallVector<ResolvedPlane, 2>& getResolvedPlanes() const {
        return _resolvedPlanes;
    }

private:
    [[maybe_unused]] DisplayList& _displayList;
    Rect _displayListFrame;
    [[maybe_unused]] Valdi::ILogger& _logger;
    Valdi::SmallVector<VisitedContext, 8> _visitedContexts;
    Valdi::SmallVector<ResolvedPlane, 2> _resolvedPlanes;
    Valdi::SmallVector<SubmittedPrepareMask, 2> _submittedPrepareMasks;
    std::vector<int> _bboxSearchResult;
    uint64_t _planeIndexSequence = 0;
    uint64_t _currentDisplayListPlaneIndex = 0;

    Rect resolveAbsoluteClippedRect(const Rect& relativeRect) {
        return getCurrentContext().compositionState.getAbsoluteClippedRect(relativeRect);
    }

    void onClipUpdated(VisitedContext& currentContext, const Operations::ClipOperation& clipOperation) {
        currentContext.clipOperation = &clipOperation;

        if (currentContext.planePresenceField != 0) {
            // If our context was already inserted into a plane,
            // we need to update the new clip for those planes
            syncClipWithPlanes(currentContext);
        }
    }

    VisitedContext& getCurrentContext() {
        return _visitedContexts[_visitedContexts.size() - 1];
    }

    void appendContext(CompositionState&& compositionState, const Operations::PushContext* pushContext) {
        _visitedContexts.emplace_back(std::move(compositionState), pushContext);
    }

    void appendDrawPictureToPlane(const Operations::DrawPicture& drawPicture,
                                  const Rect& frame,
                                  ResolvedRegularPlane& plane) {
        plane.bbox->insert(frame);

        syncDisplayListWithPlaneIfNeeded(plane);
        _displayList.appendPicture(drawPicture.picture, drawPicture.opacity);
    }

    void setCurrentPlaneIndex(uint64_t planeIndex) {
        if (_currentDisplayListPlaneIndex != planeIndex) {
            _displayList.setCurrentPlane(static_cast<size_t>(planeIndex));
            _currentDisplayListPlaneIndex = planeIndex;
        }
    }

    std::pair<VisitedContext*, VisitedContext*> findContextsToPush(ResolvedRegularPlane& plane) {
        auto planeBit = static_cast<uint64_t>(1) << plane.planeIndex;

        auto visitedContextsSize = _visitedContexts.size();

        VisitedContext* begin = _visitedContexts.data() + visitedContextsSize;
        VisitedContext* end = begin;

        auto i = visitedContextsSize;

        while (i > 1 /* The index at 0 is a placeholder */) {
            i--;

            auto& pushContext = _visitedContexts[i];

            if ((pushContext.planePresenceField & planeBit) == 0) {
                // Push operation has not been recorded in the DisplayList for that plane
                pushContext.planePresenceField |= planeBit;
                begin = &pushContext;
            } else {
                break;
            }
        }

        return std::make_pair(begin, end);
    }

    void syncClipWithPlanes(VisitedContext& pushContext) {
        pushContext.forEachPlaneIndex([&](uint64_t planeIndex) {
            setCurrentPlaneIndex(planeIndex);
            appendClipOperation(pushContext);
        });
    }

    void appendClipOperation(VisitedContext& pushContext) {
        if (pushContext.clipOperation->type == Operations::ClipRound::kId) {
            const auto& borderRadius =
                reinterpret_cast<const Operations::ClipRound*>(pushContext.clipOperation)->borderRadius;
            _displayList.appendClipRound(
                borderRadius, pushContext.clipOperation->width, pushContext.clipOperation->height);
        } else {
            _displayList.appendClipRect(pushContext.clipOperation->width, pushContext.clipOperation->height);
        }
    }

    void syncDisplayListWithPlaneIfNeeded(ResolvedRegularPlane& plane) {
        setCurrentPlaneIndex(plane.planeIndex);

        auto iterator = findContextsToPush(plane);

        auto* it = iterator.first;
        while (it != iterator.second) {
            auto& pushContext = *it;

            _displayList.pushContext(pushContext.pushContext->matrix,
                                     pushContext.pushContext->opacity,
                                     pushContext.pushContext->layerId,
                                     pushContext.pushContext->hasUpdates);

            if (pushContext.clipOperation != nullptr) {
                appendClipOperation(pushContext);
            }

            it++;
        }
    }

    ResolvedPlane& getTopPlane() {
        return _resolvedPlanes[_resolvedPlanes.size() - 1];
    }

    ResolvedRegularPlane& appendRegularPlane() {
        auto planeIndex = _planeIndexSequence++;

        while (static_cast<size_t>(planeIndex) >= _displayList.getPlanesCount()) {
            _displayList.appendPlane();
            _currentDisplayListPlaneIndex = planeIndex;
        }

        auto& plane = _resolvedPlanes.emplace_back(planeIndex, Valdi::makeShared<BoundingBoxHierarchy>());
        return *plane.getRegular();
    }

    size_t resolveExternalPlaneInsertionIndex(const Rect& absoluteFrame) {
        auto planesCount = _resolvedPlanes.size();
        auto bestInsertionIndex = planesCount;

        size_t i = planesCount;
        while (i > 0) {
            i--;
            auto& plane = _resolvedPlanes[i];

            auto* regularPlane = plane.getRegular();
            if (regularPlane != nullptr) {
                if (regularPlane->bbox->contains(absoluteFrame)) {
                    break;
                } else {
                    bestInsertionIndex = i;
                }
            } else {
                break;
            }
        }

        return bestInsertionIndex;
    }

    ResolvedExternalPlane& appendExternalPlane(ExternalSurfaceSnapshot* externalSurfaceSnapshot,
                                               const Matrix& transform,
                                               const Path& clipPath,
                                               Scalar opacity,
                                               const Rect& absoluteFrame) {
        auto insertionIndex = resolveExternalPlaneInsertionIndex(absoluteFrame);
        auto it = _resolvedPlanes.emplace(_resolvedPlanes.begin() + insertionIndex,
                                          externalSurfaceSnapshot,
                                          transform,
                                          clipPath,
                                          opacity,
                                          absoluteFrame);
        return *it->getExternal();
    }

    ResolvedRegularPlane* getTopRegularPlane() {
        size_t index = _resolvedPlanes.size();
        while (index > 0) {
            index--;

            auto& plane = _resolvedPlanes[index];
            auto* regularPlane = plane.getRegular();
            if (regularPlane != nullptr) {
                return regularPlane;
            }
        }

        return nullptr;
    }

    ResolvedRegularPlane& resolveRegularPlane(const Rect& absoluteFrame) {
        ResolvedRegularPlane* bestPlane = nullptr;

        // The algorithm goes from top plane to bottom, and finds the lowest plane
        // that is not an external plane and can host the given frame.

        size_t index = _resolvedPlanes.size();
        while (index > 0) {
            index--;

            auto& plane = _resolvedPlanes[index];

            auto* externalSurface = plane.getExternal();

            if (externalSurface != nullptr) {
                if (externalSurface->absoluteFrame.intersects(absoluteFrame)) {
                    // We are intersecting with an external plane.
                    // Stop the search to decide whether to use a plane
                    // above us or append a new one
                    break;
                }
            } else {
                if (bestPlane != nullptr && bestPlane->bbox->contains(absoluteFrame)) {
                    // A plane has already some things that intersects with our frame.
                    // We can't use a lower plane than this one
                    break;
                }
                bestPlane = plane.getRegular();
            }
        }

        if (bestPlane != nullptr) {
            return *bestPlane;
        }

        if (_planeIndexSequence < kMaxSurfacesCount) {
            return appendRegularPlane();
        }

        // Cannot create more planes, we fallback on re-using the top one
        auto* topPlane = getTopRegularPlane();
        SC_ASSERT_NOTNULL(topPlane);
        return *topPlane;
    }
};

Compositor::Compositor(Valdi::ILogger& logger) : _logger(logger) {}

Ref<DisplayList> Compositor::performComposition(DisplayList& sourceDisplayList, CompositorPlaneList& planeList) {
    if (!sourceDisplayList.hasExternalSurfaces()) {
        planeList.appendDrawableSurface();
        return Ref<DisplayList>(&sourceDisplayList);
    }

    VALDI_TRACE("SnapDrawing.performComposition");

    auto outputDisplayList =
        Valdi::makeShared<DisplayList>(sourceDisplayList.getSize(), sourceDisplayList.getFrameTime());

    PopulatePlanesVisitor visitor(*outputDisplayList, _logger);
    sourceDisplayList.visitOperations(kDisplayListAllPlaneIndexes, visitor);

    for (const auto& plane : visitor.getResolvedPlanes()) {
        const auto* resolvedExternalSurface = plane.getExternal();
        if (resolvedExternalSurface != nullptr) {
            planeList.appendPlane(
                CompositorPlane(Ref<ExternalSurfaceSnapshot>(resolvedExternalSurface->externalSurface),
                                resolvedExternalSurface->resolveDisplayState()));
        } else {
            planeList.appendDrawableSurface();
        }
    }

    return outputDisplayList;
}

} // namespace snap::drawing
