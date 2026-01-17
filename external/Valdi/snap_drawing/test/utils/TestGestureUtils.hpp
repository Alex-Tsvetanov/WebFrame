#include <gtest/gtest.h>

#include "snap_drawing/cpp/Layers/Layer.hpp"
#include "snap_drawing/cpp/Layers/LayerRoot.hpp"
#include "valdi_core/cpp/Utils/ConsoleLogger.hpp"

#include "snap_drawing/cpp/Touches/DoubleTapGestureRecognizer.hpp"
#include "snap_drawing/cpp/Touches/DragGestureRecognizer.hpp"
#include "snap_drawing/cpp/Touches/LongPressGestureRecognizer.hpp"
#include "snap_drawing/cpp/Touches/PinchGestureRecognizer.hpp"
#include "snap_drawing/cpp/Touches/RotateGestureRecognizer.hpp"
#include "snap_drawing/cpp/Touches/SingleTapGestureRecognizer.hpp"
#include "snap_drawing/cpp/Touches/TapGestureRecognizer.hpp"
#include "snap_drawing/cpp/Touches/TouchGestureRecognizer.hpp"
#include "snap_drawing/cpp/Touches/WheelGestureRecognizer.hpp"

using namespace Valdi;

namespace snap::drawing {

Ref<LayerRoot> makeRoot();

Ref<Layer> createView(Scalar x, Scalar y, Scalar width, Scalar height);

TouchEvent createTouchEvent(TouchEventType type,
                            Scalar x,
                            Scalar y,
                            Scalar dx = 0,
                            Scalar dy = 0,
                            int pointerCount = 1,
                            Duration after = Duration::fromSeconds(0));

class TestContainer : public SimpleRefCountable {
public:
    TestContainer(Rect size);

    Ref<LayerRoot> root;
    Ref<Layer> view;

    void dispatchEvent(TouchEventType type,
                       Scalar x,
                       Scalar y,
                       Scalar dx = 0,
                       Scalar dy = 0,
                       int pointerCount = 1,
                       Duration after = Duration::fromSeconds(0));
};

struct GestureRecognizerSnapshot : public SimpleRefCountable {
    GestureRecognizerState state = GestureRecognizerStatePossible;
    Point location = Point::makeEmpty();
    Vector direction = Vector::makeEmpty();
    int counter = 0;
    Ref<GestureRecognizer> parent;
};

struct RotateGestureRecognizerSnapshot : public GestureRecognizerSnapshot {
    RotateEvent rotateEvent;
};
struct PinchGestureRecognizerSnapshot : public GestureRecognizerSnapshot {
    PinchEvent pinchEvent;
};

class CustomTapGestureRecognizer : public TapGestureRecognizer {
public:
    using TapGestureRecognizer::TapGestureRecognizer;

    CustomTapGestureRecognizer(size_t taps);

    bool canRecognizeSimultaneously(const GestureRecognizer& other) const override;
    bool requiresFailureOf(const GestureRecognizer& other) const override;
    std::string_view getTypeName() const final;

    bool shouldRecognizeSimulatenously = false;
    bool shouldRequiresFailureOf = false;
};

class CustomTouchGestureRecognizer : public TouchGestureRecognizer {
public:
    using TouchGestureRecognizer::TouchGestureRecognizer;

    CustomTouchGestureRecognizer();

    bool canRecognizeSimultaneously(const GestureRecognizer& other) const override;
    bool requiresFailureOf(const GestureRecognizer& other) const override;

    bool shouldRecognizeSimulatenously = false;
    bool shouldRequiresFailureOf = false;
};

Ref<TestContainer> makeContainer(Scalar x, Scalar y, Scalar width, Scalar height);

Ref<GestureRecognizerSnapshot> addCustomTouchGesture(const Ref<Layer>& view,
                                                     bool shouldRecognizeSimulatenously = true,
                                                     bool shouldRequiresFailureOf = false);

Ref<GestureRecognizerSnapshot> addCustomTapGesture(const Ref<Layer>& view,
                                                   size_t taps,
                                                   bool shouldRecognizeSimulatenously = true,
                                                   bool shouldRequiresFailureOf = false);

Ref<GestureRecognizerSnapshot> addSingleTapGesture(const Ref<Layer>& view);
Ref<GestureRecognizerSnapshot> addDoubleTapGesture(const Ref<Layer>& view);
Ref<GestureRecognizerSnapshot> addLongPressGesture(const Ref<Layer>& view);

Ref<TouchGestureRecognizer> makeTouchGestureRecognizer(const Ref<Layer>& view);

Ref<GestureRecognizerSnapshot> addTouchGesture(const Ref<TouchGestureRecognizer>& gestureRecognizer);
Ref<GestureRecognizerSnapshot> addTouchStartGesture(const Ref<TouchGestureRecognizer>& gestureRecognizer);
Ref<GestureRecognizerSnapshot> addTouchEndGesture(const Ref<TouchGestureRecognizer>& gestureRecognizer);

Ref<GestureRecognizerSnapshot> addDragGesture(const Ref<Layer>& view, bool shouldCancelOtherGesturesOnStart = false);
Ref<RotateGestureRecognizerSnapshot> addRotateGesture(const Ref<Layer>& view);
Ref<PinchGestureRecognizerSnapshot> addPinchGesture(const Ref<Layer>& view);
Ref<GestureRecognizerSnapshot> addWheelGesture(const Ref<Layer>& view);

} // namespace snap::drawing
