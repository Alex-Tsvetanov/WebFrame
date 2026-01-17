//
//  SCValdiScrollViewDelegate.m
//  Valdi
//
//  Created by Simon Corsin on 5/13/18.
//

#import "valdi_core/SCValdiScrollViewDelegate.h"

#import "valdi_core/UIView+ValdiObjects.h"
#import "valdi_core/SCValdiIScrollPerfLoggerBridge.h"

#import "valdi_core/SCValdiError.h"
#import "valdi_core/SCValdiLogger.h"

@implementation SCValdiScrollViewDelegate {
    BOOL _readjustingContentOffset;
    CGPoint _previousVelocity;
    CGPoint _currentVelocity;
    BOOL _dragging;
}

- (instancetype)init
{
    self = [super init];

    if (self) {
        _cancelsTouchesOnScroll = YES;
        _dragging = NO;
    }

    return self;
}

- (void)updateCurrentVelocity:(UIScrollView *)scrollView 
{
//    NOTE(rjaber): It was observed that the `[[scrollView panGestureRecognizer] velocityInView:scrollView]` was returning a
//                  velocity that's different than what is provided in `scrollViewWillEndDragging:withVelocity:...`. From
//                  initial observations it seems a weighted average was being used. To find the weights following equation was
//                  assumed: v_smooth[t] = (w1*v[t]/(w1+w2+w3) + w2*v[t - 1]/(w1+w2+w3) + w3*v[t - 2]/(w1+w2+w3).
//                  So for 3 different gestures I captured the velocities and solved the system of equations. This resulted in
//                  following weight values: w1 = 1, w2 = 3, w3 = 0. The iOS smoothed velocity is also divided by 1000. This
//                  was ignored since the typescript layer counteracts that.

    CGPoint panVelocity = [[scrollView panGestureRecognizer] velocityInView:scrollView];
    
    CGFloat velocityX = ((3*_previousVelocity.x) + panVelocity.x)/4;
    CGFloat velocityY = ((3*_previousVelocity.y) + panVelocity.y)/4;

    _previousVelocity = panVelocity;
    _currentVelocity  = CGPointMake(-velocityX, -velocityY);
}

#pragma mark - UIScrollViewDelegate

- (void)scrollViewWillBeginDragging:(UIScrollView *)scrollView
{
    _previousVelocity = [[scrollView panGestureRecognizer] velocityInView:scrollView];
    _dragging = YES;

    [self updateCurrentVelocity:scrollView];
    [scrollView.valdiViewNode notifyOnDragStartWithContentOffset:scrollView.contentOffset velocity:_currentVelocity];

    if (self.scrollPerfLoggerBridge) {
        [self.scrollPerfLoggerBridge resume];
    }

    if (_cancelsTouchesOnScroll) {
        _SCCancelDescendantGestureRecognizersForView(scrollView);
    }
}

- (void)scrollViewDidScroll:(UIScrollView *)scrollView
{
    if (_readjustingContentOffset) {
        return;
    }
    
    CGPoint updatedContentOffset = scrollView.contentOffset;

    [self updateCurrentVelocity:scrollView];

    [scrollView.valdiViewNode notifyOnScrollWithContentOffset:updatedContentOffset updatedContentOffset:&updatedContentOffset velocity:_dragging ? _currentVelocity : CGPointMake(0, 0)];

    if (!CGPointEqualToPoint(scrollView.contentOffset, updatedContentOffset)) {
        _readjustingContentOffset = YES;
        [scrollView setContentOffset:updatedContentOffset];
        _readjustingContentOffset = NO;
    }
}

- (void)scrollViewDidEndDecelerating:(UIScrollView *)scrollView
{
    [self scrollViewDidEndScrolling:scrollView];
}

- (void)scrollViewWillEndDragging:(UIScrollView *)scrollView
                     withVelocity:(CGPoint)velocity
              targetContentOffset:(inout CGPoint *)targetContentOffset
{
    _dragging = NO;
    [scrollView.valdiViewNode notifyOnDragEndingWithContentOffset:scrollView.contentOffset velocity:_currentVelocity updatedContentOffset:targetContentOffset];
}

- (void)scrollViewDidEndDragging:(UIScrollView *)scrollView willDecelerate:(BOOL)decelerate
{
    if (!decelerate) {
        [self scrollViewDidEndScrolling:scrollView];
    }
}

- (void)scrollViewDidEndScrollingAnimation:(UIScrollView *)scrollView
{
    [self scrollViewDidEndScrolling:scrollView];
}

- (void)scrollViewDidScrollToTop:(UIScrollView *)scrollView
{
    [self scrollViewDidEndScrolling:scrollView];
}

#pragma mark - Public methods

- (void)scrollViewDidEndScrolling:(UIScrollView *)scrollView
{
    [scrollView.valdiViewNode notifyOnScrollEndWithContentOffset:scrollView.contentOffset];

    if (self.scrollPerfLoggerBridge) {
        [self.scrollPerfLoggerBridge pauseAndCancelLogging:NO];
    }
}

#pragma mark - Static helpers

// Cancels all gesture recognizers for all of the given view's descendants
static void _SCCancelDescendantGestureRecognizersForView(UIView *view)
{
    for (UIView *subview in view.subviews) {
        for (UIGestureRecognizer *gestureRecognizer in subview.gestureRecognizers) {
            if (gestureRecognizer.enabled) {
                gestureRecognizer.enabled = NO;
                gestureRecognizer.enabled = YES;
            }
        }

        if (subview.subviews.count) {
            _SCCancelDescendantGestureRecognizersForView(subview);
        }
    }
}

@end
