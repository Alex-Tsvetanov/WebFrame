#include <gtest/gtest.h>

#include "TestGestureUtils.hpp"

using namespace Valdi;

namespace snap::drawing {

TEST(DragGestureRecognizer, canHandleDrag) {
    auto container = makeContainer(0, 0, 100, 100);

    auto childView = createView(25, 25, 25, 25);
    container->view->addChild(childView);

    auto dragSnapshot = addDragGesture(childView);

    container->dispatchEvent(TouchEventTypeDown, 30, 35);
    ASSERT_EQ(GestureRecognizerStatePossible, dragSnapshot->state);

    container->dispatchEvent(TouchEventTypeMoved, 30, 35);
    ASSERT_EQ(GestureRecognizerStatePossible, dragSnapshot->state);

    container->dispatchEvent(TouchEventTypeMoved, 30 + GesturesConfiguration::getDefault().dragTouchSlop, 35);
    ASSERT_EQ(GestureRecognizerStateBegan, dragSnapshot->state);
    ASSERT_EQ(30 + GesturesConfiguration::getDefault().dragTouchSlop - 25, dragSnapshot->location.x);
    ASSERT_EQ(35 - 25, dragSnapshot->location.y);

    container->dispatchEvent(TouchEventTypeMoved, 30 + GesturesConfiguration::getDefault().dragTouchSlop, 35);
    ASSERT_EQ(GestureRecognizerStateChanged, dragSnapshot->state);

    container->dispatchEvent(TouchEventTypeUp, 30, 35);
    ASSERT_EQ(GestureRecognizerStateEnded, dragSnapshot->state);

    ASSERT_TRUE(container->root->getTouchDispatcher().isEmpty());
}

TEST(DragGestureRecognizer, canHandleDragWithSingleTap) {
    auto container = makeContainer(0, 0, 100, 100);

    auto childView = createView(25, 25, 25, 25);
    container->view->addChild(childView);

    auto dragSnapshot = addDragGesture(childView);
    auto singleTapSnapshot = addSingleTapGesture(childView);

    container->dispatchEvent(TouchEventTypeDown, 30, 35);
    ASSERT_EQ(GestureRecognizerStatePossible, dragSnapshot->state);
    ASSERT_EQ(GestureRecognizerStatePossible, singleTapSnapshot->state);

    container->dispatchEvent(TouchEventTypeMoved, 30, 35);
    ASSERT_EQ(GestureRecognizerStatePossible, dragSnapshot->state);
    ASSERT_EQ(GestureRecognizerStatePossible, singleTapSnapshot->state);

    container->dispatchEvent(TouchEventTypeMoved, 30 + GesturesConfiguration::getDefault().dragTouchSlop, 35);
    ASSERT_EQ(GestureRecognizerStateBegan, dragSnapshot->state);
    ASSERT_EQ(GestureRecognizerStatePossible, singleTapSnapshot->state);

    container->dispatchEvent(TouchEventTypeMoved, 30 + GesturesConfiguration::getDefault().dragTouchSlop, 35);
    ASSERT_EQ(GestureRecognizerStateChanged, dragSnapshot->state);
    ASSERT_EQ(GestureRecognizerStatePossible, singleTapSnapshot->state);

    container->dispatchEvent(TouchEventTypeUp, 30, 35);
    ASSERT_EQ(GestureRecognizerStateEnded, dragSnapshot->state);
    ASSERT_EQ(GestureRecognizerStatePossible, singleTapSnapshot->state);

    ASSERT_TRUE(container->root->getTouchDispatcher().isEmpty());
}

TEST(DragGestureRecognizer, canHandleDragWithLongPress) {
    auto container = makeContainer(0, 0, 100, 100);

    auto childView = createView(25, 25, 25, 25);
    container->view->addChild(childView);

    auto dragSnapshot = addDragGesture(childView);
    auto longPressSnapshot = addLongPressGesture(childView);

    container->dispatchEvent(TouchEventTypeDown, 30, 35);
    ASSERT_EQ(GestureRecognizerStatePossible, dragSnapshot->state);
    ASSERT_EQ(GestureRecognizerStatePossible, longPressSnapshot->state);

    container->dispatchEvent(TouchEventTypeMoved, 30 + GesturesConfiguration::getDefault().dragTouchSlop, 35);
    ASSERT_EQ(GestureRecognizerStateBegan, dragSnapshot->state);
    ASSERT_EQ(GestureRecognizerStatePossible, longPressSnapshot->state);

    container->dispatchEvent(
        TouchEventTypeMoved, 30, 35, 0, 0, 1, GesturesConfiguration::getDefault().longPressTimeout);
    ASSERT_EQ(GestureRecognizerStateChanged, dragSnapshot->state);
    ASSERT_EQ(GestureRecognizerStatePossible, longPressSnapshot->state);

    container->dispatchEvent(TouchEventTypeUp, 30, 35);
    ASSERT_EQ(GestureRecognizerStateEnded, dragSnapshot->state);
    ASSERT_EQ(GestureRecognizerStatePossible, longPressSnapshot->state);

    ASSERT_TRUE(container->root->getTouchDispatcher().isEmpty());
}

TEST(DragGestureRecognizer, canHandleDragWithoutLongPress) {
    auto container = makeContainer(0, 0, 100, 100);

    auto childView = createView(25, 25, 25, 25);
    container->view->addChild(childView);

    auto dragSnapshot = addDragGesture(childView);

    container->dispatchEvent(TouchEventTypeDown, 30, 35);
    ASSERT_EQ(GestureRecognizerStatePossible, dragSnapshot->state);

    container->dispatchEvent(
        TouchEventTypeMoved, 30, 35, 0, 0, 1, GesturesConfiguration::getDefault().longPressTimeout);
    ASSERT_EQ(GestureRecognizerStatePossible, dragSnapshot->state);

    container->dispatchEvent(TouchEventTypeMoved, 30 + GesturesConfiguration::getDefault().dragTouchSlop, 35);
    ASSERT_EQ(GestureRecognizerStateBegan, dragSnapshot->state);

    container->dispatchEvent(TouchEventTypeUp, 30, 35);

    ASSERT_TRUE(container->root->getTouchDispatcher().isEmpty());
}

TEST(DragGestureRecognizer, canHandleDragDelayed) {
    auto container = makeContainer(0, 0, 100, 100);

    auto childView = createView(25, 25, 25, 25);
    container->view->addChild(childView);

    auto dragSnapshot = addDragGesture(childView);

    auto hold = GesturesConfiguration::getDefault().longPressTimeout;

    container->dispatchEvent(TouchEventTypeDown, 30, 35);
    ASSERT_EQ(GestureRecognizerStatePossible, dragSnapshot->state);

    container->dispatchEvent(TouchEventTypeMoved, 30, 35, 0, 0, 1, hold);
    ASSERT_EQ(GestureRecognizerStatePossible, dragSnapshot->state);

    container->dispatchEvent(
        TouchEventTypeMoved, 30 + GesturesConfiguration::getDefault().dragTouchSlop, 35, 0, 0, 1, hold);
    ASSERT_EQ(GestureRecognizerStateBegan, dragSnapshot->state);

    container->dispatchEvent(
        TouchEventTypeMoved, 30 + GesturesConfiguration::getDefault().dragTouchSlop, 35, 0, 0, 1, hold);
    ASSERT_EQ(GestureRecognizerStateChanged, dragSnapshot->state);

    container->dispatchEvent(TouchEventTypeUp, 30, 35, 0, 0, 1, hold);
    ASSERT_EQ(GestureRecognizerStateEnded, dragSnapshot->state);

    ASSERT_TRUE(container->root->getTouchDispatcher().isEmpty());
}

TEST(DragGestureRecognizer, canHandleDragTwoPointers) {
    auto container = makeContainer(0, 0, 100, 100);

    auto childView = createView(25, 25, 25, 25);
    container->view->addChild(childView);

    auto dragSnapshot = addDragGesture(childView);

    auto hold = GesturesConfiguration::getDefault().longPressTimeout;

    container->dispatchEvent(TouchEventTypeDown, 30, 35, 1, 1, 2);
    ASSERT_EQ(GestureRecognizerStateBegan, dragSnapshot->state);

    container->dispatchEvent(TouchEventTypeMoved, 30, 35, 1, 1, 2);
    ASSERT_EQ(GestureRecognizerStateChanged, dragSnapshot->state);

    container->dispatchEvent(TouchEventTypeMoved, 30 + GesturesConfiguration::getDefault().dragTouchSlop, 35, 1, 1, 2);
    ASSERT_EQ(GestureRecognizerStateChanged, dragSnapshot->state);

    container->dispatchEvent(TouchEventTypeUp, 30, 35, 0, 0, 1, hold);
    ASSERT_EQ(GestureRecognizerStateEnded, dragSnapshot->state);

    ASSERT_TRUE(container->root->getTouchDispatcher().isEmpty());
}

} // namespace snap::drawing
