#include <gtest/gtest.h>

#include "TestGestureUtils.hpp"

using namespace Valdi;

namespace snap::drawing {

TEST(TouchGestureRecognizer, canHandleTouch) {
    auto container = makeContainer(0, 0, 100, 100);

    auto childView = createView(25, 25, 25, 25);
    container->view->addChild(childView);

    auto gestureRecognizer = makeTouchGestureRecognizer(childView);

    auto touchSnapshot = addTouchGesture(gestureRecognizer);

    container->dispatchEvent(TouchEventTypeDown, 30, 35);
    ASSERT_EQ(GestureRecognizerStateBegan, touchSnapshot->state);

    container->dispatchEvent(TouchEventTypeMoved, 30, 35);
    ASSERT_EQ(GestureRecognizerStateChanged, touchSnapshot->state);

    container->dispatchEvent(TouchEventTypeUp, 30, 35);
    ASSERT_EQ(GestureRecognizerStateEnded, touchSnapshot->state);

    ASSERT_TRUE(container->root->getTouchDispatcher().isEmpty());
}

TEST(TouchGestureRecognizer, canHandleTouchWithSingleTap) {
    auto container = makeContainer(0, 0, 100, 100);

    auto childView = createView(25, 25, 25, 25);
    container->view->addChild(childView);

    auto gestureRecognizer = makeTouchGestureRecognizer(childView);

    auto touchSnapshot = addTouchGesture(gestureRecognizer);
    auto singleTapSnapshot = addSingleTapGesture(childView);

    container->dispatchEvent(TouchEventTypeDown, 30, 35);
    ASSERT_EQ(GestureRecognizerStateBegan, touchSnapshot->state);
    ASSERT_EQ(GestureRecognizerStatePossible, singleTapSnapshot->state);

    container->dispatchEvent(TouchEventTypeMoved, 30, 35);
    ASSERT_EQ(GestureRecognizerStateChanged, touchSnapshot->state);
    ASSERT_EQ(GestureRecognizerStatePossible, singleTapSnapshot->state);

    container->dispatchEvent(TouchEventTypeUp, 30, 35);
    ASSERT_EQ(GestureRecognizerStateEnded, touchSnapshot->state);
    ASSERT_EQ(GestureRecognizerStateEnded, singleTapSnapshot->state);

    ASSERT_TRUE(container->root->getTouchDispatcher().isEmpty());
}

TEST(TouchGestureRecognizer, canHandleTouchWithDoubleTap) {
    auto container = makeContainer(0, 0, 100, 100);

    auto childView = createView(25, 25, 25, 25);
    container->view->addChild(childView);

    auto gestureRecognizer = makeTouchGestureRecognizer(childView);

    auto touchSnapshot = addTouchGesture(gestureRecognizer);
    auto doubleTapSnapshot = addDoubleTapGesture(childView);

    container->dispatchEvent(TouchEventTypeDown, 30, 35);
    ASSERT_EQ(GestureRecognizerStateBegan, touchSnapshot->state);
    ASSERT_EQ(GestureRecognizerStatePossible, doubleTapSnapshot->state);

    container->dispatchEvent(TouchEventTypeMoved, 30, 35);
    ASSERT_EQ(GestureRecognizerStateChanged, touchSnapshot->state);
    ASSERT_EQ(GestureRecognizerStatePossible, doubleTapSnapshot->state);

    container->dispatchEvent(TouchEventTypeUp, 30, 35);
    ASSERT_EQ(GestureRecognizerStateEnded, touchSnapshot->state);
    ASSERT_EQ(GestureRecognizerStatePossible, doubleTapSnapshot->state);

    ASSERT_FALSE(container->root->getTouchDispatcher().isEmpty());

    container->dispatchEvent(TouchEventTypeDown, 30, 35);
    ASSERT_EQ(GestureRecognizerStateBegan, touchSnapshot->state);
    ASSERT_EQ(GestureRecognizerStatePossible, doubleTapSnapshot->state);

    container->dispatchEvent(TouchEventTypeMoved, 30, 35);
    ASSERT_EQ(GestureRecognizerStateChanged, touchSnapshot->state);
    ASSERT_EQ(GestureRecognizerStatePossible, doubleTapSnapshot->state);

    container->dispatchEvent(TouchEventTypeUp, 30, 35);
    ASSERT_EQ(GestureRecognizerStateEnded, touchSnapshot->state);
    ASSERT_EQ(GestureRecognizerStateEnded, doubleTapSnapshot->state);

    ASSERT_EQ(6, touchSnapshot->counter);
    ASSERT_EQ(1, doubleTapSnapshot->counter);

    ASSERT_TRUE(container->root->getTouchDispatcher().isEmpty());
}

TEST(TouchGestureRecognizer, canHandleTouchWithLongPress) {
    auto container = makeContainer(0, 0, 100, 100);

    auto childView = createView(25, 25, 25, 25);
    container->view->addChild(childView);

    auto gestureRecognizer = makeTouchGestureRecognizer(childView);

    auto touchSnapshot = addTouchGesture(gestureRecognizer);
    auto longPressSnapshot = addLongPressGesture(childView);

    container->dispatchEvent(TouchEventTypeDown, 30, 35);
    ASSERT_EQ(GestureRecognizerStateBegan, touchSnapshot->state);
    ASSERT_EQ(GestureRecognizerStatePossible, longPressSnapshot->state);

    container->dispatchEvent(
        TouchEventTypeMoved, 30, 35, 0, 0, 1, GesturesConfiguration::getDefault().longPressTimeout);
    ASSERT_EQ(GestureRecognizerStateChanged, touchSnapshot->state);
    ASSERT_EQ(GestureRecognizerStateBegan, longPressSnapshot->state);

    container->dispatchEvent(TouchEventTypeUp, 30, 35);
    ASSERT_EQ(GestureRecognizerStateEnded, touchSnapshot->state);
    ASSERT_EQ(GestureRecognizerStateEnded, longPressSnapshot->state);

    ASSERT_TRUE(container->root->getTouchDispatcher().isEmpty());
}

TEST(TouchGestureRecognizer, canHandleTouchStart) {
    auto container = makeContainer(0, 0, 100, 100);

    auto childView = createView(25, 25, 25, 25);
    container->view->addChild(childView);

    auto gestureRecognizer = makeTouchGestureRecognizer(childView);

    auto touchStartSnapshot = addTouchStartGesture(gestureRecognizer);

    container->dispatchEvent(TouchEventTypeDown, 30, 35);
    ASSERT_EQ(GestureRecognizerStateBegan, touchStartSnapshot->state);

    container->dispatchEvent(TouchEventTypeMoved, 30, 35);
    ASSERT_EQ(GestureRecognizerStateBegan, touchStartSnapshot->state);

    container->dispatchEvent(TouchEventTypeUp, 30, 35);
    ASSERT_EQ(GestureRecognizerStateBegan, touchStartSnapshot->state);

    ASSERT_TRUE(container->root->getTouchDispatcher().isEmpty());
}

TEST(TouchGestureRecognizer, canHandleTouchEnd) {
    auto container = makeContainer(0, 0, 100, 100);

    auto childView = createView(25, 25, 25, 25);
    container->view->addChild(childView);

    auto gestureRecognizer = makeTouchGestureRecognizer(childView);

    auto touchEndSnapshot = addTouchEndGesture(gestureRecognizer);

    container->dispatchEvent(TouchEventTypeDown, 30, 35);
    ASSERT_EQ(GestureRecognizerStatePossible, touchEndSnapshot->state);

    container->dispatchEvent(TouchEventTypeMoved, 30, 35);
    ASSERT_EQ(GestureRecognizerStatePossible, touchEndSnapshot->state);

    container->dispatchEvent(TouchEventTypeUp, 30, 35);
    ASSERT_EQ(GestureRecognizerStateEnded, touchEndSnapshot->state);

    ASSERT_TRUE(container->root->getTouchDispatcher().isEmpty());
}

TEST(TouchGestureRecognizer, canHandleTouchCombined) {
    auto container = makeContainer(0, 0, 100, 100);

    auto childView = createView(25, 25, 25, 25);
    container->view->addChild(childView);

    auto gestureRecognizer = makeTouchGestureRecognizer(childView);

    auto touchSnapshot = addTouchGesture(gestureRecognizer);
    auto touchStartSnapshot = addTouchStartGesture(gestureRecognizer);
    auto touchEndSnapshot = addTouchEndGesture(gestureRecognizer);

    container->dispatchEvent(TouchEventTypeDown, 30, 35);
    ASSERT_EQ(GestureRecognizerStateBegan, touchSnapshot->state);
    ASSERT_EQ(GestureRecognizerStateBegan, touchStartSnapshot->state);
    ASSERT_EQ(GestureRecognizerStatePossible, touchEndSnapshot->state);

    container->dispatchEvent(TouchEventTypeMoved, 30, 35);
    ASSERT_EQ(GestureRecognizerStateChanged, touchSnapshot->state);
    ASSERT_EQ(GestureRecognizerStateBegan, touchStartSnapshot->state);
    ASSERT_EQ(GestureRecognizerStatePossible, touchEndSnapshot->state);

    container->dispatchEvent(TouchEventTypeUp, 30, 35);
    ASSERT_EQ(GestureRecognizerStateEnded, touchSnapshot->state);
    ASSERT_EQ(GestureRecognizerStateBegan, touchStartSnapshot->state);
    ASSERT_EQ(GestureRecognizerStateEnded, touchEndSnapshot->state);

    ASSERT_EQ(3, touchSnapshot->counter);
    ASSERT_EQ(1, touchStartSnapshot->counter);
    ASSERT_EQ(1, touchEndSnapshot->counter);

    ASSERT_TRUE(container->root->getTouchDispatcher().isEmpty());
}

TEST(TouchGestureRecognizer, canHandleOverlappingTapGestures) {
    auto container = makeContainer(0, 0, 100, 100);

    auto childView1 = createView(25, 25, 25, 25);
    container->view->addChild(childView1);
    auto singleTapSnapshot1 = addSingleTapGesture(childView1);

    auto childView2 = createView(0, 0, 25, 25);
    childView1->addChild(childView2);
    auto singleTapSnapshot2 = addSingleTapGesture(childView2);

    container->dispatchEvent(TouchEventTypeDown, 30, 35);
    ASSERT_EQ(GestureRecognizerStatePossible, singleTapSnapshot1->state);
    ASSERT_EQ(GestureRecognizerStatePossible, singleTapSnapshot2->state);

    container->dispatchEvent(TouchEventTypeUp, 30, 35);
    ASSERT_EQ(GestureRecognizerStatePossible, singleTapSnapshot1->state);
    ASSERT_EQ(GestureRecognizerStateEnded, singleTapSnapshot2->state);

    ASSERT_TRUE(container->root->getTouchDispatcher().isEmpty());
}

} // namespace snap::drawing
