#include <gtest/gtest.h>

#include "TestGestureUtils.hpp"

using namespace Valdi;

namespace snap::drawing {

TEST(PinchGestureRecognizer, canHandlePinchTwoPointers) {
    auto container = makeContainer(0, 0, 100, 100);

    auto childView = createView(25, 25, 25, 25);
    container->view->addChild(childView);

    auto pinchSnapshot = addPinchGesture(childView);

    container->dispatchEvent(TouchEventTypeDown, 30, 35, 1, 1, 2);
    ASSERT_EQ(GestureRecognizerStateBegan, pinchSnapshot->state);
    ASSERT_FLOAT_EQ(1, pinchSnapshot->pinchEvent.scale);

    container->dispatchEvent(TouchEventTypeMoved, 35, 40, 1, 2, 2);
    ASSERT_EQ(GestureRecognizerStateChanged, pinchSnapshot->state);
    ASSERT_FLOAT_EQ(1.5811388, pinchSnapshot->pinchEvent.scale);

    container->dispatchEvent(TouchEventTypePointerUp, 35, 40, 1, 2, 2);
    ASSERT_EQ(GestureRecognizerStateChanged, pinchSnapshot->state);
    ASSERT_FLOAT_EQ(1.5811388, pinchSnapshot->pinchEvent.scale);

    container->dispatchEvent(TouchEventTypeUp, 35, 40, 1, 2, 1);
    ASSERT_EQ(GestureRecognizerStateEnded, pinchSnapshot->state);
    ASSERT_FLOAT_EQ(1.5811388, pinchSnapshot->pinchEvent.scale);

    ASSERT_TRUE(container->root->getTouchDispatcher().isEmpty());
}

TEST(PinchGestureRecognizer, canHandlePinchTwoPointersInterrupted) {
    auto container = makeContainer(0, 0, 100, 100);

    auto childView = createView(25, 25, 25, 25);
    container->view->addChild(childView);

    auto pinchSnapshot = addPinchGesture(childView);

    container->dispatchEvent(TouchEventTypeDown, 30, 35, 1, 1, 2);
    ASSERT_EQ(GestureRecognizerStateBegan, pinchSnapshot->state);
    ASSERT_FLOAT_EQ(1, pinchSnapshot->pinchEvent.scale);

    container->dispatchEvent(TouchEventTypeMoved, 40, 35, 1, 2, 2);
    ASSERT_EQ(GestureRecognizerStateChanged, pinchSnapshot->state);
    ASSERT_FLOAT_EQ(1.5811388, pinchSnapshot->pinchEvent.scale);

    container->dispatchEvent(TouchEventTypePointerUp, 40, 35, 0, 0, 2);
    ASSERT_EQ(GestureRecognizerStateChanged, pinchSnapshot->state);
    ASSERT_FLOAT_EQ(1.5811388, pinchSnapshot->pinchEvent.scale);

    container->dispatchEvent(TouchEventTypeMoved, 40, 35, 0, 0, 1);
    ASSERT_EQ(GestureRecognizerStateChanged, pinchSnapshot->state);
    ASSERT_FLOAT_EQ(1.5811388, pinchSnapshot->pinchEvent.scale);

    container->dispatchEvent(TouchEventTypeUp, 40, 35, 0, 0, 1);
    ASSERT_EQ(GestureRecognizerStateEnded, pinchSnapshot->state);
    ASSERT_FLOAT_EQ(1.5811388, pinchSnapshot->pinchEvent.scale);

    ASSERT_TRUE(container->root->getTouchDispatcher().isEmpty());
}

TEST(PinchGestureRecognizer, canHandlePinchOneThenTwoPointers) {
    auto container = makeContainer(0, 0, 100, 100);

    auto childView = createView(25, 25, 25, 25);
    container->view->addChild(childView);

    auto pinchSnapshot = addPinchGesture(childView);

    auto holdDuration = Duration::fromSeconds(GesturesConfiguration::getDefault().dragTouchSlop);

    container->dispatchEvent(TouchEventTypeDown, 30, 35);
    ASSERT_EQ(GestureRecognizerStatePossible, pinchSnapshot->state);

    container->dispatchEvent(TouchEventTypeMoved, 30, 35);
    ASSERT_EQ(GestureRecognizerStatePossible, pinchSnapshot->state);

    container->dispatchEvent(TouchEventTypeMoved, 30, 35, 0, 0, 1, holdDuration);
    ASSERT_EQ(GestureRecognizerStatePossible, pinchSnapshot->state);

    container->dispatchEvent(TouchEventTypeMoved, 30, 35, 1, 1, 2, holdDuration);
    ASSERT_EQ(GestureRecognizerStateBegan, pinchSnapshot->state);

    container->dispatchEvent(TouchEventTypeMoved, 30, 35, 1, 2, 2, holdDuration);
    ASSERT_EQ(GestureRecognizerStateChanged, pinchSnapshot->state);

    container->dispatchEvent(TouchEventTypeUp, 30, 35);
    ASSERT_EQ(GestureRecognizerStateEnded, pinchSnapshot->state);

    ASSERT_TRUE(container->root->getTouchDispatcher().isEmpty());
}

TEST(PinchGestureRecognizer, canHandlePinchResumption) {
    auto container = makeContainer(0, 0, 100, 100);

    auto childView = createView(25, 25, 25, 25);
    container->view->addChild(childView);

    auto pinchSnapshot = addPinchGesture(childView);

    container->dispatchEvent(TouchEventTypeDown, 30, 35, 1, 1, 2);
    ASSERT_EQ(GestureRecognizerStateBegan, pinchSnapshot->state);
    ASSERT_FLOAT_EQ(1, pinchSnapshot->pinchEvent.scale);

    container->dispatchEvent(TouchEventTypeMoved, 40, 35, 1, 2, 2);
    ASSERT_EQ(GestureRecognizerStateChanged, pinchSnapshot->state);
    ASSERT_FLOAT_EQ(1.5811388, pinchSnapshot->pinchEvent.scale);

    container->dispatchEvent(TouchEventTypePointerUp, 40, 35, 0, 0, 2);
    ASSERT_EQ(GestureRecognizerStateChanged, pinchSnapshot->state);
    ASSERT_FLOAT_EQ(1.5811388, pinchSnapshot->pinchEvent.scale);

    container->dispatchEvent(TouchEventTypeMoved, 60, 35, 0, 0, 1);
    ASSERT_EQ(GestureRecognizerStateChanged, pinchSnapshot->state);
    ASSERT_FLOAT_EQ(1.5811388, pinchSnapshot->pinchEvent.scale);

    container->dispatchEvent(TouchEventTypePointerDown, 60, 35, 0, 0, 2);
    ASSERT_EQ(GestureRecognizerStateChanged, pinchSnapshot->state);
    ASSERT_FLOAT_EQ(1.5811388, pinchSnapshot->pinchEvent.scale);

    container->dispatchEvent(TouchEventTypeMoved, 40, 35, 1, 2, 2);
    ASSERT_EQ(GestureRecognizerStateChanged, pinchSnapshot->state);
    ASSERT_FLOAT_EQ(2.5, pinchSnapshot->pinchEvent.scale);

    container->dispatchEvent(TouchEventTypePointerUp, 40, 35, 1, 2, 2);
    ASSERT_EQ(GestureRecognizerStateChanged, pinchSnapshot->state);
    ASSERT_FLOAT_EQ(2.5, pinchSnapshot->pinchEvent.scale);

    container->dispatchEvent(TouchEventTypeUp, 40, 35, 0, 0, 1);
    ASSERT_EQ(GestureRecognizerStateEnded, pinchSnapshot->state);
    ASSERT_FLOAT_EQ(2.5, pinchSnapshot->pinchEvent.scale);

    ASSERT_TRUE(container->root->getTouchDispatcher().isEmpty());
}

} // namespace snap::drawing
