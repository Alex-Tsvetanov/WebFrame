#include <gtest/gtest.h>

#include "TestGestureUtils.hpp"

using namespace Valdi;

namespace snap::drawing {

TEST(DoubleTapGestureRecognizer, canHandleDoubleTap) {
    auto container = makeContainer(0, 0, 100, 100);

    auto childView = createView(25, 25, 25, 25);
    container->view->addChild(childView);

    auto doubleTapSnapshot = addDoubleTapGesture(childView);

    container->dispatchEvent(TouchEventTypeDown, 30, 35);
    container->dispatchEvent(TouchEventTypeUp, 30, 35);

    ASSERT_EQ(GestureRecognizerStatePossible, doubleTapSnapshot->state);

    ASSERT_FALSE(container->root->getTouchDispatcher().isEmpty());

    container->dispatchEvent(TouchEventTypeDown, 30, 35);
    container->dispatchEvent(TouchEventTypeUp, 30, 35);

    ASSERT_EQ(GestureRecognizerStateEnded, doubleTapSnapshot->state);

    ASSERT_TRUE(container->root->getTouchDispatcher().isEmpty());
}

TEST(DoubleTapGestureRecognizer, canHandleDoubleTapWithSingleTap) {
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

    container->dispatchEvent(TouchEventTypeDown, 30, 35);
    container->dispatchEvent(TouchEventTypeUp, 30, 35);

    ASSERT_EQ(GestureRecognizerStatePossible, singleTapSnapshot->state);
    ASSERT_EQ(GestureRecognizerStateEnded, doubleTapSnapshot->state);

    ASSERT_TRUE(container->root->getTouchDispatcher().isEmpty());
}

} // namespace snap::drawing
