//
//  SkiaTouchDispatcher.cpp
//  valdi-skia-apple
//
//  Created by Simon Corsin on 6/27/20.
//

#include "snap_drawing/cpp/Touches/TouchDispatcher.hpp"

#include "valdi_core/cpp/Utils/LoggerUtils.hpp"

namespace snap::drawing {

#define TOUCHDISPATCHER_DEBUG(____format, ...)                                                                         \
    if (_enableLogging) {                                                                                              \
        VALDI_INFO(_logger, "TouchDispatcher - {}", fmt::format(____format VALDI_LOG_ARGS(__VA_ARGS__)));              \
    }

TouchDispatcher::TouchDispatcher(Valdi::ILogger& logger, bool enableLogging)
    : _logger(logger), _enableLogging(enableLogging) {}

static bool removeGestureRecognizer(std::vector<Valdi::Ref<GestureRecognizer>>& gestures,
                                    const Valdi::Ref<GestureRecognizer>& gestureRecognizer) {
    auto candidateIt = std::find(gestures.begin(), gestures.end(), gestureRecognizer);
    if (candidateIt == gestures.end()) {
        return false;
    }
    gestures.erase(candidateIt);
    return true;
}

static size_t indexOfGestureRecognizer(const std::vector<Valdi::Ref<GestureRecognizer>>& gestures,
                                       const Valdi::Ref<GestureRecognizer>& gestureRecognizer) {
    return std::find(gestures.begin(), gestures.end(), gestureRecognizer) - gestures.begin();
}

void TouchDispatcher::cancelAllGestures() {
    while (!_candidateGestureRecognizers.empty()) {
        auto gestureRecognizer = _candidateGestureRecognizers[_candidateGestureRecognizers.size() - 1];
        cancelGestureRecognizer(gestureRecognizer);
    }
}

bool TouchDispatcher::dispatchEvent(const TouchEvent& event, const Valdi::Ref<Layer>& rootLayer) {
    _dispatchingEvent = true;
    _lastEvent = event;
    TOUCHDISPATCHER_DEBUG("Dispatching event '{}'", event);

    if (event.getType() == TouchEventTypeDown || event.getType() == TouchEventTypeWheel) {
        // Step 1, we capture all the layers and their gesture recognizers which are within the event's target.
        // We only do this on touch down.
        [[maybe_unused]] auto sizeBefore = _candidateGestureRecognizers.size();
        captureCandidates(event, rootLayer, _candidateGestureRecognizers);

        if (_enableLogging) {
            TOUCHDISPATCHER_DEBUG("Captured {} new gestures candidates (total {})",
                                  _candidateGestureRecognizers.size() - sizeBefore,
                                  _candidateGestureRecognizers.size());
            for (size_t i = 0; i < _candidateGestureRecognizers.size(); i++) {
                TOUCHDISPATCHER_DEBUG("Candidate gesture #{}: {}", i, _candidateGestureRecognizers[i]->toDebugString());
            }
        }
    }

    auto processed = processGestureRecognizers(rootLayer);
    _dispatchingEvent = false;
    return processed;
}

std::vector<Valdi::Ref<GestureRecognizer>> TouchDispatcher::getGestureCandidatesForEvent(
    const TouchEvent& event, const Valdi::Ref<Layer>& rootLayer) const {
    std::vector<Valdi::Ref<GestureRecognizer>> candidateGestures;

    captureCandidates(event, rootLayer, candidateGestures);

    return candidateGestures;
}

bool TouchDispatcher::containsGestureRecognizer(const Valdi::Ref<GestureRecognizer>& gestureRecognizer,
                                                const std::vector<Valdi::Ref<GestureRecognizer>>& gestureRecognizers) {
    return std::find(gestureRecognizers.begin(), gestureRecognizers.end(), gestureRecognizer) !=
           gestureRecognizers.end();
}

bool TouchDispatcher::captureCandidates(const TouchEvent& event,
                                        const Valdi::Ref<Layer>& layer,
                                        std::vector<Valdi::Ref<GestureRecognizer>>& candidateGestureRecognizers) const {
    if (!layer->hitTest(event.getLocation())) {
        return false;
    }

    size_t gestureRecognizerSize = layer->getGestureRecognizersSize();
    for (size_t i = 0; i < gestureRecognizerSize; i++) {
        auto gestureRecognizer = layer->getGestureRecognizer(i);
        if (!containsGestureRecognizer(gestureRecognizer, candidateGestureRecognizers)) {
            if (gestureRecognizer->shouldProcessBeforeOtherGestures()) {
                size_t insertionIndex = 0;
                while (insertionIndex < candidateGestureRecognizers.size() &&
                       candidateGestureRecognizers[i]->shouldProcessBeforeOtherGestures()) {
                    insertionIndex++;
                }
                candidateGestureRecognizers.emplace(candidateGestureRecognizers.begin() + insertionIndex,
                                                    std::move(gestureRecognizer));
            } else {
                candidateGestureRecognizers.emplace_back(std::move(gestureRecognizer));
            }
        }
    }

    // Dispatch touches to children, starting from the last child.
    auto i = layer->getChildrenSize();
    while (i > 0) {
        i--;

        auto child = layer->getChild(i);

        auto childPoint = child->convertPointFromParent(event.getLocation());

        if (captureCandidates(event.withLocation(childPoint), child, candidateGestureRecognizers)) {
            // Among siblings, we only capture the first sibling which is hit.
            break;
        }
    }

    return true;
}

bool TouchDispatcher::processGestureRecognizers(const Valdi::Ref<Layer>& rootLayer) {
    if (!_lastEvent) {
        return false;
    }

    // Step 2, we update all the active gesture recognizers. They will update their states.
    updateGestureRecognizers(rootLayer);

    // Step 3, we go over all the gesture recognizers which want to start. We resolve the conflicts.
    // For each gesture which wants to start, we will ask any other active gestures or gestures which want to
    // start if they agree that this gesture should be allowed to start. If any one the gesture says no, the
    // gesture will not start. As such we go from the back, as the gestures which are at the front are deepest
    // in the hierarchy, they should be prioritized and canceled only as a last resort.
    startPendingGestureRecognizers();

    // Step 4, we process all active gesture recognizers,
    processActiveGestureRecognizers();

    return !_candidateGestureRecognizers.empty();
}

void TouchDispatcher::updateGestureRecognizers(const Valdi::Ref<Layer>& rootLayer) {
    TOUCHDISPATCHER_DEBUG("Updating {} gesture candidates", _candidateGestureRecognizers.size());

    auto candidateIt = _candidateGestureRecognizers.begin();
    while (candidateIt != _candidateGestureRecognizers.end()) {
        auto gestureRecognizer = *candidateIt;
        auto shouldCancel = false;

        if (!isGestureRecognizerDeferred(gestureRecognizer)) {
            auto childEvent =
                adjustEventCoordinatesToLayer(rootLayer, gestureRecognizer->getLayer(), _lastEvent.value());
            if (childEvent) {
                TOUCHDISPATCHER_DEBUG("Calling update() on {}", gestureRecognizer->toDebugString());

                auto previousState = gestureRecognizer->getState();
                gestureRecognizer->update(childEvent.value());

                if (gestureRecognizer->getState() == GestureRecognizerStateFailed) {
                    shouldCancel = true;
                    TOUCHDISPATCHER_DEBUG("failed after update on {}", gestureRecognizer->toDebugString());
                } else if (gestureRecognizer->getState() == GestureRecognizerStateBegan ||
                           (previousState == GestureRecognizerStatePossible &&
                            gestureRecognizer->getState() == GestureRecognizerStateEnded)) {
                    _gestureRecognizersToStart.emplace_back(gestureRecognizer);
                    TOUCHDISPATCHER_DEBUG("{} is requesting to start", gestureRecognizer->toDebugString());
                }
            } else {
                // Cancelling gesture since the layer is not in the layer hierarchy anymore
                shouldCancel = true;
                TOUCHDISPATCHER_DEBUG("{} is no longer in hierarchy", gestureRecognizer->toDebugString());
            }
        } else {
            TOUCHDISPATCHER_DEBUG("{} is deferred", gestureRecognizer->toDebugString());
        }

        if (shouldCancel) {
            TOUCHDISPATCHER_DEBUG("Canceling {}", gestureRecognizer->toDebugString());
            gestureRecognizer->cancel();
            candidateIt = _candidateGestureRecognizers.erase(candidateIt);
        } else {
            candidateIt++;
        }
    }
}

void TouchDispatcher::cancelGestureRecognizer(const Valdi::Ref<GestureRecognizer>& gestureRecognizer) {
    TOUCHDISPATCHER_DEBUG("Canceling {}", gestureRecognizer->toDebugString());
    removeGestureRecognizer(_candidateGestureRecognizers, gestureRecognizer);
    removeGestureRecognizer(_gestureRecognizersToStart, gestureRecognizer);

    gestureRecognizer->cancel();
}

void TouchDispatcher::cancelGestureRecognizersBeforeIndex(size_t index) {
    index++;
    for (; index < _candidateGestureRecognizers.size(); index++) {
        auto gestureRecognizer = _candidateGestureRecognizers[index];
        cancelGestureRecognizer(gestureRecognizer);
    }
}

void TouchDispatcher::startPendingGestureRecognizers() {
    auto index = _gestureRecognizersToStart.size();
    while (index > 0) {
        index--;

        auto gestureRecognizer = _gestureRecognizersToStart[index];

        TOUCHDISPATCHER_DEBUG("Processing gesture requesting start on {}", gestureRecognizer->toDebugString());

        auto shouldStart = true;
        auto shouldDefer = false;

        auto downIndex = index;
        while (downIndex > 0) {
            downIndex--;
            auto conflictingGestureRecognizer = _gestureRecognizersToStart[downIndex];
            if (!canRecognizeSimultaneously(gestureRecognizer, conflictingGestureRecognizer)) {
                if (conflictingGestureRecognizer->requiresFailureOf(*gestureRecognizer)) {
                    // The conflicting gesture required failure of this gesture but this gesture succeeded, so we cancel
                    // the conflicting gesture.
                    TOUCHDISPATCHER_DEBUG("Cancelling conflicting gesture {} with {}",
                                          conflictingGestureRecognizer->toDebugString(),
                                          gestureRecognizer->toDebugString());

                    conflictingGestureRecognizer->cancel();
                    _gestureRecognizersToStart.erase(_gestureRecognizersToStart.begin() + downIndex);
                    removeGestureRecognizer(_candidateGestureRecognizers, conflictingGestureRecognizer);

                    index--;
                } else {
                    shouldStart = false;
                    TOUCHDISPATCHER_DEBUG("{} conflicts with {}, waiting for resolution",
                                          conflictingGestureRecognizer->toDebugString(),
                                          gestureRecognizer->toDebugString());
                    break;
                }
            }
        }

        if (shouldStart) {
            // Also look up in the active gesture recognizers
            for (const auto& activeGestureRecognizer : _candidateGestureRecognizers) {
                if (activeGestureRecognizer == gestureRecognizer) {
                    continue;
                }

                if (activeGestureRecognizer->getState() == GestureRecognizerStateChanged ||
                    activeGestureRecognizer->getState() == GestureRecognizerStateEnded) {
                    if (!canRecognizeSimultaneously(gestureRecognizer, activeGestureRecognizer)) {
                        shouldStart = false;
                        TOUCHDISPATCHER_DEBUG("Cannot start {} due to conflict with active gesture {}",
                                              gestureRecognizer->toDebugString(),
                                              activeGestureRecognizer->toDebugString());
                        break;
                    }
                } else if (activeGestureRecognizer->getState() == GestureRecognizerStatePossible) {
                    if (gestureRecognizer->requiresFailureOf(*activeGestureRecognizer)) {
                        shouldDefer = true;
                        TOUCHDISPATCHER_DEBUG("Deferring start of {} due to conflict with candidate {}",
                                              gestureRecognizer->toDebugString(),
                                              activeGestureRecognizer->toDebugString());
                        break;
                    }
                }
            }
        }

        if (!shouldDefer) {
            removeGestureRecognizer(_gestureRecognizersToStart, gestureRecognizer);

            if (shouldStart) {
                if (gestureRecognizer->shouldCancelOtherGesturesOnStart()) {
                    cancelGestureRecognizersBeforeIndex(
                        indexOfGestureRecognizer(_candidateGestureRecognizers, gestureRecognizer));
                    index = 0;
                }
                TOUCHDISPATCHER_DEBUG("Starting {}", gestureRecognizer->toDebugString());

                gestureRecognizer->onStarted();
            } else {
                cancelGestureRecognizer(gestureRecognizer);
            }
        }
    }
}

void TouchDispatcher::processActiveGestureRecognizers() {
    auto it = _candidateGestureRecognizers.begin();
    while (it != _candidateGestureRecognizers.end()) {
        const auto& gestureRecognizer = *it;

        if (!isGestureRecognizerDeferred(gestureRecognizer) && gestureRecognizer->isActive()) {
            TOUCHDISPATCHER_DEBUG("Processing active {}", gestureRecognizer->toDebugString());

            gestureRecognizer->process();

            if (gestureRecognizer->getState() == GestureRecognizerStateEnded) {
                TOUCHDISPATCHER_DEBUG("Gesture completed {}", gestureRecognizer->toDebugString());

                gestureRecognizer->cancel();
                it = _candidateGestureRecognizers.erase(it);
                continue;
            }
        } else {
            TOUCHDISPATCHER_DEBUG("Candidate gesture is not active {}", gestureRecognizer->toDebugString());
        }

        it++;
    }
}

bool TouchDispatcher::isGestureRecognizerDeferred(const Valdi::Ref<GestureRecognizer>& gestureRecognizer) const {
    return std::find(_gestureRecognizersToStart.begin(), _gestureRecognizersToStart.end(), gestureRecognizer) !=
           _gestureRecognizersToStart.end();
}

bool TouchDispatcher::canRecognizeSimultaneously(const Valdi::Ref<GestureRecognizer>& leftGestureRecognizer,
                                                 const Valdi::Ref<GestureRecognizer>& rightGestureRecognizer) {
    if (leftGestureRecognizer->canRecognizeSimultaneously(*rightGestureRecognizer)) {
        return true;
    }
    if (rightGestureRecognizer->canRecognizeSimultaneously(*leftGestureRecognizer)) {
        return true;
    }
    return false;
}

std::optional<TouchEvent> TouchDispatcher::adjustEventCoordinatesToLayer(const Valdi::Ref<Layer>& rootLayer,
                                                                         const Valdi::Ref<ILayer>& childLayer,
                                                                         const TouchEvent& event) {
    auto typedChildLayer = Valdi::castOrNull<Layer>(childLayer);
    if (typedChildLayer == nullptr) {
        return std::nullopt;
    }

    auto convertedPoint = rootLayer->convertPointToLayer(event.getLocation(), typedChildLayer);
    if (!convertedPoint) {
        return std::nullopt;
    }

    return event.withLocation(convertedPoint.value());
}

bool TouchDispatcher::isDispatchingEvent() const {
    return _dispatchingEvent;
}

bool TouchDispatcher::isEmpty() const {
    return _candidateGestureRecognizers.empty() && _gestureRecognizersToStart.empty();
}

const std::optional<TouchEvent>& TouchDispatcher::getLastEvent() const {
    return _lastEvent;
}

} // namespace snap::drawing
