#include <gtest/gtest.h>

#include "TestGestureUtils.hpp"

using namespace Valdi;

namespace snap::drawing {

TEST(SingleTapGestureRecognizer, canHandleSingleTap) {
    auto container = makeContainer(0, 0, 100, 100);

    auto childView = createView(25, 25, 25, 25);
    container->view->addChild(childView);

    auto singleTapSnapshot = addSingleTapGesture(childView);

    container->dispatchEvent(TouchEventTypeDown, 30, 35);
    container->dispatchEvent(TouchEventTypeUp, 30, 35);

    ASSERT_EQ(GestureRecognizerStateEnded, singleTapSnapshot->state);

    ASSERT_TRUE(container->root->getTouchDispatcher().isEmpty());
}

TEST(SingleTapGestureRecognizer, canHandleSingleTapWithDoubleTap) {
    auto container = makeContainer(0, 0, 100, 100);

    auto childView = createView(25, 25, 25, 25);
    container->view->addChild(childView);

    auto singleTapSnapshot = addSingleTapGesture(childView);
    auto doubleTapSnapshot = addDoubleTapGesture(childView);

    container->dispatchEvent(TouchEventTypeDown, 30, 35);
    container->dispatchEvent(TouchEventTypeUp, 30, 35);

    ASSERT_EQ(GestureRecognizerStatePossible, singleTapSnapshot->state);
    ASSERT_EQ(GestureRecognizerStatePossible, doubleTapSnapshot->state);

    ASSERT_FALSE(container->root->getTouchDispatcher().isEmpty());

    container->dispatchEvent(TouchEventTypeIdle, 30, 35, 0, 0, 1, GesturesConfiguration::getDefault().longPressTimeout);

    ASSERT_EQ(GestureRecognizerStateEnded, singleTapSnapshot->state);
    ASSERT_EQ(GestureRecognizerStatePossible, doubleTapSnapshot->state);

    ASSERT_TRUE(container->root->getTouchDispatcher().isEmpty());
}

TEST(SingleTapGestureRecognizer, canHandleSingleTapQuickTwo) {
    auto container = makeContainer(0, 0, 100, 100);

    auto childView = createView(25, 25, 25, 25);
    container->view->addChild(childView);

    auto singleTapSnapshot = addSingleTapGesture(childView);

    container->dispatchEvent(TouchEventTypeDown, 30, 35);
    container->dispatchEvent(TouchEventTypeUp, 30, 35);

    ASSERT_EQ(GestureRecognizerStateEnded, singleTapSnapshot->state);
    ASSERT_EQ(1, singleTapSnapshot->counter);

    ASSERT_TRUE(container->root->getTouchDispatcher().isEmpty());

    container->dispatchEvent(TouchEventTypeDown, 30, 35);
    container->dispatchEvent(TouchEventTypeUp, 30, 35);

    ASSERT_EQ(GestureRecognizerStateEnded, singleTapSnapshot->state);
    ASSERT_EQ(2, singleTapSnapshot->counter);

    ASSERT_TRUE(container->root->getTouchDispatcher().isEmpty());
}

TEST(SingleTapGestureRecognizer, canHandleSingleTapWithDrag) {
    auto container = makeContainer(0, 0, 100, 100);

    auto childView = createView(25, 25, 25, 25);
    container->view->addChild(childView);

    auto singleTapSnapshot = addSingleTapGesture(childView);
    auto dragSnapshot = addDragGesture(childView);

    container->dispatchEvent(TouchEventTypeDown, 30, 35);
    ASSERT_EQ(GestureRecognizerStatePossible, singleTapSnapshot->state);
    ASSERT_EQ(GestureRecognizerStatePossible, dragSnapshot->state);

    container->dispatchEvent(TouchEventTypeMoved, 30, 35);
    ASSERT_EQ(GestureRecognizerStatePossible, singleTapSnapshot->state);
    ASSERT_EQ(GestureRecognizerStatePossible, dragSnapshot->state);

    container->dispatchEvent(TouchEventTypeUp, 30, 35);
    ASSERT_EQ(GestureRecognizerStateEnded, singleTapSnapshot->state);
    ASSERT_EQ(GestureRecognizerStatePossible, dragSnapshot->state);

    ASSERT_TRUE(container->root->getTouchDispatcher().isEmpty());
}

TEST(SingleTapGestureRecognizer, canHandleSingleTapWithFailedDoubleTap) {
    auto container = makeContainer(0, 0, 100, 100);

    auto childView = createView(25, 25, 25, 25);
    container->view->addChild(childView);

    auto singleTapSnapshot = addSingleTapGesture(childView);
    auto doubleTapSnapshot = addDoubleTapGesture(childView);

    container->dispatchEvent(TouchEventTypeDown, 30, 35);
    ASSERT_EQ(GestureRecognizerStatePossible, singleTapSnapshot->state);
    ASSERT_EQ(GestureRecognizerStatePossible, doubleTapSnapshot->state);

    container->dispatchEvent(TouchEventTypeUp, 30, 35);
    ASSERT_EQ(GestureRecognizerStatePossible, singleTapSnapshot->state);
    ASSERT_EQ(GestureRecognizerStatePossible, doubleTapSnapshot->state);

    container->dispatchEvent(TouchEventTypeDown, 30, 35);
    ASSERT_EQ(GestureRecognizerStatePossible, singleTapSnapshot->state);
    ASSERT_EQ(GestureRecognizerStatePossible, doubleTapSnapshot->state);

    container->dispatchEvent(TouchEventTypeUp, 30, 35, 0, 0, 1, GesturesConfiguration::getDefault().longPressTimeout);
    ASSERT_EQ(GestureRecognizerStateEnded, singleTapSnapshot->state);
    ASSERT_EQ(GestureRecognizerStatePossible, doubleTapSnapshot->state);

    ASSERT_TRUE(container->root->getTouchDispatcher().isEmpty());
}

} // namespace snap::drawing
