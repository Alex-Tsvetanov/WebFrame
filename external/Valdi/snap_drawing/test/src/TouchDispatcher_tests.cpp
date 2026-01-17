#include <gtest/gtest.h>

#include "TestGestureUtils.hpp"

using namespace Valdi;

namespace snap::drawing {

TEST(Layer, canResolveCoordinates) {
    auto rootView = createView(25, 25, 100, 100);
    auto middleView = createView(5, 5, 50, 50);
    auto leafView = createView(7, 9, 10, 10);

    rootView->addChild(middleView);
    middleView->addChild(leafView);

    ASSERT_EQ(Rect::makeXYWH(25, 25, 100, 100), rootView->getVisualFrame());
    ASSERT_EQ(Rect::makeXYWH(5, 5, 50, 50), middleView->getVisualFrame());
    ASSERT_EQ(Rect::makeXYWH(7, 9, 10, 10), leafView->getVisualFrame());

    ASSERT_EQ(Rect::makeXYWH(25, 25, 100, 100), rootView->getAbsoluteVisualFrame());
    ASSERT_EQ(Rect::makeXYWH(30, 30, 50, 50), middleView->getAbsoluteVisualFrame());
    ASSERT_EQ(Rect::makeXYWH(37, 39, 10, 10), leafView->getAbsoluteVisualFrame());

    // 7, 9
    // 12, 14
    //

    auto result = rootView->convertPointToLayer(Point::make(2, 15), rootView);
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(Point::make(2, 15), result.value());

    result = rootView->convertPointToLayer(Point::make(2, 15), middleView);
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(Point::make(-3, 10), result.value());

    result = rootView->convertPointToLayer(Point::make(2, 15), leafView);
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(Point::make(-10, 1), result.value());

    result = middleView->convertPointToLayer(Point::make(-3, 10), leafView);
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(Point::make(-10, 1), result.value());
}

TEST(Layer, canResolveCoordinatesWithTranslate) {
    auto rootView = createView(25, 25, 100, 100);
    auto middleView = createView(5, 5, 50, 50);
    auto leafView = createView(7, 9, 10, 10);

    rootView->addChild(middleView);
    middleView->addChild(leafView);

    middleView->setTranslationX(4);
    middleView->setTranslationY(-3);

    ASSERT_EQ(Rect::makeXYWH(25, 25, 100, 100), rootView->getVisualFrame());
    ASSERT_EQ(Rect::makeXYWH(9, 2, 50, 50), middleView->getVisualFrame());
    ASSERT_EQ(Rect::makeXYWH(7, 9, 10, 10), leafView->getVisualFrame());

    ASSERT_EQ(Rect::makeXYWH(25, 25, 100, 100), rootView->getAbsoluteVisualFrame());
    ASSERT_EQ(Rect::makeXYWH(34, 27, 50, 50), middleView->getAbsoluteVisualFrame());
    ASSERT_EQ(Rect::makeXYWH(41, 36, 10, 10), leafView->getAbsoluteVisualFrame());

    auto result = rootView->convertPointToLayer(Point::make(2, 15), rootView);
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(Point::make(2, 15), result.value());

    result = rootView->convertPointToLayer(Point::make(2, 15), middleView);
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(Point::make(-7, 13), result.value());

    result = rootView->convertPointToLayer(Point::make(2, 15), leafView);
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(Point::make(-14, 4), result.value());

    result = middleView->convertPointToLayer(Point::make(-7, 13), leafView);
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(Point::make(-14, 4), result.value());
}

TEST(Layer, canResolveCoordinatesWithScale) {
    auto rootView = createView(25, 25, 100, 100);
    auto middleView = createView(5, 5, 50, 50);
    auto leafView = createView(7, 9, 10, 10);

    rootView->addChild(middleView);
    middleView->addChild(leafView);

    middleView->setScaleX(2);
    middleView->setScaleY(4);

    ASSERT_EQ(Rect::makeXYWH(25, 25, 100, 100), rootView->getVisualFrame());
    ASSERT_EQ(Rect::makeXYWH(-20, -70, 100, 200), middleView->getVisualFrame());
    ASSERT_EQ(Rect::makeXYWH(7, 9, 10, 10), leafView->getVisualFrame());

    ASSERT_EQ(Rect::makeXYWH(25, 25, 100, 100), rootView->getAbsoluteVisualFrame());
    ASSERT_EQ(Rect::makeXYWH(5, -45, 100, 200), middleView->getAbsoluteVisualFrame());
    ASSERT_EQ(Rect::makeXYWH(19, -9, 20, 40), leafView->getAbsoluteVisualFrame());

    auto result = rootView->convertPointToLayer(Point::make(2, 15), rootView);
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(Point::make(2, 15), result.value());

    result = rootView->convertPointToLayer(Point::make(2, 15), middleView);
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(Point::make(11, 21.25), result.value());

    result = rootView->convertPointToLayer(Point::make(2, 15), leafView);
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(Point::make(4, 12.25), result.value());

    result = middleView->convertPointToLayer(Point::make(11, 21.25), leafView);
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(Point::make(4, 12.25), result.value());
}

//
// TEST(Layer, canResolveCoordinatesWithScaleTranslate) {
//
//}

TEST(TouchDispatcher, transitionFromStates) {
    auto container = makeContainer(0, 0, 100, 100);

    auto childView = createView(25, 25, 25, 25);

    container->view->addChild(childView);

    auto snapshot = addCustomTouchGesture(childView);

    ASSERT_EQ(GestureRecognizerStatePossible, snapshot->state);
    ASSERT_EQ(Point::makeEmpty(), snapshot->location);

    container->dispatchEvent(TouchEventTypeDown, 30, 35);

    ASSERT_EQ(GestureRecognizerStateBegan, snapshot->state);
    ASSERT_EQ(5, snapshot->location.x);
    ASSERT_EQ(10, snapshot->location.y);

    container->dispatchEvent(TouchEventTypeMoved, 40, 40);

    ASSERT_EQ(GestureRecognizerStateChanged, snapshot->state);
    ASSERT_EQ(15, snapshot->location.x);
    ASSERT_EQ(15, snapshot->location.y);

    container->dispatchEvent(TouchEventTypeMoved, 1000, 1000);

    ASSERT_EQ(GestureRecognizerStateChanged, snapshot->state);
    ASSERT_EQ(975, snapshot->location.x);
    ASSERT_EQ(975, snapshot->location.y);

    container->dispatchEvent(TouchEventTypeUp, 5, 5);

    ASSERT_EQ(GestureRecognizerStateEnded, snapshot->state);
    ASSERT_EQ(-20, snapshot->location.x);
    ASSERT_EQ(-20, snapshot->location.y);

    ASSERT_TRUE(container->root->getTouchDispatcher().isEmpty());
}

TEST(TouchDispatcher, canHandleSimpleGesture) {
    auto container = makeContainer(0, 0, 100, 100);

    auto childView = createView(25, 25, 25, 25);

    container->view->addChild(childView);

    auto snapshot = addCustomTapGesture(childView, 1);

    // Touch outside
    container->dispatchEvent(TouchEventTypeDown, 20, 20);

    ASSERT_EQ(GestureRecognizerStatePossible, snapshot->state);
    ASSERT_EQ(Point::makeEmpty(), snapshot->location);

    // Touch inside
    container->dispatchEvent(TouchEventTypeDown, 30, 35);

    // This is a tap, so it should still be null at this point
    ASSERT_EQ(GestureRecognizerStatePossible, snapshot->state);
    ASSERT_EQ(Point::makeEmpty(), snapshot->location);

    // Lift finger
    container->dispatchEvent(TouchEventTypeUp, 30, 35);

    ASSERT_EQ(GestureRecognizerStateEnded, snapshot->state);
    ASSERT_EQ(5, snapshot->location.x);
    ASSERT_EQ(10, snapshot->location.y);

    ASSERT_TRUE(container->root->getTouchDispatcher().isEmpty());
}

TEST(TouchDispatcher, canCalculateRelativeCoordinates) {
    auto root = makeRoot();

    auto rootView = createView(10, 10, 100, 100);
    auto childView = createView(15, 15, 50, 50);
    auto childView2 = createView(17, 17, 25, 25);

    rootView->addChild(childView);
    childView->addChild(childView2);

    root->setContentLayer(rootView, ContentLayerSizingModeMatchSize);

    auto snapshot = addCustomTouchGesture(childView2);

    root->dispatchTouchEvent(createTouchEvent(TouchEventTypeDown, 49, 48));

    ASSERT_EQ(GestureRecognizerStateBegan, snapshot->state);
    ASSERT_EQ(17, snapshot->location.x);
    ASSERT_EQ(16, snapshot->location.y);

    ASSERT_FALSE(root->getTouchDispatcher().isEmpty());
    root->dispatchTouchEvent(createTouchEvent(TouchEventTypeUp, 0, 0));

    ASSERT_TRUE(root->getTouchDispatcher().isEmpty());
}

TEST(TouchDispatcher, onlyRecognizeOnHit) {
    auto container = makeContainer(10, 10, 100, 100);

    auto childView = createView(15, 15, 50, 50);
    auto childView2 = createView(17, 17, 25, 25);

    container->view->addChild(childView);
    childView->addChild(childView2);

    auto snapshot = addCustomTouchGesture(childView2);

    container->dispatchEvent(TouchEventTypeDown, 67, 48);

    ASSERT_EQ(GestureRecognizerStatePossible, snapshot->state);
    ASSERT_EQ(Point::makeEmpty(), snapshot->location);

    container->dispatchEvent(TouchEventTypeUp, 67, 48);

    container->dispatchEvent(TouchEventTypeDown, 49, 67);

    ASSERT_EQ(GestureRecognizerStatePossible, snapshot->state);
    ASSERT_EQ(Point::makeEmpty(), snapshot->location);

    container->dispatchEvent(TouchEventTypeUp, 49, 67);
    container->dispatchEvent(TouchEventTypeDown, 67, 67);

    ASSERT_EQ(GestureRecognizerStatePossible, snapshot->state);
    ASSERT_EQ(Point::makeEmpty(), snapshot->location);

    container->dispatchEvent(TouchEventTypeUp, 67, 48);

    // This time we should hit
    container->dispatchEvent(TouchEventTypeDown, 50, 50);

    ASSERT_NE(GestureRecognizerStatePossible, snapshot->state);
    ASSERT_NE(Point::makeEmpty(), snapshot->location);

    ASSERT_FALSE(container->root->getTouchDispatcher().isEmpty());

    container->dispatchEvent(TouchEventTypeUp, 0, 0);

    ASSERT_TRUE(container->root->getTouchDispatcher().isEmpty());
}

TEST(TouchDispatcher, siblingsCanPreventHitTest) {
    auto root = makeRoot();
    auto rootView = createView(0, 0, 100, 100);
    auto childView = createView(15, 15, 50, 50);
    auto childView2 = createView(15, 15, 50, 50);

    rootView->addChild(childView);
    rootView->addChild(childView2);
    root->setContentLayer(rootView, ContentLayerSizingModeMatchSize);

    auto snapshot = addCustomTouchGesture(childView);

    root->dispatchTouchEvent(createTouchEvent(TouchEventTypeDown, 20, 20));

    ASSERT_EQ(GestureRecognizerStatePossible, snapshot->state);

    ASSERT_TRUE(root->getTouchDispatcher().isEmpty());
}

TEST(TouchDispatcher, siblingsDontPreventHitTestWithTouchDisabled) {
    auto root = makeRoot();
    auto rootView = createView(0, 0, 100, 100);
    auto childView = createView(15, 15, 50, 50);
    auto childView2 = createView(15, 15, 50, 50);

    rootView->addChild(childView);
    rootView->addChild(childView2);
    root->setContentLayer(rootView, ContentLayerSizingModeMatchSize);

    auto snapshot = addCustomTouchGesture(childView);
    childView2->setTouchEnabled(false);

    root->dispatchTouchEvent(createTouchEvent(TouchEventTypeDown, 20, 20));

    ASSERT_EQ(GestureRecognizerStateBegan, snapshot->state);

    ASSERT_FALSE(root->getTouchDispatcher().isEmpty());

    root->dispatchTouchEvent(createTouchEvent(TouchEventTypeUp, 0, 0));

    ASSERT_TRUE(root->getTouchDispatcher().isEmpty());
}

TEST(TouchDispatcher, onlyConsiderTopSiblings) {
    auto root = makeRoot();
    auto rootView = createView(0, 0, 100, 100);
    auto childView = createView(15, 15, 50, 50);
    auto childView2 = createView(15, 15, 50, 50);

    rootView->addChild(childView);
    rootView->addChild(childView2);
    root->setContentLayer(rootView, ContentLayerSizingModeMatchSize);

    auto stateChild = addCustomTouchGesture(childView);
    auto stateChild2 = addCustomTouchGesture(childView2);

    root->dispatchTouchEvent(createTouchEvent(TouchEventTypeDown, 20, 20));

    ASSERT_EQ(GestureRecognizerStatePossible, stateChild->state);
    ASSERT_EQ(GestureRecognizerStateBegan, stateChild2->state);

    ASSERT_FALSE(root->getTouchDispatcher().isEmpty());

    root->dispatchTouchEvent(createTouchEvent(TouchEventTypeUp, 0, 0));

    ASSERT_TRUE(root->getTouchDispatcher().isEmpty());
}

TEST(TouchDispatcher, canHandleSimulatenousGesture) {
    auto root = makeRoot();

    auto rootView = createView(0, 0, 100, 100);
    auto childView = createView(25, 25, 25, 25);
    auto childView2 = createView(5, 5, 25, 25);
    rootView->addChild(childView);
    childView->addChild(childView2);

    root->setContentLayer(rootView, ContentLayerSizingModeMatchSize);

    auto tap1 = addCustomTapGesture(childView, 1, true);
    auto tap2 = addCustomTapGesture(childView2, 1, true);

    // Touch inside
    root->dispatchTouchEvent(createTouchEvent(TouchEventTypeDown, 30, 35));
    root->dispatchTouchEvent(createTouchEvent(TouchEventTypeUp, 30, 35));

    ASSERT_EQ(GestureRecognizerStateEnded, tap1->state);
    ASSERT_EQ(5, tap1->location.x);
    ASSERT_EQ(10, tap1->location.y);

    ASSERT_EQ(GestureRecognizerStateEnded, tap2->state);
    ASSERT_EQ(0, tap2->location.x);
    ASSERT_EQ(5, tap2->location.y);

    ASSERT_TRUE(root->getTouchDispatcher().isEmpty());
}

TEST(TouchDispatcher, canHandleSimulatenousDelayedGesture) {
    auto root = makeRoot();

    auto rootView = createView(0, 0, 100, 100);
    auto childView = createView(25, 25, 25, 25);
    auto childView2 = createView(5, 5, 25, 25);

    rootView->addChild(childView);
    childView->addChild(childView2);
    root->setContentLayer(rootView, ContentLayerSizingModeMatchSize);

    auto touch = addCustomTouchGesture(childView);
    auto tap = addCustomTapGesture(childView2, 1);

    // Touch inside
    root->dispatchTouchEvent(createTouchEvent(TouchEventTypeDown, 30, 35));

    ASSERT_EQ(GestureRecognizerStateBegan, touch->state);
    ASSERT_EQ(5, touch->location.x);
    ASSERT_EQ(10, touch->location.y);

    ASSERT_EQ(GestureRecognizerStatePossible, tap->state);
    ASSERT_EQ(0, tap->location.x);
    ASSERT_EQ(0, tap->location.y);

    // We move very slightly
    root->dispatchTouchEvent(createTouchEvent(TouchEventTypeMoved, 31, 35));

    ASSERT_EQ(GestureRecognizerStateChanged, touch->state);
    ASSERT_EQ(6, touch->location.x);
    ASSERT_EQ(10, touch->location.y);

    // Tap should still be off
    ASSERT_EQ(GestureRecognizerStatePossible, tap->state);
    ASSERT_EQ(0, tap->location.x);
    ASSERT_EQ(0, tap->location.y);

    // We then release
    root->dispatchTouchEvent(createTouchEvent(TouchEventTypeUp, 31, 35));

    ASSERT_EQ(GestureRecognizerStateEnded, touch->state);
    ASSERT_EQ(6, touch->location.x);
    ASSERT_EQ(10, touch->location.y);

    // The tap gesture should have triggered, because we barely moved
    ASSERT_EQ(GestureRecognizerStateEnded, tap->state);
    ASSERT_EQ(1, tap->location.x);
    ASSERT_EQ(5, tap->location.y);

    ASSERT_TRUE(root->getTouchDispatcher().isEmpty());
}

TEST(TouchDispatcher, canHandleConflicts) {
    auto root = makeRoot();

    auto rootView = createView(0, 0, 100, 100);
    auto childView = createView(25, 25, 25, 25);
    auto childView2 = createView(30, 30, 25, 25);

    rootView->addChild(childView);
    rootView->addChild(childView2);

    root->setContentLayer(rootView, ContentLayerSizingModeMatchSize);

    auto tap1 = addCustomTapGesture(childView, 1, false);

    auto tap2 = addCustomTapGesture(childView2, 1, false);

    // Touch inside
    root->dispatchTouchEvent(createTouchEvent(TouchEventTypeDown, 30, 35));
    root->dispatchTouchEvent(createTouchEvent(TouchEventTypeUp, 30, 35));

    // Only tap2 should be called, since it's the highest child.
    ASSERT_EQ(GestureRecognizerStateEnded, tap2->state);
    ASSERT_EQ(0, tap2->location.x);
    ASSERT_EQ(5, tap2->location.y);

    ASSERT_EQ(GestureRecognizerStatePossible, tap1->state);
    ASSERT_EQ(0, tap1->location.x);
    ASSERT_EQ(0, tap1->location.y);

    ASSERT_TRUE(root->getTouchDispatcher().isEmpty());
}

TEST(TouchDispatcher, canCancelOtherGesturesOnStart) {
    auto root = makeRoot();

    auto rootView = createView(0, 0, 100, 100);
    auto childView = createView(25, 25, 25, 25);
    auto childView2 = createView(5, 5, 25, 25);
    rootView->addChild(childView);
    childView->addChild(childView2);

    root->setContentLayer(rootView, ContentLayerSizingModeMatchSize);

    auto drag = addDragGesture(childView, /*shouldCancelOtherGesturesOnStart*/ true);
    auto touch = addCustomTouchGesture(childView2);

    // Touch inside
    root->dispatchTouchEvent(createTouchEvent(TouchEventTypeDown, 30, 35));

    ASSERT_EQ(GestureRecognizerStatePossible, drag->state);
    ASSERT_EQ(GestureRecognizerStateBegan, touch->state);

    root->dispatchTouchEvent(
        createTouchEvent(TouchEventTypeMoved, 30 + (GesturesConfiguration::getDefault().dragTouchSlop / 2), 35));

    ASSERT_EQ(GestureRecognizerStatePossible, drag->state);
    ASSERT_EQ(GestureRecognizerStateChanged, touch->state);

    root->dispatchTouchEvent(
        createTouchEvent(TouchEventTypeMoved, 30 + GesturesConfiguration::getDefault().dragTouchSlop, 35));

    ASSERT_EQ(GestureRecognizerStateBegan, drag->state);
    ASSERT_EQ(GestureRecognizerStateEnded, touch->state);

    root->dispatchTouchEvent(
        createTouchEvent(TouchEventTypeUp, 30 + GesturesConfiguration::getDefault().dragTouchSlop, 35));

    ASSERT_EQ(GestureRecognizerStateEnded, drag->state);
    ASSERT_EQ(GestureRecognizerStateEnded, touch->state);

    ASSERT_TRUE(root->getTouchDispatcher().isEmpty());
}

TEST(TouchDispatcher, canRefreshTouchGestures) {
    auto root = makeRoot();

    auto rootView = createView(10, 10, 100, 100);
    auto childView = createView(15, 15, 50, 50);

    rootView->addChild(childView);

    root->setContentLayer(rootView, ContentLayerSizingModeMatchSize);

    auto snapshot = addCustomTouchGesture(childView);

    auto event = createTouchEvent(TouchEventTypeDown, 17, 16);

    root->dispatchTouchEvent(event);

    ASSERT_EQ(GestureRecognizerStateBegan, snapshot->state);
    ASSERT_EQ(2, snapshot->location.x);
    ASSERT_EQ(1, snapshot->location.y);
    ASSERT_EQ(1, snapshot->counter);

    ASSERT_FALSE(root->getTouchDispatcher().isEmpty());

    // No refreshes when called in under 10ms.
    ASSERT_FALSE(root->refreshTouches(event.getTime()));
    ASSERT_FALSE(root->refreshTouches(event.getTime() + Duration::fromMilliseconds(5)));

    // Refresh for >10ms succeeds.
    ASSERT_TRUE(root->refreshTouches(event.getTime() + Duration::fromMilliseconds(15)));

    // No state change because it didn't move.
    ASSERT_EQ(GestureRecognizerStateBegan, snapshot->state);
    ASSERT_EQ(1, snapshot->counter);

    // Ensure state changes after a move.
    event = createTouchEvent(TouchEventTypeMoved, 18, 17);
    root->dispatchTouchEvent(event);

    ASSERT_EQ(GestureRecognizerStateChanged, snapshot->state);
    ASSERT_EQ(3, snapshot->location.x);
    ASSERT_EQ(2, snapshot->location.y);
    ASSERT_EQ(2, snapshot->counter);
}

} // namespace snap::drawing
