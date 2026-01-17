#include <gtest/gtest.h>

#include "TestGestureUtils.hpp"

using namespace Valdi;

namespace snap::drawing {

TEST(DragGestureRecognizer, doesHandleWheel) {
    auto container = makeContainer(0, 0, 100, 100);

    auto childView = createView(25, 25, 25, 25);
    container->view->addChild(childView);

    auto wheelSnapshot = addWheelGesture(childView);

    container->dispatchEvent(TouchEventTypeWheel, 30, 35, 0, 10);
    ASSERT_EQ(GestureRecognizerStateBegan, wheelSnapshot->state);
    ASSERT_EQ(5, wheelSnapshot->location.x);
    ASSERT_EQ(10, wheelSnapshot->location.y);
    ASSERT_EQ(0, wheelSnapshot->direction.dx);
    ASSERT_EQ(10, wheelSnapshot->direction.dy);

    container->dispatchEvent(TouchEventTypeWheel, 35, 40, 5, 5);
    ASSERT_EQ(GestureRecognizerStateBegan, wheelSnapshot->state);
    ASSERT_EQ(10, wheelSnapshot->location.x);
    ASSERT_EQ(15, wheelSnapshot->location.y);
    ASSERT_EQ(5, wheelSnapshot->direction.dx);
    ASSERT_EQ(5, wheelSnapshot->direction.dy);

    ASSERT_TRUE(container->root->getTouchDispatcher().isEmpty());
}

} // namespace snap::drawing
