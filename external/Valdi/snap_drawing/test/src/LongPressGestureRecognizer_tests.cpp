#include <gtest/gtest.h>

#include "TestGestureUtils.hpp"

using namespace Valdi;

namespace snap::drawing {

TEST(LongPressGestureRecognizer, canHandleLongPress) {
    auto container = makeContainer(0, 0, 100, 100);

    auto childView = createView(25, 25, 25, 25);
    container->view->addChild(childView);

    auto longPressSnapshot = addLongPressGesture(childView);

    container->dispatchEvent(TouchEventTypeDown, 30, 35);

    container->dispatchEvent(
        TouchEventTypeMoved, 30, 35, 0, 0, 1, GesturesConfiguration::getDefault().longPressTimeout);
    ASSERT_EQ(GestureRecognizerStateBegan, longPressSnapshot->state);

    container->dispatchEvent(TouchEventTypeUp, 30, 35);
    ASSERT_EQ(GestureRecognizerStateEnded, longPressSnapshot->state);

    ASSERT_TRUE(container->root->getTouchDispatcher().isEmpty());
}

TEST(LongPressGestureRecognizer, canHandleLongPressWithDoubleTap) {
    auto container = makeContainer(0, 0, 100, 100);

    auto childView = createView(25, 25, 25, 25);
    container->view->addChild(childView);

    auto doubleTapSnapshot = addDoubleTapGesture(childView);
    auto longPressSnapshot = addLongPressGesture(childView);

    container->dispatchEvent(TouchEventTypeDown, 30, 35);
    container->dispatchEvent(
        TouchEventTypeMoved, 30, 35, 0, 0, 1, GesturesConfiguration::getDefault().longPressTimeout);
    ASSERT_EQ(GestureRecognizerStatePossible, doubleTapSnapshot->state);
    ASSERT_EQ(GestureRecognizerStateBegan, longPressSnapshot->state);

    container->dispatchEvent(TouchEventTypeUp, 30, 35);
    ASSERT_EQ(GestureRecognizerStatePossible, doubleTapSnapshot->state);
    ASSERT_EQ(GestureRecognizerStateEnded, longPressSnapshot->state);

    ASSERT_TRUE(container->root->getTouchDispatcher().isEmpty());
}

TEST(LongPressGestureRecognizer, canHandleLongPressWithSingleTap) {
    auto container = makeContainer(0, 0, 100, 100);

    auto childView = createView(25, 25, 25, 25);
    container->view->addChild(childView);

    auto singleTapSnapshot = addSingleTapGesture(childView);
    auto longPressSnapshot = addLongPressGesture(childView);

    container->dispatchEvent(TouchEventTypeDown, 30, 35);
    container->dispatchEvent(
        TouchEventTypeMoved, 30, 35, 0, 0, 1, GesturesConfiguration::getDefault().longPressTimeout);
    ASSERT_EQ(GestureRecognizerStatePossible, singleTapSnapshot->state);
    ASSERT_EQ(GestureRecognizerStateBegan, longPressSnapshot->state);

    container->dispatchEvent(TouchEventTypeUp, 30, 35);
    ASSERT_EQ(GestureRecognizerStatePossible, singleTapSnapshot->state);
    ASSERT_EQ(GestureRecognizerStateEnded, longPressSnapshot->state);

    ASSERT_TRUE(container->root->getTouchDispatcher().isEmpty());
}

TEST(LongPressGestureRecognizer, triggersLongPressWithoutWaitingForTapFailure) {
    auto container = makeContainer(0, 0, 100, 100);

    auto childView = createView(25, 25, 25, 25);
    container->view->addChild(childView);

    auto singleTapSnapshot = addSingleTapGesture(childView);
    auto longPressSnapshot = addLongPressGesture(childView);

    Valdi::castOrNull<LongPressGestureRecognizer>(longPressSnapshot->parent)
        ->setLongPressTimeout(Duration::fromSeconds(0.1));

    container->dispatchEvent(TouchEventTypeDown, 30, 35);
    container->dispatchEvent(TouchEventTypeMoved, 30, 35, 0, 0, 1, Duration::fromSeconds(0.15));
    ASSERT_EQ(GestureRecognizerStatePossible, singleTapSnapshot->state);
    ASSERT_EQ(GestureRecognizerStateBegan, longPressSnapshot->state);

    container->dispatchEvent(TouchEventTypeUp, 30, 35);
    ASSERT_EQ(GestureRecognizerStatePossible, singleTapSnapshot->state);
    ASSERT_EQ(GestureRecognizerStateEnded, longPressSnapshot->state);

    ASSERT_TRUE(container->root->getTouchDispatcher().isEmpty());
}

TEST(LongPressGestureRecognizer, canHandleLongPressWithSingleTapWithDoubleTap) {
    auto container = makeContainer(0, 0, 100, 100);

    auto childView = createView(25, 25, 25, 25);
    container->view->addChild(childView);

    auto singleTapSnapshot = addSingleTapGesture(childView);
    auto doubleTapSnapshot = addDoubleTapGesture(childView);
    auto longPressSnapshot = addLongPressGesture(childView);

    container->dispatchEvent(TouchEventTypeDown, 30, 35);
    container->dispatchEvent(
        TouchEventTypeMoved, 30, 35, 0, 0, 1, GesturesConfiguration::getDefault().longPressTimeout);
    ASSERT_EQ(GestureRecognizerStatePossible, singleTapSnapshot->state);
    ASSERT_EQ(GestureRecognizerStatePossible, doubleTapSnapshot->state);
    ASSERT_EQ(GestureRecognizerStateBegan, longPressSnapshot->state);

    container->dispatchEvent(TouchEventTypeUp, 30, 35);
    ASSERT_EQ(GestureRecognizerStatePossible, singleTapSnapshot->state);
    ASSERT_EQ(GestureRecognizerStatePossible, doubleTapSnapshot->state);
    ASSERT_EQ(GestureRecognizerStateEnded, longPressSnapshot->state);

    ASSERT_TRUE(container->root->getTouchDispatcher().isEmpty());
}

TEST(LongPressGestureRecognizer, canHandleLongPressQuickTwo) {
    auto container = makeContainer(0, 0, 100, 100);

    auto childView = createView(25, 25, 25, 25);
    container->view->addChild(childView);

    auto longPressSnapshot = addLongPressGesture(childView);

    container->dispatchEvent(TouchEventTypeDown, 30, 35);
    container->dispatchEvent(
        TouchEventTypeMoved, 30, 35, 0, 0, 1, GesturesConfiguration::getDefault().longPressTimeout);
    ASSERT_EQ(GestureRecognizerStateBegan, longPressSnapshot->state);

    container->dispatchEvent(TouchEventTypeUp, 30, 35);
    ASSERT_EQ(GestureRecognizerStateEnded, longPressSnapshot->state);

    container->dispatchEvent(TouchEventTypeDown, 30, 35);
    container->dispatchEvent(
        TouchEventTypeMoved, 30, 35, 0, 0, 1, GesturesConfiguration::getDefault().longPressTimeout);
    ASSERT_EQ(GestureRecognizerStateBegan, longPressSnapshot->state);

    container->dispatchEvent(TouchEventTypeUp, 30, 35);
    ASSERT_EQ(GestureRecognizerStateEnded, longPressSnapshot->state);

    ASSERT_EQ(4, longPressSnapshot->counter);
    ASSERT_TRUE(container->root->getTouchDispatcher().isEmpty());
}

TEST(LongPressGestureRecognizer, canHandleLongPressWithDrag) {
    auto container = makeContainer(0, 0, 100, 100);

    auto childView = createView(25, 25, 25, 25);
    container->view->addChild(childView);

    auto dragSnapshot = addDragGesture(childView);
    auto longPressSnapshot = addLongPressGesture(childView);

    container->dispatchEvent(TouchEventTypeDown, 30, 35);
    ASSERT_EQ(GestureRecognizerStatePossible, dragSnapshot->state);
    ASSERT_EQ(GestureRecognizerStatePossible, longPressSnapshot->state);

    container->dispatchEvent(
        TouchEventTypeMoved, 30, 35, 0, 0, 1, GesturesConfiguration::getDefault().longPressTimeout);
    ASSERT_EQ(GestureRecognizerStatePossible, dragSnapshot->state);
    ASSERT_EQ(GestureRecognizerStateBegan, longPressSnapshot->state);

    container->dispatchEvent(TouchEventTypeMoved, 30 + GesturesConfiguration::getDefault().dragTouchSlop, 35);
    ASSERT_EQ(GestureRecognizerStatePossible, dragSnapshot->state);
    ASSERT_EQ(GestureRecognizerStateChanged, longPressSnapshot->state);

    container->dispatchEvent(TouchEventTypeUp, 30, 35);
    ASSERT_EQ(GestureRecognizerStatePossible, dragSnapshot->state);
    ASSERT_EQ(GestureRecognizerStateEnded, longPressSnapshot->state);

    ASSERT_TRUE(container->root->getTouchDispatcher().isEmpty());
}

TEST(LongPressGestureRecognizer, canHandleLongPressWithoutDrag) {
    auto container = makeContainer(0, 0, 100, 100);

    auto childView = createView(25, 25, 25, 25);
    container->view->addChild(childView);

    auto dragSnapshot = addDragGesture(childView);
    auto longPressSnapshot = addLongPressGesture(childView);

    container->dispatchEvent(TouchEventTypeDown, 30, 35);
    ASSERT_EQ(GestureRecognizerStatePossible, longPressSnapshot->state);

    container->dispatchEvent(
        TouchEventTypeMoved, 30, 35, 0, 0, 1, GesturesConfiguration::getDefault().longPressTimeout);
    ASSERT_EQ(GestureRecognizerStateBegan, longPressSnapshot->state);

    container->dispatchEvent(TouchEventTypeMoved, 30 + GesturesConfiguration::getDefault().dragTouchSlop, 35);
    ASSERT_EQ(GestureRecognizerStateChanged, longPressSnapshot->state);

    container->dispatchEvent(TouchEventTypeUp, 30, 35);
    ASSERT_EQ(GestureRecognizerStateEnded, longPressSnapshot->state);

    ASSERT_TRUE(container->root->getTouchDispatcher().isEmpty());
}

} // namespace snap::drawing
