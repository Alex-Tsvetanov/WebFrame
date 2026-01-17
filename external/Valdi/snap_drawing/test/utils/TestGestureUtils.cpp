
#include "TestGestureUtils.hpp"

using namespace Valdi;

namespace snap::drawing {

Ref<Resources> createResources() {
    return makeShared<Resources>(nullptr, 1.0f, ConsoleLogger::getLogger());
}

Ref<LayerRoot> makeRoot() {
    return makeShared<LayerRoot>(createResources());
}

Ref<Layer> createView(Scalar x, Scalar y, Scalar width, Scalar height) {
    auto view = makeShared<Layer>(createResources());
    view->setFrame(Rect::makeXYWH(x, y, width, height));
    return view;
}

TouchEvent createTouchEvent(
    TouchEventType type, Scalar x, Scalar y, Scalar dx, Scalar dy, int pointerCount, Duration delay) {
    static auto referencePoint = TimePoint::now();

    // TODO(2835) - add support for these when they're used
    TouchEvent::PointerLocations pointerLocations;
    pointerLocations.push_back(Point::make(x, y));
    return TouchEvent(type,
                      Point::make(x, y),
                      Point::make(x, y),
                      Vector::make(dx, dy),
                      pointerCount,
                      0,
                      std::move(pointerLocations),
                      referencePoint + delay,
                      Duration(),
                      nullptr);
}

TestContainer::TestContainer(Rect size) {
    root = makeRoot();
    view = createView(size.left, size.top, size.width(), size.height());
    root->setContentLayer(view, ContentLayerSizingModeMatchSize);
}

void TestContainer::dispatchEvent(
    TouchEventType type, Scalar x, Scalar y, Scalar dx, Scalar dy, int pointerCount, Duration delay) {
    root->dispatchTouchEvent(createTouchEvent(type, x, y, dx, dy, pointerCount, delay));
}

Ref<TestContainer> makeContainer(Scalar x, Scalar y, Scalar width, Scalar height) {
    return makeShared<TestContainer>(Rect::makeXYWH(x, y, width, height));
}

CustomTapGestureRecognizer::CustomTapGestureRecognizer(size_t taps)
    : TapGestureRecognizer(taps, GesturesConfiguration::getDefault()) {}

bool CustomTapGestureRecognizer::canRecognizeSimultaneously(const GestureRecognizer& other) const {
    return shouldRecognizeSimulatenously;
}

bool CustomTapGestureRecognizer::requiresFailureOf(const GestureRecognizer& other) const {
    return shouldRequiresFailureOf;
}

std::string_view CustomTapGestureRecognizer::getTypeName() const {
    return "custom";
}

CustomTouchGestureRecognizer::CustomTouchGestureRecognizer()
    : TouchGestureRecognizer(GesturesConfiguration::getDefault()) {}

bool CustomTouchGestureRecognizer::canRecognizeSimultaneously(const GestureRecognizer& other) const {
    return shouldRecognizeSimulatenously;
}

bool CustomTouchGestureRecognizer::requiresFailureOf(const GestureRecognizer& other) const {
    return shouldRequiresFailureOf;
}

Ref<GestureRecognizerSnapshot> addCustomTouchGesture(const Ref<Layer>& view,
                                                     bool shouldRecognizeSimulatenously,
                                                     bool shouldRequiresFailureOf) {
    auto snapshot = makeShared<GestureRecognizerSnapshot>();
    auto gestureRecognizer = makeShared<CustomTouchGestureRecognizer>();
    gestureRecognizer->setListener(([=](const auto& gesture, auto state, const auto& point) {
        snapshot->state = state;
        snapshot->location = point;
        snapshot->counter++;
    }));
    snapshot->parent = gestureRecognizer;
    gestureRecognizer->shouldRecognizeSimulatenously = shouldRecognizeSimulatenously;
    gestureRecognizer->shouldRequiresFailureOf = shouldRequiresFailureOf;
    view->addGestureRecognizer(gestureRecognizer);
    return snapshot;
}

Ref<GestureRecognizerSnapshot> addCustomTapGesture(const Ref<Layer>& view,
                                                   size_t taps,
                                                   bool shouldRecognizeSimulatenously,
                                                   bool shouldRequiresFailureOf) {
    auto snapshot = makeShared<GestureRecognizerSnapshot>();
    auto gestureRecognizer = makeShared<CustomTapGestureRecognizer>(taps);
    gestureRecognizer->setListener(([=](const auto& gesture, auto state, const auto& point) {
        snapshot->state = state;
        snapshot->location = point;
        snapshot->counter++;
    }));
    snapshot->parent = gestureRecognizer;
    gestureRecognizer->shouldRecognizeSimulatenously = shouldRecognizeSimulatenously;
    gestureRecognizer->shouldRequiresFailureOf = shouldRequiresFailureOf;
    view->addGestureRecognizer(gestureRecognizer);
    return snapshot;
}

Ref<GestureRecognizerSnapshot> addSingleTapGesture(const Ref<Layer>& view) {
    auto snapshot = makeShared<GestureRecognizerSnapshot>();
    auto gestureRecognizer = makeShared<SingleTapGestureRecognizer>(GesturesConfiguration::getDefault());
    gestureRecognizer->setListener(([=](const auto& gesture, auto state, const auto& point) {
        snapshot->state = state;
        snapshot->location = point;
        snapshot->counter++;
    }));
    snapshot->parent = gestureRecognizer;
    view->addGestureRecognizer(gestureRecognizer);
    return snapshot;
}

Ref<GestureRecognizerSnapshot> addDoubleTapGesture(const Ref<Layer>& view) {
    auto snapshot = makeShared<GestureRecognizerSnapshot>();
    auto gestureRecognizer = makeShared<DoubleTapGestureRecognizer>(GesturesConfiguration::getDefault());
    gestureRecognizer->setListener(([=](const auto& gesture, auto state, const auto& point) {
        snapshot->state = state;
        snapshot->location = point;
        snapshot->counter++;
    }));
    snapshot->parent = gestureRecognizer;
    view->addGestureRecognizer(gestureRecognizer);
    return snapshot;
}

Ref<GestureRecognizerSnapshot> addLongPressGesture(const Ref<Layer>& view) {
    auto snapshot = makeShared<GestureRecognizerSnapshot>();
    auto gestureRecognizer = makeShared<LongPressGestureRecognizer>(GesturesConfiguration::getDefault());
    gestureRecognizer->setListener(([=](const auto& gesture, auto state, const auto& point) {
        snapshot->state = state;
        snapshot->location = point;
        snapshot->counter++;
    }));
    snapshot->parent = gestureRecognizer;
    view->addGestureRecognizer(gestureRecognizer);
    return snapshot;
}

Ref<TouchGestureRecognizer> makeTouchGestureRecognizer(const Ref<Layer>& view) {
    auto gestureRecognizer = makeShared<TouchGestureRecognizer>(GesturesConfiguration::getDefault());
    view->addGestureRecognizer(gestureRecognizer);
    return gestureRecognizer;
}

Ref<GestureRecognizerSnapshot> addTouchGesture(const Ref<TouchGestureRecognizer>& gestureRecognizer) {
    auto snapshot = makeShared<GestureRecognizerSnapshot>();
    gestureRecognizer->setListener(([=](const auto& gesture, auto state, const auto& point) {
        snapshot->state = state;
        snapshot->location = point;
        snapshot->counter++;
    }));
    snapshot->parent = gestureRecognizer;
    return snapshot;
}

Ref<GestureRecognizerSnapshot> addTouchStartGesture(const Ref<TouchGestureRecognizer>& gestureRecognizer) {
    auto snapshot = makeShared<GestureRecognizerSnapshot>();
    gestureRecognizer->setOnStartListener(([=](const auto& gesture, auto state, const auto& point) {
        snapshot->state = state;
        snapshot->location = point;
        snapshot->counter++;
    }));
    snapshot->parent = gestureRecognizer;
    return snapshot;
}

Ref<GestureRecognizerSnapshot> addTouchEndGesture(const Ref<TouchGestureRecognizer>& gestureRecognizer) {
    auto snapshot = makeShared<GestureRecognizerSnapshot>();
    gestureRecognizer->setOnEndListener(([=](const auto& gesture, auto state, const auto& point) {
        snapshot->state = state;
        snapshot->location = point;
        snapshot->counter++;
    }));
    snapshot->parent = gestureRecognizer;
    return snapshot;
}

Ref<GestureRecognizerSnapshot> addDragGesture(const Ref<Layer>& view, bool shouldCancelOtherGesturesOnStart) {
    auto snapshot = makeShared<GestureRecognizerSnapshot>();
    auto gestureRecognizer = makeShared<DragGestureRecognizer>(GesturesConfiguration::getDefault());
    gestureRecognizer->setListener([=](const auto& gesture, auto state, const auto& point) {
        snapshot->state = state;
        snapshot->location = point.location;
        snapshot->counter++;
    });
    snapshot->parent = gestureRecognizer;
    gestureRecognizer->setShouldCancelOtherGesturesOnStart(shouldCancelOtherGesturesOnStart);
    view->addGestureRecognizer(gestureRecognizer);
    return snapshot;
}

Ref<RotateGestureRecognizerSnapshot> addRotateGesture(const Ref<Layer>& view) {
    auto snapshot = makeShared<RotateGestureRecognizerSnapshot>();
    auto gestureRecognizer = makeShared<RotateGestureRecognizer>(GesturesConfiguration::getDefault());
    gestureRecognizer->setListener([=](const auto& gesture, auto state, const auto& point) {
        snapshot->state = state;
        snapshot->location = point.location;
        snapshot->rotateEvent = point;
        snapshot->counter++;
    });
    snapshot->parent = gestureRecognizer;
    view->addGestureRecognizer(gestureRecognizer);
    return snapshot;
}

Ref<PinchGestureRecognizerSnapshot> addPinchGesture(const Ref<Layer>& view) {
    auto snapshot = makeShared<PinchGestureRecognizerSnapshot>();
    auto gestureRecognizer = makeShared<PinchGestureRecognizer>(GesturesConfiguration::getDefault());
    gestureRecognizer->setListener([=](const auto& gesture, auto state, const auto& point) {
        snapshot->state = state;
        snapshot->location = point.location;
        snapshot->pinchEvent = point;
        snapshot->counter++;
    });
    snapshot->parent = gestureRecognizer;
    view->addGestureRecognizer(gestureRecognizer);
    return snapshot;
}

Ref<GestureRecognizerSnapshot> addWheelGesture(const Ref<Layer>& view) {
    auto snapshot = makeShared<GestureRecognizerSnapshot>();
    auto gestureRecognizer =
        makeShared<WheelGestureRecognizer>([=](const auto& gesture, auto state, const auto& scrollEvent) {
            snapshot->state = state;
            snapshot->location = scrollEvent.location;
            snapshot->direction = scrollEvent.direction;
            snapshot->counter++;
        });
    snapshot->parent = gestureRecognizer;
    view->addGestureRecognizer(gestureRecognizer);
    return snapshot;
}

} // namespace snap::drawing
