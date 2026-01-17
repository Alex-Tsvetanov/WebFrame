//
//  SkiaGestureRecognizer.hpp
//  valdi-skia
//
//  Created by Simon Corsin on 6/29/20.
//

#pragma once

#include "valdi_core/cpp/Utils/Function.hpp"
#include "valdi_core/cpp/Utils/Shared.hpp"

#include "snap_drawing/cpp/Touches/TouchEvent.hpp"

#include "snap_drawing/cpp/Layers/Interfaces/ILayer.hpp"
#include "snap_drawing/cpp/Utils/Geometry.hpp"

namespace snap::drawing {

enum GestureRecognizerState {
    GestureRecognizerStatePossible,
    GestureRecognizerStateFailed,
    GestureRecognizerStateBegan,
    GestureRecognizerStateChanged,
    GestureRecognizerStateEnded
};

template<typename Gesture, typename Event>
using GestureRecognizerListener = Valdi::Function<void(const Gesture&, GestureRecognizerState, const Event&)>;

template<typename Gesture, typename Event>
using GestureRecognizerShouldBeginListener = Valdi::Function<bool(const Gesture&, const Event&)>;

class GestureRecognizer : public Valdi::SimpleRefCountable {
public:
    GestureRecognizer();
    ~GestureRecognizer() override;

    void setLayer(ILayer* layer);
    Valdi::Ref<ILayer> getLayer() const;

    const TouchEvent* getLastEvent() const;
    Point getLocation() const;
    Point getLocationInWindow() const;

    GestureRecognizerState getState() const;

    virtual bool requiresFailureOf(const GestureRecognizer& other) const;
    virtual bool canRecognizeSimultaneously(const GestureRecognizer& other) const;

    void update(const TouchEvent& event);
    void process();
    void cancel();

    virtual void onStarted();
    virtual void onReset();

    bool isActive() const;

    bool shouldCancelOtherGesturesOnStart() const;
    void setShouldCancelOtherGesturesOnStart(bool shouldCancelOtherGesturesOnStart);

    bool shouldProcessBeforeOtherGestures() const;
    void setShouldProcessBeforeOtherGestures(bool shouldProcessBeforeOtherGestures);

    std::string toDebugString() const;

protected:
    virtual bool shouldBegin();

    virtual void onUpdate(const TouchEvent& event) = 0;
    virtual void onProcess() = 0;

    void transitionToState(GestureRecognizerState newState);

    virtual std::string_view getTypeName() const = 0;

private:
    Valdi::Weak<ILayer> _layer;
    std::optional<TouchEvent> _lastEvent;
    GestureRecognizerState _state = GestureRecognizerStatePossible;
    bool _wasProcessed = false;
    bool _shouldCancelOtherGesturesOnStart = false;
    bool _shouldProcessBeforeOtherGestures = false;
};

} // namespace snap::drawing
