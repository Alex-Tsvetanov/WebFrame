#import "valdi_core/SCValdiFunction.h"

#import <UIKit/UIKit.h>

@interface UIView (ValdiBase)

@property (strong, nonatomic) id<SCValdiFunction> valdiHitTest;

/**
 When you are making a change that can alter the view layout,
 you should call this to make sure the view tree will layout properly.

 Will call invalidateLayoutAndMarkFlexBoxDirty:YES
 */
- (void)invalidateLayout;

/**
 When you are making a change that can alter the view layout,
 you should call this and pass YES to make sure the view tree will layout properly.
*/
- (void)invalidateLayoutAndMarkFlexBoxDirty:(BOOL)markFlexBoxDirty;

/**
 Called before the view will be enqueued into the view pool.
 If this returns NO, the view will not pooled.
 */
- (BOOL)willEnqueueIntoValdiPool;

- (void)scrollSpecsDidChangeWithContentOffset:(CGPoint)contentOffset
                                  contentSize:(CGSize)contentSize
                                     animated:(BOOL)animated;

/**
 Whether this view should clipsToBounds by default.
 */
- (BOOL)clipsToBoundsByDefault;

/**
 Whether this view requires using a shape layer when applying border radius.
 */
- (BOOL)requiresShapeLayerForBorderRadius;

/**
 Returns whether this view requires a layout pass when animating its bounds.
 */
- (BOOL)requiresLayoutWhenAnimatingBounds;

/**
 Implementation of convertPoint:fromView: that takes in account animations
 */
- (CGPoint)valdi_convertPoint:(CGPoint)point fromView:(UIView*)view;

/**
 Implementation of convertPoint:toView:
 */
- (CGPoint)valdi_convertPoint:(CGPoint)point toView:(UIView*)view;

/**
 Implementation of hitTest:withEvent: supporting override by custom hit test
 */
- (UIView*)valdi_hitTest:(CGPoint)point withEvent:(UIEvent*)event withCustomHitTest:(id<SCValdiFunction>)customHitTest;

@end
