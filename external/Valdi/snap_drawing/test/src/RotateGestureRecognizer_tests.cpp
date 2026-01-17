#include <gtest/gtest.h>

#include "TestGestureUtils.hpp"

using namespace Valdi;

namespace snap::drawing {

TEST(RotateGestureRecognizer, canHandleRotateTwoPointers) {
    auto container = makeContainer(0, 0, 100, 100);

    auto childView = createView(25, 25, 25, 25);
    container->view->addChild(childView);

    auto rotateSnapshot = addRotateGesture(childView);

    container->dispatchEvent(TouchEventTypeDown, 30, 35, 1, 1, 2);
    ASSERT_EQ(GestureRecognizerStateBegan, rotateSnapshot->state);
    ASSERT_FLOAT_EQ(0, rotateSnapshot->rotateEvent.rotation);

    container->dispatchEvent(TouchEventTypeMoved, 35, 40, 1, 2, 2);
    ASSERT_EQ(GestureRecognizerStateChanged, rotateSnapshot->state);
    ASSERT_FLOAT_EQ(0.32175058, rotateSnapshot->rotateEvent.rotation);

    container->dispatchEvent(TouchEventTypePointerUp, 35, 40, 1, 2, 2);
    ASSERT_EQ(GestureRecognizerStateChanged, rotateSnapshot->state);
    ASSERT_FLOAT_EQ(0.32175058, rotateSnapshot->rotateEvent.rotation);

    container->dispatchEvent(TouchEventTypeUp, 35, 40, 1, 2, 1);
    ASSERT_EQ(GestureRecognizerStateEnded, rotateSnapshot->state);
    ASSERT_FLOAT_EQ(0.32175058, rotateSnapshot->rotateEvent.rotation);

    ASSERT_TRUE(container->root->getTouchDispatcher().isEmpty());
}

TEST(RotateGestureRecognizer, canHandleRotateTwoPointersInterrupted) {
    auto container = makeContainer(0, 0, 100, 100);

    auto childView = createView(25, 25, 25, 25);
    container->view->addChild(childView);

    auto rotateSnapshot = addRotateGesture(childView);

    container->dispatchEvent(TouchEventTypeDown, 30, 35, 1, 1, 2);
    ASSERT_EQ(GestureRecognizerStateBegan, rotateSnapshot->state);
    ASSERT_FLOAT_EQ(0, rotateSnapshot->rotateEvent.rotation);

    container->dispatchEvent(TouchEventTypeMoved, 35, 40, 1, 2, 2);
    ASSERT_EQ(GestureRecognizerStateChanged, rotateSnapshot->state);
    ASSERT_FLOAT_EQ(0.32175058, rotateSnapshot->rotateEvent.rotation);

    container->dispatchEvent(TouchEventTypePointerUp, 35, 40, 0, 0, 2);
    ASSERT_EQ(GestureRecognizerStateChanged, rotateSnapshot->state);
    ASSERT_FLOAT_EQ(0.32175058, rotateSnapshot->rotateEvent.rotation);

    container->dispatchEvent(TouchEventTypeMoved, 35, 40, 0, 0, 1);
    ASSERT_EQ(GestureRecognizerStateChanged, rotateSnapshot->state);
    ASSERT_FLOAT_EQ(0.32175058, rotateSnapshot->rotateEvent.rotation);

    container->dispatchEvent(TouchEventTypeUp, 35, 40, 0, 0, 1);
    ASSERT_EQ(GestureRecognizerStateEnded, rotateSnapshot->state);
    ASSERT_FLOAT_EQ(0.32175058, rotateSnapshot->rotateEvent.rotation);

    ASSERT_TRUE(container->root->getTouchDispatcher().isEmpty());
}

TEST(RotateGestureRecognizer, canHandleRotateOneThenTwoPointers) {
    auto container = makeContainer(0, 0, 100, 100);

    auto childView = createView(25, 25, 25, 25);
    container->view->addChild(childView);

    auto rotateSnapshot = addRotateGesture(childView);

    auto holdDuration = Duration::fromSeconds(GesturesConfiguration::getDefault().dragTouchSlop);

    container->dispatchEvent(TouchEventTypeDown, 30, 35);
    ASSERT_EQ(GestureRecognizerStatePossible, rotateSnapshot->state);

    container->dispatchEvent(TouchEventTypeMoved, 30, 35);
    ASSERT_EQ(GestureRecognizerStatePossible, rotateSnapshot->state);

    container->dispatchEvent(TouchEventTypeMoved, 30, 35, 0, 0, 1, holdDuration);
    ASSERT_EQ(GestureRecognizerStatePossible, rotateSnapshot->state);

    container->dispatchEvent(TouchEventTypeMoved, 30, 35, 1, 1, 2, holdDuration);
    ASSERT_EQ(GestureRecognizerStateBegan, rotateSnapshot->state);

    container->dispatchEvent(TouchEventTypeMoved, 30, 35, 1, 2, 2, holdDuration);
    ASSERT_EQ(GestureRecognizerStateChanged, rotateSnapshot->state);

    container->dispatchEvent(TouchEventTypeUp, 30, 35);
    ASSERT_EQ(GestureRecognizerStateEnded, rotateSnapshot->state);

    ASSERT_TRUE(container->root->getTouchDispatcher().isEmpty());
}

TEST(RotateGestureRecognizer, canHandleRotateResumption) {
    auto container = makeContainer(0, 0, 100, 100);

    auto childView = createView(25, 25, 25, 25);
    container->view->addChild(childView);

    auto rotateSnapshot = addRotateGesture(childView);

    container->dispatchEvent(TouchEventTypeDown, 30, 35, 1, 1, 2);
    ASSERT_EQ(GestureRecognizerStateBegan, rotateSnapshot->state);
    ASSERT_FLOAT_EQ(0, rotateSnapshot->rotateEvent.rotation);

    container->dispatchEvent(TouchEventTypeMoved, 35, 40, 1, 2, 2);
    ASSERT_EQ(GestureRecognizerStateChanged, rotateSnapshot->state);
    ASSERT_FLOAT_EQ(0.32175058, rotateSnapshot->rotateEvent.rotation);

    container->dispatchEvent(TouchEventTypePointerUp, 35, 40, 0, 0, 2);
    ASSERT_EQ(GestureRecognizerStateChanged, rotateSnapshot->state);
    ASSERT_FLOAT_EQ(0.32175058, rotateSnapshot->rotateEvent.rotation);

    container->dispatchEvent(TouchEventTypeMoved, 55, 60, 0, 0, 1);
    ASSERT_EQ(GestureRecognizerStateChanged, rotateSnapshot->state);
    ASSERT_FLOAT_EQ(0.32175058, rotateSnapshot->rotateEvent.rotation);

    container->dispatchEvent(TouchEventTypePointerDown, 55, 60, 0, 0, 2);
    ASSERT_EQ(GestureRecognizerStateChanged, rotateSnapshot->state);
    ASSERT_FLOAT_EQ(0.32175058, rotateSnapshot->rotateEvent.rotation);

    container->dispatchEvent(TouchEventTypeMoved, 60, 65, 0, 0, 2);
    ASSERT_EQ(GestureRecognizerStateChanged, rotateSnapshot->state);
    ASSERT_FLOAT_EQ(1.1071488, rotateSnapshot->rotateEvent.rotation);

    container->dispatchEvent(TouchEventTypePointerUp, 60, 65, 0, 0, 2);
    ASSERT_EQ(GestureRecognizerStateChanged, rotateSnapshot->state);
    ASSERT_FLOAT_EQ(1.1071488, rotateSnapshot->rotateEvent.rotation);

    container->dispatchEvent(TouchEventTypeUp, 60, 65, 0, 0, 1);
    ASSERT_EQ(GestureRecognizerStateEnded, rotateSnapshot->state);
    ASSERT_FLOAT_EQ(1.1071488, rotateSnapshot->rotateEvent.rotation);

    ASSERT_TRUE(container->root->getTouchDispatcher().isEmpty());
}

} // namespace snap::drawing
