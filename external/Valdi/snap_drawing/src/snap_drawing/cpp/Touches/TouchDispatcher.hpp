//
//  SkiaTouchDispatcher.hpp
//  valdi-skia-apple
//
//  Created by Simon Corsin on 6/27/20.
//

#pragma once

#include "snap_drawing/cpp/Layers/Layer.hpp"
#include "snap_drawing/cpp/Touches/GestureRecognizer.hpp"
#include "snap_drawing/cpp/Touches/TouchEvent.hpp"

#include "valdi_core/cpp/Interfaces/ILogger.hpp"

#include <optional>
#include <vector>

namespace snap::drawing {

class TouchDispatcher {
public:
    TouchDispatcher(Valdi::ILogger& logger, bool enableLogging);

    void cancelAllGestures();

    bool dispatchEvent(const TouchEvent& event, const Valdi::Ref<Layer>& rootLayer);

    bool isDispatchingEvent() const;

    std::vector<Valdi::Ref<GestureRecognizer>> getGestureCandidatesForEvent(const TouchEvent& event,
                                                                            const Valdi::Ref<Layer>& rootLayer) const;

    bool isEmpty() const;

    const std::optional<TouchEvent>& getLastEvent() const;

private:
    std::vector<Valdi::Ref<GestureRecognizer>> _candidateGestureRecognizers;
    std::vector<Valdi::Ref<GestureRecognizer>> _gestureRecognizersToStart;
    std::optional<TouchEvent> _lastEvent;
    [[maybe_unused]] Valdi::ILogger& _logger;
    bool _dispatchingEvent = false;
    bool _enableLogging = false;

    bool captureCandidates(const TouchEvent& event,
                           const Valdi::Ref<Layer>& layer,
                           std::vector<Valdi::Ref<GestureRecognizer>>& candidateGestureRecognizers) const;

    static bool containsGestureRecognizer(const Valdi::Ref<GestureRecognizer>& gestureRecognizer,
                                          const std::vector<Valdi::Ref<GestureRecognizer>>& gestureRecognizers);

    bool processGestureRecognizers(const Valdi::Ref<Layer>& rootLayer);

    bool isGestureRecognizerDeferred(const Valdi::Ref<GestureRecognizer>& gestureRecognizer) const;

    void cancelGestureRecognizer(const Valdi::Ref<GestureRecognizer>& gestureRecognizer);
    void cancelGestureRecognizersBeforeIndex(size_t index);

    void updateGestureRecognizers(const Valdi::Ref<Layer>& rootLayer);
    void startPendingGestureRecognizers();
    void processActiveGestureRecognizers();

    static bool canRecognizeSimultaneously(const Valdi::Ref<GestureRecognizer>& leftGestureRecognizer,
                                           const Valdi::Ref<GestureRecognizer>& rightGestureRecognizer);

    static std::optional<TouchEvent> adjustEventCoordinatesToLayer(const Valdi::Ref<Layer>& rootLayer,
                                                                   const Valdi::Ref<ILayer>& childLayer,
                                                                   const TouchEvent& event);
};

} // namespace snap::drawing
